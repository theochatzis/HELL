"""Validation of trained models: ROC curves and AUC, score distributions
train vs test (overtraining check with a KS test), confusion summary;
for regressors: residuals, resolution vs target, response. All plots with
matplotlib; numbers to a JSON report."""
import json
import os

import numpy as np

from hell import log


def _plt():
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    return plt


def roc(scores, truth, weights):
    order = np.argsort(-scores)
    s, t, w = scores[order], truth[order], weights[order]
    tp = np.cumsum(w * t); fp = np.cumsum(w * (1 - t))
    tpr = tp / max(tp[-1], 1e-12); fpr = fp / max(fp[-1], 1e-12)
    auc = float(np.trapz(tpr, fpr))
    return fpr, tpr, auc


def ks_statistic(a, b, wa=None, wb=None):
    wa = np.ones(len(a)) if wa is None else wa
    wb = np.ones(len(b)) if wb is None else wb
    grid = np.sort(np.concatenate([a, b]))
    ca = np.array([wa[a <= g].sum() for g in grid[::max(1, len(grid) // 200)]]) / wa.sum()
    cb = np.array([wb[b <= g].sum() for g in grid[::max(1, len(grid) // 200)]]) / wb.sum()
    return float(np.max(np.abs(ca - cb)))


def validate_classifier(pred_train, pred_test, train_ds, test_ds, outdir, stem):
    plt = _plt()
    os.makedirs(outdir, exist_ok=True)
    report = {"classes": train_ds.class_names, "auc": {}, "ks": {}}
    for c, name in enumerate(train_ds.class_names):
        fpr, tpr, auc = roc(pred_test[:, c], (test_ds.y == c).astype(float), test_ds.w)
        report["auc"][name] = auc
        plt.figure(figsize=(5, 5)); plt.plot(fpr, tpr, label=f"{name} AUC={auc:.3f}"); plt.plot([0, 1], [0, 1], "k--", lw=0.5)
        plt.xlabel("false positive rate"); plt.ylabel("true positive rate"); plt.legend(); plt.title(f"ROC: {name} vs rest")
        plt.savefig(os.path.join(outdir, f"{stem}_roc_{name}.png"), dpi=120); plt.close()
        # Overtraining: score of class c for true-c and others, train vs test
        plt.figure(figsize=(6, 4))
        bins = np.linspace(0, 1, 26)
        for cc, cname in enumerate(train_ds.class_names):
            mtr, mte = train_ds.y == cc, test_ds.y == cc
            plt.hist(pred_train[mtr, c], bins=bins, weights=train_ds.w[mtr], density=True, histtype="step", label=f"{cname} train")
            plt.hist(pred_test[mte, c], bins=bins, weights=test_ds.w[mte], density=True, histtype="stepfilled", alpha=0.25, label=f"{cname} test")
            report["ks"][f"{name}|{cname}"] = ks_statistic(pred_train[mtr, c], pred_test[mte, c], train_ds.w[mtr], test_ds.w[mte])
        plt.xlabel(f"score {name}"); plt.ylabel("a.u."); plt.legend(fontsize=7); plt.yscale("log")
        plt.savefig(os.path.join(outdir, f"{stem}_score_{name}.png"), dpi=120); plt.close()
    # Score vs spectators (mass sculpting check): mean score in bins of each spectator
    for sname, svals in test_ds.spectators.items():
        plt.figure(figsize=(6, 4))
        edges = np.quantile(svals, np.linspace(0, 1, 21))
        centers = 0.5 * (edges[1:] + edges[:-1])
        for c, name in enumerate(train_ds.class_names):
            means = [np.average(pred_test[(svals >= edges[i]) & (svals < edges[i + 1]), c]) if ((svals >= edges[i]) & (svals < edges[i + 1])).any() else np.nan for i in range(20)]
            plt.plot(centers, means, "o-", ms=3, label=f"score {name}")
        plt.xlabel(sname); plt.ylabel("mean score"); plt.legend(fontsize=7); plt.title("score dependence on spectator")
        plt.savefig(os.path.join(outdir, f"{stem}_spectator_{sname}.png"), dpi=120); plt.close()
    with open(os.path.join(outdir, f"{stem}_validation.json"), "w") as f:
        json.dump(report, f, indent=1)
    log("validation: AUC " + ", ".join(f"{k}={v:.3f}" for k, v in report["auc"].items()) + f"; max KS {max(report['ks'].values()):.3f}")
    return report


def validate_regressor(pred_train, pred_test, train_ds, test_ds, outdir, stem, target_name="target"):
    plt = _plt()
    os.makedirs(outdir, exist_ok=True)
    res = pred_test[:, 0] - test_ds.y
    report = {"bias": float(np.average(res, weights=test_ds.w)), "resolution": float(np.sqrt(np.average(res ** 2, weights=test_ds.w))),
              "median_abs_residual": float(np.median(np.abs(res)))}
    plt.figure(figsize=(6, 4)); plt.hist(res, bins=80, weights=test_ds.w, histtype="step", label="test")
    plt.hist(pred_train[:, 0] - train_ds.y, bins=80, weights=train_ds.w, histtype="step", label="train")
    plt.xlabel(f"predicted - true {target_name}"); plt.legend(); plt.savefig(os.path.join(outdir, f"{stem}_residual.png"), dpi=120); plt.close()
    plt.figure(figsize=(5, 5)); plt.hist2d(test_ds.y, pred_test[:, 0], bins=60, cmin=1)
    plt.xlabel(f"true {target_name}"); plt.ylabel("predicted"); plt.savefig(os.path.join(outdir, f"{stem}_response.png"), dpi=120); plt.close()
    with open(os.path.join(outdir, f"{stem}_validation.json"), "w") as f:
        json.dump(report, f, indent=1)
    log(f"validation: bias {report['bias']:.3f}, resolution {report['resolution']:.3f}")
    return report
