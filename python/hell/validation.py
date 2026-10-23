"""Data/MC validation of object reconstruction and scale factors.

Input: hell-run outputs (one directory per sample) holding the histograms of
ObjectValidationAnalyzer (or any TH1 directory). MC is normalised to
lumi * xsec / genEventSumw like the histogrammer; data is the sum of the
data samples. For every histogram the tool reports:
  yield ratio data/MC, chi2/ndf (data Poisson and MC statistical errors),
  Kolmogorov-Smirnov distance and p-value of the shapes, the maximum
  |pull| and the number of bins with |pull| > 3,
for the weighted histogram and, when present, for its `_noSF` twin, so
the effect of the scale factors is visible side by side. Output: plots
with ratio panels, a JSON with all numbers and a markdown report.

configs/SingleTopMass/validation.yaml:
  era: "2022"
  input: /eos/user/x/HELL/trees/2022         # <input>/<sample>/*.root
  directory: objVal                           # histogram directory in the files
  processes:                                  # as in hist.yaml
    - {name: tch, label: "t-channel", color: "#e42536", samples: [TBbarQtoLNu_tch, TbarBQtoLNu_tch]}
    - {name: data, label: "Data", isData: true, samples: [Muon_2022C, Muon_2022D]}
  histograms: ["muons/*", "electrons/*", "jets/*", "event/*"]   # globs on <subdir>/<name>
  normalizeShapes: true    # metrics on MC scaled to the data yield (the yield ratio is reported anyway)
  output: validation/2022
"""
import fnmatch
import glob
import json
import math
import os

import numpy as np
import yaml

from hell import HellError, log, warn
from hell import samples as S


def _open(path):
    import ROOT
    f = ROOT.TFile.Open(path)
    if not f or f.IsZombie():
        raise HellError(f"cannot open {path}")
    return f


def read_sumw(files, label="genEventSumw"):
    total = 0.0
    for p in files:
        f = _open(p)
        h = f.Get("hell/counts")
        if h:
            b = h.GetXaxis().FindFixBin(label)
            if b > 0:
                total += h.GetBinContent(b)
        f.Close()
    return total


def list_histograms(file_path, directory):
    """All TH1 (not TH2/profile) paths under <directory>, as 'sub/name'."""
    import ROOT
    f = _open(file_path)
    d = f.Get(directory)
    if not d:
        f.Close()
        raise HellError(f"no directory {directory} in {file_path}")
    out = []

    def walk(dd, prefix):
        for k in dd.GetListOfKeys():
            o = k.ReadObj()
            if o.InheritsFrom("TDirectory"):
                walk(o, prefix + k.GetName() + "/")
            elif o.InheritsFrom("TH1") and not o.InheritsFrom("TH2") and not o.InheritsFrom("TProfile"):
                out.append(prefix + k.GetName())
    walk(d, "")
    f.Close()
    return out


def sum_histograms(files, path, scale=1.0):
    import ROOT
    ROOT.TH1.AddDirectory(False)
    total = None
    for p in files:
        f = _open(p)
        h = f.Get(path)
        if h:
            h = h.Clone()
            h.SetDirectory(0)
            if total is None:
                total = h
            else:
                total.Add(h)
        f.Close()
    if total is not None and scale != 1.0:
        total.Scale(scale)
    return total


def arrays(h):
    n = h.GetNbinsX()
    y = np.array([h.GetBinContent(i) for i in range(1, n + 1)])
    e = np.array([h.GetBinError(i) for i in range(1, n + 1)])
    return y, e


