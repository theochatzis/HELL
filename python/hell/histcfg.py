"""Build the JSON for hell-hist from a YAML analysis histogram config.

configs/SingleTopMass/hist.yaml:
  era: "2022"                       # lumi and samples from samples/2022.yaml
  input: /eos/user/x/HELL/trees/2022   # <input>/<sample>/*.root (analysis trees)
  variationInput: /eos/user/x/HELL/trees/2022_{variation}   # optional, for tree systematics
  treeName: Events
  output: hists/2022.root
  processes:
    - {name: tch,   label: "t-channel", color: "#e42536", samples: [TbarBQtoLNu_tch, TBbarQtoLNu_tch]}
    - {name: tt,    label: "t#bar{t}", color: "#f89c20", samples: [TTtoLNu2Q, TTto2L2Nu]}
    - {name: data,  label: "Data", color: "#000000", isData: true, samples: [Muon_2022C]}
  regions:
    2j1b_mu: {selection: "st_nJet==2 && st_nBJet==1 && st_isMuon==1", weight: "Weight_nominal"}
  variables:
    topMass: {expr: st_topMass, bins: [20, 100, 300], title: "m_{l#nub} [GeV]"}
  systematics:
    pu:  {type: weight, up: "Weight_puUp/Weight_nominal", down: "Weight_puDown/Weight_nominal"}
    jes: {type: tree, up: jesUp, down: jesDown}
"""
import glob
import json
import os

import yaml

from hell import HellError, generated_by, log, warn
from hell import samples as S


def _files(pattern_dir, sample):
    cands = sorted(glob.glob(os.path.join(pattern_dir, sample, "**", "*.root"), recursive=True))
    if not cands:
        cands = sorted(glob.glob(os.path.join(pattern_dir, f"{sample}*.root")))
    return cands


def build(yaml_path, output=None, processes=None, lumi=None):
    with open(yaml_path) as f:
        cfg = yaml.safe_load(f)
    era = str(cfg.get("era", ""))
    era_data = S.load_era(era) if era else {"samples": {}, "lumi": 1.0}
    out = {
        "generatedBy": generated_by(),
        "output": output or cfg.get("output", "hists.root"),
        "treeName": cfg.get("treeName", "Events"),
        "lumi": float(lumi if lumi is not None else cfg.get("lumi", era_data.get("lumi", 1.0))),
        "regions": cfg["regions"],
        "variables": cfg["variables"],
        "systematics": cfg.get("systematics", {}),
        "processes": [],
    }
    tree_variations = {s["up"] for s in out["systematics"].values() if s.get("type") == "tree"} | \
                      {s["down"] for s in out["systematics"].values() if s.get("type") == "tree"}
    input_dir = cfg["input"]
    var_input = cfg.get("variationInput", input_dir.rstrip("/") + "_{variation}")
    for p in cfg["processes"]:
        if processes and p["name"] not in processes:
            continue
        entry = {"name": p["name"], "label": p.get("label", p["name"]), "color": p.get("color", "#000000"),
                 "isData": bool(p.get("isData", False)), "cut": p.get("cut", ""), "samples": []}
        for sname in p["samples"]:
            files = _files(input_dir, sname)
            if not files:
                warn(f"process {p['name']}: no files for sample {sname} under {input_dir}")
                continue
            s = {"name": sname, "files": files}
            if not entry["isData"]:
                sample = era_data["samples"].get(sname)
                if sample is None:
                    raise HellError(f"sample {sname} is not in samples/{era}.yaml (needed for its cross section)")
                s["xsec"] = S.cross_section(sample)
                s["sumw"] = float(p.get("sumw", {}).get(sname, 0.0)) if isinstance(p.get("sumw"), dict) else 0.0
                s["variations"] = {}
                for v in sorted(tree_variations):
                    vf = _files(var_input.format(variation=v), sname)
                    if vf:
                        s["variations"][v] = vf
            entry["samples"].append(s)
        out["processes"].append(entry)
    return out


def write(yaml_path, json_path, **kw):
    j = build(yaml_path, **kw)
    os.makedirs(os.path.dirname(os.path.abspath(json_path)), exist_ok=True)
    with open(json_path, "w") as f:
        json.dump(j, f, indent=1)
    log(f"histogram configuration written to {json_path} ({len(j['processes'])} processes)")
    return json_path
