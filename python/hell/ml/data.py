"""Datasets from HELL analysis trees.

configs/SingleTopMass/ml.yaml:
  task: classifier                   # classifier | regressor
  treeName: Events
  input: /eos/user/x/HELL/trees/2022   # <input>/<sample>/*.root, or absolute file lists per class
  preselection: "st_nJet==2 && st_nBJet==1 && st_nLepton==1"
  features: [st_lightJetAbsEta, st_mlb, st_mtw, st_cosThetaStar, st_dEtaBQ, st_ht, st_leptonPt, st_bJetPt]
  spectators: [st_topMass]
  weight: Weight_gen                 # per-event weight expression (optional); |w| is used for training
  classes:                           # classifier: name -> samples
    signal: [TbarBQtoLNu_tch, TBbarQtoLNu_tch]
    top:    [TTtoLNu2Q]
    wjets:  [WtoLNu_2Jets]
  target: st_topMass                 # regressor only
  maxEventsPerSample: 200000
  testFraction: 0.3
  model: {hidden: [64, 64], activation: relu, dropout: 0.1}
  training: {epochs: 30, batchSize: 512, lr: 1e-3, weightDecay: 1e-5, seed: 42}
  output: models/st_classifier
"""
import glob
import os

import numpy as np
import yaml

from hell import HellError, log, warn


def load_yaml(path):
    with open(path) as f:
        cfg = yaml.safe_load(f)
    cfg.setdefault("task", "classifier")
    cfg.setdefault("treeName", "Events")
    cfg.setdefault("spectators", [])
    cfg.setdefault("testFraction", 0.3)
    cfg.setdefault("maxEventsPerSample", -1)
    return cfg


def sample_files(input_dir, sample):
    if os.path.isfile(sample):
        return [sample]
    files = sorted(glob.glob(os.path.join(input_dir, sample, "**", "*.root"), recursive=True))
    if not files:
        files = sorted(glob.glob(os.path.join(input_dir, f"{sample}*.root")))
    return files


def read_arrays(files, tree, expressions, selection, max_events=-1):
    """Evaluate expressions with ROOT (TTree::Draw syntax works for any HELL tree)."""
    from hell.rootutils import quiet_root
    ROOT = quiet_root()
    cols = {e: [] for e in expressions}
    n_total = 0
    for path in files:
        f = ROOT.TFile.Open(path)
        if not f or f.IsZombie():
            warn(f"cannot open {path}")
            continue
        t = f.Get(tree)
        if not t:
            warn(f"no tree {tree} in {path}")
            continue
        t.SetEstimate(t.GetEntries() + 1)
        for e in expressions:
            n = t.Draw(e, selection or "", "goff")
            if n < 0:
                raise HellError(f"invalid expression '{e}' or selection '{selection}' in {path}")
            v = t.GetV1()
            cols[e].append(np.array([v[i] for i in range(n)], dtype=np.float32))
        n_total += len(cols[expressions[0]][-1])
        f.Close()
        if max_events > 0 and n_total >= max_events:
            break
    out = {e: (np.concatenate(v) if v else np.zeros(0, dtype=np.float32)) for e, v in cols.items()}
    if max_events > 0:
        out = {e: v[:max_events] for e, v in out.items()}
    return out


class Dataset:
    """Feature matrix X, target y (class index or regression target), weights w, spectators."""

    def __init__(self, X, y, w, spectators, class_names, feature_names, spectator_names):
        self.X, self.y, self.w, self.spectators = X, y, w, spectators
        self.class_names, self.feature_names, self.spectator_names = class_names, feature_names, spectator_names

    def split(self, test_fraction, seed=42):
        rng = np.random.default_rng(seed)
        idx = rng.permutation(len(self.y))
        n_test = int(len(idx) * test_fraction)
        te, tr = idx[:n_test], idx[n_test:]
        return self.subset(tr), self.subset(te)

    def subset(self, idx):
        return Dataset(self.X[idx], self.y[idx], self.w[idx], {k: v[idx] for k, v in self.spectators.items()},
                       self.class_names, self.feature_names, self.spectator_names)

    def __len__(self):
        return len(self.y)


def build(cfg):
    feats = list(cfg["features"])
    spect = list(cfg.get("spectators", []))
    weight = cfg.get("weight")
    exprs = feats + spect + ([weight] if weight else [])
    if cfg["task"] == "regressor":
        exprs.append(cfg["target"])
    Xs, ys, ws, sp = [], [], [], {s: [] for s in spect}
    if cfg["task"] == "classifier":
        class_names = list(cfg["classes"].keys())
        groups = [(i, cfg["classes"][c]) for i, c in enumerate(class_names)]
    else:
        class_names = []
        groups = [(0, cfg["samples"])]
    for label, samples in groups:
        for s in samples:
            files = sample_files(cfg["input"], s)
            if not files:
                warn(f"no files for sample {s}")
                continue
            arr = read_arrays(files, cfg["treeName"], exprs, cfg.get("preselection", ""), cfg["maxEventsPerSample"])
            n = len(arr[feats[0]])
            if n == 0:
                warn(f"sample {s}: no events after preselection")
                continue
            Xs.append(np.stack([arr[f] for f in feats], axis=1))
            ys.append(np.full(n, label, dtype=np.float32) if cfg["task"] == "classifier" else arr[cfg["target"]])
            ws.append(np.abs(arr[weight]) if weight else np.ones(n, dtype=np.float32))
            for sname in spect:
                sp[sname].append(arr[sname])
            log(f"{s}: {n} events")
    if not Xs:
        raise HellError("no training events at all")
    X = np.concatenate(Xs); y = np.concatenate(ys); w = np.concatenate(ws)
    spectators = {k: np.concatenate(v) for k, v in sp.items()}
    # Classifier: equalise the total weight of the classes so that none dominates.
    if cfg["task"] == "classifier":
        for c in range(len(class_names)):
            m = y == c
            if w[m].sum() > 0:
                w[m] *= w.sum() / len(class_names) / w[m].sum()
    good = np.isfinite(X).all(axis=1) & np.isfinite(y) & np.isfinite(w)
    if (~good).sum():
        warn(f"dropping {(~good).sum()} events with non-finite values")
    return Dataset(X[good], y[good], w[good], {k: v[good] for k, v in spectators.items()}, class_names, feats, spect)