def metrics(data_h, mc_h, normalize_shapes=True):
    d, de = arrays(data_h)
    m, me = arrays(mc_h)
    nd, nm = d.sum(), m.sum()
    out = {"n_data": float(nd), "n_mc": float(nm), "yield_ratio": float(nd / nm) if nm > 0 else float("nan")}
    if nd <= 0 or nm <= 0:
        out.update({"chi2": float("nan"), "ndf": 0, "chi2_ndf": float("nan"), "ks": float("nan"), "ks_pvalue": float("nan"), "max_pull": float("nan"), "bad_bins": 0})
        return out
    scale = nd / nm if normalize_shapes else 1.0
    ms, mes = m * scale, me * scale
    var = de ** 2 + mes ** 2
    mask = (d > 0) | (ms > 0)
    var[var == 0] = 1.0
    pulls = (d - ms) / np.sqrt(var)
    chi2 = float((pulls[mask] ** 2).sum())
    ndf = int(mask.sum()) - (1 if normalize_shapes else 0)
    # KS on the cumulative shapes; effective sample sizes for the p-value
    cd, cm = np.cumsum(d) / nd, np.cumsum(m) / nm
    ks = float(np.max(np.abs(cd - cm)))
    n_eff_mc = float(nm ** 2 / max((me ** 2).sum(), 1e-12))
    n_eff = nd * n_eff_mc / (nd + n_eff_mc)
    z = (math.sqrt(n_eff) + 0.12 + 0.11 / math.sqrt(n_eff)) * ks if n_eff > 0 else 0.0
    pval = 2 * sum((-1) ** (k - 1) * math.exp(-2 * k * k * z * z) for k in range(1, 100)) if z > 0 else 1.0
    out.update({"chi2": chi2, "ndf": ndf, "chi2_ndf": chi2 / ndf if ndf > 0 else float("nan"), "ks": ks,
                "ks_pvalue": float(min(max(pval, 0.0), 1.0)), "max_pull": float(np.max(np.abs(pulls[mask]))) if mask.any() else 0.0,
                "bad_bins": int((np.abs(pulls[mask]) > 3).sum())})
    return out


def plot(data_h, mc_hists, labels, colors, title, out_path, normalize_shapes, text, lumi_text=None):
    from hell.plot import stack_with_ratio
    stack_with_ratio(title.replace("/", "_"), data_h, mc_hists, labels, colors, mc_hists[0].GetXaxis().GetTitle(), lumi_text, out_path,
                     region_label=title, info_text=text, normalize_to_data=normalize_shapes)


