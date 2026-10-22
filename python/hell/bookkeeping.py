"""Harvest the bookkeeping of produced files: sum of weights, event counts,
runs and lumi sections per sample, written to a JSON used for normalisation.

    hell bookkeep /eos/user/x/HELL/skims/2022 -o samples/bookkeeping/2022.json
"""
import glob
import json
import os

from hell import HellError, log, warn


def sample_dirs(root):
    out = {}
    for d in sorted(glob.glob(os.path.join(root, "*"))):
        if os.path.isdir(d):
            files = sorted(glob.glob(os.path.join(d, "**", "*.root"), recursive=True))
            if files:
                out[os.path.basename(d)] = files
    if not out:
        files = sorted(glob.glob(os.path.join(root, "*.root")))
        if files:
            out[os.path.basename(root.rstrip("/"))] = files
    return out


def harvest_files(files, with_lumis=True):
    import ROOT
    ROOT.gErrorIgnoreLevel = ROOT.kWarning
    rec = {"files": 0, "bad_files": [], "counts": {}, "runs": set(), "lumis": {}}
    for p in files:
        f = ROOT.TFile.Open(p)
        if not f or f.IsZombie():
            rec["bad_files"].append(p)
            continue
        rec["files"] += 1
        h = f.Get("hell/counts")
        if h:
            for i in range(1, h.GetNbinsX() + 1):
                lab = h.GetXaxis().GetBinLabel(i)
                if lab:
                    rec["counts"][lab] = rec["counts"].get(lab, 0.0) + h.GetBinContent(i)
        runs = f.Get("Runs")
        if runs and runs.GetBranch("run"):
            for e in runs:
                rec["runs"].add(int(e.run))
        if with_lumis:
            lumis = f.Get("LuminosityBlocks")
            if lumis and lumis.GetBranch("run"):
                for e in lumis:
                    rec["lumis"].setdefault(int(e.run), set()).add(int(e.luminosityBlock))
        f.Close()
    rec["runs"] = sorted(rec["runs"])
    rec["lumis"] = {str(r): sorted(v) for r, v in sorted(rec["lumis"].items())}
    return rec


def lumis_to_json_ranges(lumis):
    """{run: [ls...]} -> {run: [[first,last],...]} (Golden-JSON style)."""
    out = {}
    for run, ls in lumis.items():
        ranges = []
        for l in sorted(ls):
            if ranges and l == ranges[-1][1] + 1:
                ranges[-1][1] = l
            else:
                ranges.append([l, l])
        out[run] = ranges
    return out


def harvest(root, output=None, with_lumis=True):
    dirs = sample_dirs(root)
    if not dirs:
        raise HellError(f"no ROOT files under {root}")
    result = {}
    for name, files in dirs.items():
        rec = harvest_files(files, with_lumis)
        c = rec["counts"]
        result[name] = {
            "files": rec["files"], "bad_files": rec["bad_files"],
            "events_in": c.get("events_in", 0), "events_out": c.get("events_out", 0),
            "genEventCount": c.get("genEventCount", 0), "genEventSumw": c.get("genEventSumw", 0),
            "genEventSumw2": c.get("genEventSumw2", 0), "sumw_pos": c.get("sumw_pos", 0), "sumw_neg": c.get("sumw_neg", 0),
            "runs": rec["runs"], "processedLumis": lumis_to_json_ranges(rec["lumis"]) if with_lumis else {},
        }
        gen_n, gen_w = c.get("genEventCount", 0), c.get("genEventSumw", 0)
        if gen_n and c.get("events_in", 0) < gen_n:
            result[name]["derived"] = True   # produced from a skim: events_in counts skimmed events, genEventSumw all generated ones
        elif gen_w and abs(c.get("sumw_in", 0) - gen_w) > 1e-3 * abs(gen_w):
            warn(f"{name}: sum of processed weights ({c.get('sumw_in', 0):.6g}) differs from Runs genEventSumw ({gen_w:.6g}): were all events processed?")
        log(f"{name}: {rec['files']} files, events_in={c.get('events_in', 0):.0f}, events_out={c.get('events_out', 0):.0f}, genEventSumw={c.get('genEventSumw', 0):.6g}, runs={len(rec['runs'])}")
    if output:
        os.makedirs(os.path.dirname(os.path.abspath(output)), exist_ok=True)
        with open(output, "w") as f:
            json.dump(result, f, indent=1)
        log(f"bookkeeping written to {output}")
    return result
