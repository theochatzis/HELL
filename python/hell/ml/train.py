"""Training loop for classifiers (cross-entropy, softmax scores) and
regressors (Huber loss). Inputs are standardised; the scaler is exported
with the model so that the C++ evaluation is identical."""
import json
import os
import time

import numpy as np

from hell import log
from hell.ml.models import make_mlp, _torch


class Scaler:
    def __init__(self, X):
        self.mean = X.mean(axis=0)
        self.scale = X.std(axis=0)
        self.scale[self.scale == 0] = 1.0

    def __call__(self, X):
        return (X - self.mean) / self.scale


def train(cfg, train_ds, test_ds, output_dir):
    torch = _torch()
    t0 = time.time()
    tcfg = dict({"epochs": 30, "batchSize": 512, "lr": 1e-3, "weightDecay": 1e-5, "seed": 42}, **cfg.get("training", {}))
    mcfg = dict({"hidden": [64, 64], "activation": "relu", "dropout": 0.0}, **cfg.get("model", {}))
    torch.manual_seed(tcfg["seed"]); np.random.seed(tcfg["seed"])
    classifier = cfg["task"] == "classifier"
    n_out = len(train_ds.class_names) if classifier else 1
    scaler = Scaler(train_ds.X)
    model = make_mlp(train_ds.X.shape[1], n_out, mcfg["hidden"], mcfg["activation"], mcfg["dropout"])
    device = "cuda" if torch.cuda.is_available() else "cpu"
    model.to(device)
    opt = torch.optim.Adam(model.parameters(), lr=tcfg["lr"], weight_decay=tcfg["weightDecay"])
    Xtr = torch.tensor(scaler(train_ds.X), dtype=torch.float32, device=device)
    Xte = torch.tensor(scaler(test_ds.X), dtype=torch.float32, device=device)
    ytr = torch.tensor(train_ds.y, dtype=torch.long if classifier else torch.float32, device=device)
    yte = torch.tensor(test_ds.y, dtype=torch.long if classifier else torch.float32, device=device)
    wtr = torch.tensor(train_ds.w, dtype=torch.float32, device=device)
    wte = torch.tensor(test_ds.w, dtype=torch.float32, device=device)
    if classifier:
        def loss_fn(out, y, w):
            return (torch.nn.functional.cross_entropy(out, y, reduction="none") * w).sum() / w.sum()
    else:
        def loss_fn(out, y, w):
            return (torch.nn.functional.huber_loss(out.squeeze(1), y, reduction="none", delta=20.0) * w).sum() / w.sum()
    history = []
    best = (float("inf"), None)
    n = len(Xtr)
    log(f"training {cfg['task']} on {n} events ({len(Xte)} test), {sum(p.numel() for p in model.parameters())} parameters, device {device}")
    for epoch in range(tcfg["epochs"]):
        model.train()
        perm = torch.randperm(n, device=device)
        tot = 0.0
        for i in range(0, n, tcfg["batchSize"]):
            idx = perm[i:i + tcfg["batchSize"]]
            opt.zero_grad()
            loss = loss_fn(model(Xtr[idx]), ytr[idx], wtr[idx])
            loss.backward(); opt.step()
            tot += loss.item() * len(idx)
        model.eval()
        with torch.no_grad():
            test_loss = loss_fn(model(Xte), yte, wte).item()
        history.append({"epoch": epoch + 1, "train": tot / n, "test": test_loss})
        if test_loss < best[0]:
            best = (test_loss, {k: v.detach().clone() for k, v in model.state_dict().items()})
        if (epoch + 1) % max(1, tcfg["epochs"] // 10) == 0 or epoch == 0:
            log(f"epoch {epoch + 1:3d}: train loss {tot / n:.4f}  test loss {test_loss:.4f}")
    if best[1] is not None:
        model.load_state_dict(best[1])
    model.eval()
    os.makedirs(output_dir, exist_ok=True)
    stem = os.path.join(output_dir, os.path.basename(cfg.get("output", "model")))
    torch.save(model.state_dict(), stem + ".pt")
    with open(stem + "_history.json", "w") as f:
        json.dump(history, f, indent=1)
    log(f"best test loss {best[0]:.4f}; model saved to {stem}.pt ({time.time() - t0:.0f} s)")
    return model, scaler, history, stem


def predict(model, scaler, X):
    torch = _torch()
    model.eval()
    with torch.no_grad():
        out = model(torch.tensor(scaler(X), dtype=torch.float32, device=next(model.parameters()).device))
        if out.shape[1] > 1:
            out = torch.softmax(out, dim=1)
    return out.cpu().numpy()