def validate(yaml_path, output=None, max_hists=None):
    with open(yaml_path) as f:
        cfg = yaml.safe_load(f)
    era = str(cfg.get("era", ""))
    era_data = S.load_era(era)
    lumi = float(cfg.get("lumi", era_data.get("lumi", 1.0)))
    directory = cfg.get("directory", "objVal")
    normalize = bool(cfg.get("normalizeShapes", True))
    outdir = output or cfg.get("output", "validation")
    os.makedirs(os.path.join(outdir, "plots"), exist_ok=True)
    # Resolve files and scales per process
    procs = []
    for p in cfg["processes"]:
        entry = {"name": p["name"], "label": p.get("label", p["name"]), "color": p.get("color", "#888888"), "isData": bool(p.get("isData", False)), "samples": []}
        for sname in p["samples"]:
            files = sorted(glob.glob(os.path.join(cfg["input"], sname, "**", "*.root"), recursive=True)) or sorted(glob.glob(os.path.join(cfg["input"], f"{sname}*.root")))
            if not files:
                warn(f"{p['name']}: no files for sample {sname}")
                continue
            scale = 1.0
            if not entry["isData"]:
                sample = era_data["samples"].get(sname)
                if sample is None:
                    raise HellError(f"sample {sname} not in samples/{era}.yaml")
                sumw = read_sumw(files)
                if sumw <= 0:
                    raise HellError(f"sample {sname}: no genEventSumw in the files")
                scale = lumi * S.cross_section(sample) / sumw
            entry["samples"].append({"name": sname, "files": files, "scale": scale})
        if entry["samples"]:
            procs.append(entry)
    data_procs = [p for p in procs if p["isData"]]
    mc_procs = [p for p in procs if not p["isData"]]
    if not mc_procs:
        raise HellError("no MC process with files")
    first_file = mc_procs[0]["samples"][0]["files"][0]
    all_hists = list_histograms(first_file, directory)
    patterns = cfg.get("histograms", ["*"])
    selected = [h for h in all_hists if any(fnmatch.fnmatch(h, pat) for pat in patterns)]
    if max_hists:
        selected = selected[:max_hists]
    from hell import tdr
    lumi_text = tdr.lumi_text(lumi, cfg.get("extraText", "Preliminary" if data_procs else "Simulation"))
    log(f"validating {len(selected)} histograms of {directory}/ : {len(mc_procs)} MC processes, {len(data_procs)} data processes, lumi {lumi} pb^-1")
    results = {}
    for hp in selected:
        full = f"{directory}/{hp}"
        mc_hs, labels, colors = [], [], []
        for p in mc_procs:
            tot = None
            for s in p["samples"]:
                h = sum_histograms(s["files"], full, s["scale"])
                if h is None:
                    continue
                tot = h if tot is None else (tot.Add(h) or tot)
            if tot is not None:
                mc_hs.append(tot); labels.append(p["label"]); colors.append(p["color"])
        data_h = None
        for p in data_procs:
            for s in p["samples"]:
                h = sum_histograms(s["files"], full)
                if h is not None:
                    data_h = h if data_h is None else (data_h.Add(h) or data_h)
        if not mc_hs:
            continue
        mc_total = mc_hs[0].Clone("mc_total"); mc_total.SetDirectory(0)
        for h in mc_hs[1:]:
            mc_total.Add(h)
        rec = {"histogram": hp}
        if data_h is not None:
            rec.update(metrics(data_h, mc_total, normalize))
            text = "N_{data}/N_{MC}=%.3f, #chi^{2}/ndf=%.1f/%d, KS p=%.3f, max|pull|=%.1f" % (rec["yield_ratio"], rec["chi2"], rec["ndf"], rec["ks_pvalue"], rec["max_pull"])
        else:
            rec.update({"n_mc": float(mc_total.Integral())})
            text = "no data process: MC only"
        plot(data_h, mc_hs, labels, colors, hp, os.path.join(outdir, "plots", hp.replace("/", "_") + ".png"), normalize, text, lumi_text)
        results[hp] = rec
    # Report: pair weighted / _noSF histograms
    rows = []
    for hp, rec in results.items():
        if hp.endswith("_noSF"):
            continue
        twin = results.get(hp + "_noSF")
        rows.append((hp, rec, twin))
    with open(os.path.join(outdir, "validation.json"), "w") as f:
        json.dump({"config": cfg, "lumi": lumi, "normalizeShapes": normalize, "results": results}, f, indent=1)
    md = [f"# Object validation report ({era}, {lumi} pb^-1, shapes {'normalised to data' if normalize else 'absolute'})", "",
          "| histogram | N_data | N_MC | data/MC | chi2/ndf | KS p | max pull | bad bins | chi2/ndf noSF | KS p noSF | max pull noSF |",
          "|---|---|---|---|---|---|---|---|---|---|---|"]
    bad = 0
    for hp, rec, twin in rows:
        if "chi2_ndf" not in rec:
            md.append(f"| {hp} | - | {rec['n_mc']:.1f} | - | - | - | - | - | - | - | - |")
            continue
        flag = " **!**" if (rec["ks_pvalue"] < 0.01 or rec["bad_bins"] > 0) else ""
        bad += 1 if flag else 0
        t = twin or {}
        c2 = lambda r: f"{r['chi2_ndf']:.2f}" if r["ndf"] > 0 else "-"
        md.append(f"| {hp}{flag} | {rec['n_data']:.0f} | {rec['n_mc']:.1f} | {rec['yield_ratio']:.3f} | {c2(rec)} | {rec['ks_pvalue']:.3f} | {rec['max_pull']:.1f} | {rec['bad_bins']} |"
                  + (f" {c2(t)} | {t['ks_pvalue']:.3f} | {t['max_pull']:.1f} |" if t else " - | - | - |"))
    md += ["", "Metrics: shapes compared with MC scaled to the data yield when normalizeShapes is true; chi2 uses data Poisson and MC statistical",
           "errors; KS p-value uses the effective sample sizes; a row is flagged (!) when KS p < 0.01 or a bin has |pull| > 3.",
           "Columns 'noSF' are the same histogram filled without the object scale factors: they should agree worse than the nominal ones",
           "if the scale factors are applied correctly (and the yield ratio moves towards 1).", "",
           "Plots: `plots/<subdir>_<name>.png`."]
    with open(os.path.join(outdir, "validation.md"), "w") as f:
        f.write("\n".join(md) + "\n")
    log(f"report: {outdir}/validation.md ({len(rows)} distributions, {bad} flagged), plots in {outdir}/plots/")
    return results
