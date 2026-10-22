"""Entry points of `hell train` / `hell validate`."""
import os

from hell import log
from hell.ml import data as D
from hell.ml.train import train, predict
from hell.ml import export as E
from hell.ml import validate as V


def train_from_yaml(yaml_path, output_dir=None, epochs=None, max_events=None):
    cfg = D.load_yaml(yaml_path)
    if epochs:
        cfg.setdefault("training", {})["epochs"] = epochs
    if max_events:
        cfg["maxEventsPerSample"] = max_events
    ds = D.build(cfg)
    tr, te = ds.split(cfg["testFraction"], cfg.get("training", {}).get("seed", 42))
    outdir = output_dir or os.path.dirname(cfg.get("output", "models/model")) or "models"
    model, scaler, history, stem = train(cfg, tr, te, outdir)
    pred_tr, pred_te = predict(model, scaler, tr.X), predict(model, scaler, te.X)
    E.to_json(model, scaler, ds.feature_names, ds.class_names, cfg["task"], stem + ".json")
    try:
        E.to_torchscript(model, ds.X.shape[1], stem + ".ts")
    except Exception as e:  # TorchScript is optional
        log(f"TorchScript export skipped: {e}", "WARNING")
    if cfg["task"] == "classifier":
        V.validate_classifier(pred_tr, pred_te, tr, te, outdir, os.path.basename(stem))
    else:
        V.validate_regressor(pred_tr, pred_te, tr, te, outdir, os.path.basename(stem), cfg.get("target", "target"))
    return stem
