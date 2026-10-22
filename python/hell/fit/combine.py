"""Wrapper around the standalone Higgs Combine tool.

configs/SingleTopMass/fit.yaml:
  histograms: hists/2022.root
  variable: topMass
  regions: [2j1b_mu, 2j1b_el]
  signal: [tch]
  backgrounds: [tt, wjets]
  data: data
  shapeSystematics: [pu, jes]
  normalisation:
    lumi: {value: 1.014, processes: ["*"]}
    xsec_tt: {value: 1.05, processes: [tt]}
  autoMCStats: 10
  output: cards/st
  parametric:                      # optional, for `hell fit --parametric`
    region: 2j1b_mu
    signal: tch
    massPoints: {"166.5": tch_mt1665, "169.5": tch_mt1695, "171.5": tch_mt1715, "173.5": tch_mt1735, "175.5": tch_mt1755, "178.5": tch_mt1785}
  combine:
    method: FitDiagnostics       # or MultiDimFit, AsymptoticLimits, ...
    poi: r                       # mtop for the parametric fit
    extra: "--rMin -5 --rMax 5"

`hell fit` sources $HELL_COMBINE_SETUP (a script that puts `combine` on the PATH),
writes the datacards and runs combine with the chosen --strategy.
"""
import json
import os
import shutil
import subprocess

import yaml

from hell import HellError, log, warn
from hell.runner import find_executable


def load(yaml_path):
    with open(yaml_path) as f:
        return yaml.safe_load(f)


def write_datacards(cfg, parametric=False, workdir="work"):
    os.makedirs(workdir, exist_ok=True)
    j = {k: v for k, v in cfg.items() if k not in ("combine", "parametric")}
    if parametric:
        p = cfg.get("parametric", {})
        j.update({"region": p.get("region", cfg["regions"][0]), "signal": p.get("signal", cfg["signal"][0]),
                  "massPoints": p.get("massPoints", {}), "output": p.get("output", cfg.get("output", "cards/hell") + "_parametric")})
    path = os.path.join(workdir, "fit_" + os.path.basename(cfg.get("output", "cards/hell")) + ("_parametric" if parametric else "") + ".json")
    with open(path, "w") as f:
        json.dump(j, f, indent=1)
    cmd = [find_executable("hell-datacard"), path] + (["--parametric"] if parametric else [])
    log(" ".join(cmd))
    out = subprocess.run(cmd, capture_output=True, text=True)
    print(out.stdout, end="")
    if out.returncode != 0:
        raise HellError("hell-datacard failed:\n" + out.stderr)
    cards = [l.strip() for l in out.stdout.splitlines() if l.strip().endswith(".txt") and not l.startswith("[")]
    return cards


def combine_cards(cards, output, dry_run=False):
    if len(cards) == 1:
        return cards[0]
    cmd = "combineCards.py " + " ".join(f"{os.path.basename(c).replace('.txt', '')}={os.path.abspath(c)}" for c in cards)
    log(cmd + f" > {output}")
    if dry_run:
        return output
    prefix = combine_env_prefix()
    with open(output, "w") as f:
        rc = subprocess.call(prefix + cmd, shell=True, stdout=f, executable="/bin/bash")
    if rc != 0:
        raise HellError("combineCards.py failed")
    return output


def combine_env_prefix():
    setup = os.environ.get("HELL_COMBINE_SETUP", "")
    if setup:
        return f"source {setup} >/dev/null 2>&1 && "
    if shutil.which("combine"):
        return ""
    raise HellError("combine not found. Set HELL_COMBINE_SETUP to a script that provides it (standalone Combine from cvmfs/CMSSW), "
                    "or use --dry-run to only write the datacards.")


def run_combine(card, cfg, strategy=1, extra=(), dry_run=False, name="HELL"):
    c = cfg.get("combine", {})
    method = c.get("method", "FitDiagnostics")
    poi = c.get("poi", "r")
    cmd = f"combine -M {method} {card} -n _{name} --cminDefaultMinimizerStrategy {strategy} {c.get('extra', '')} " + " ".join(extra)
    if poi != "r":
        cmd += f" --redefineSignalPOIs {poi}" if method != "MultiDimFit" else f" -P {poi}"
    log("[strategy %d] %s" % (strategy, cmd))
    if dry_run:
        return 0
    prefix = combine_env_prefix()
    rc = subprocess.call(prefix + cmd, shell=True, executable="/bin/bash")
    if rc != 0:
        log("Fit failed to converge. Try `hell fit --strategy 2`, or `hell pray`.", "ERROR")
    else:
        log("Fit converged. The forge is pleased.")
    return rc
