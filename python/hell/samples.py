"""Sample registry: samples/<era>.yaml + samples/xsec.yaml + cached file lists.

samples/2022.yaml:
  era: "2022"
  lumi: 7980.4            # pb^-1
  goldenJSON: /eos/user/c/cmsdqm/www/CAF/certification/Collisions22/Cert_Collisions2022_355100_362760_Golden.json
  samples:
    TbarBQtoLNu_tch:
      das: /TbarBQtoLNu-t-channel-4FS_TuneCP5_13p6TeV_powheg-madspin-pythia8/Run3Summer22NanoAODv12-130X_mcRun3_2022_realistic_v5-v2/NANOAODSIM
      process: tch          # process name used by the histogrammer
      xsec: TbarBQtoLNu_tch # key in xsec.yaml, or a number in pb
      isData: false
    Muon_2022C:
      das: /Muon/Run2022C-22Sep2023-v1/NANOAOD
      process: data
      isData: true

File lists are cached in samples/files/<era>/<name>.txt (not in git).
They are made with dasgoclient (`hell samples files --era 2022`) or from a
local directory / glob given as `path:` instead of `das:`.
"""
import glob
import json
import os
import subprocess

import yaml

from hell import HELL_BASE, HellError, log, warn

SAMPLES_DIR = os.path.join(HELL_BASE, "samples")
FILES_DIR = os.path.join(SAMPLES_DIR, "files")


def eras():
    out = []
    for p in sorted(glob.glob(os.path.join(SAMPLES_DIR, "*.yaml"))):
        name = os.path.splitext(os.path.basename(p))[0]
        if name != "xsec":
            out.append(name)
    return out


def load_era(era):
    path = os.path.join(SAMPLES_DIR, f"{era}.yaml")
    if not os.path.exists(path):
        raise HellError(f"no sample file for era '{era}' ({path}). Known eras: {', '.join(eras())}")
    with open(path) as f:
        data = yaml.safe_load(f) or {}
    data.setdefault("era", era)
    data.setdefault("samples", {})
    for name, s in data["samples"].items():
        s.setdefault("name", name)
        s.setdefault("isData", False)
        s.setdefault("process", "data" if s["isData"] else name)
        s["era"] = era
    return data


def load_xsec():
    path = os.path.join(SAMPLES_DIR, "xsec.yaml")
    if not os.path.exists(path):
        return {}
    with open(path) as f:
        return yaml.safe_load(f) or {}


def cross_section(sample):
    """Cross section in pb of a sample entry (number or key into xsec.yaml)."""
    if sample.get("isData"):
        return 0.0
    x = sample.get("xsec", sample["name"])
    if isinstance(x, (int, float)):
        return float(x)
    table = load_xsec()
    if x not in table:
        raise HellError(f"no cross section '{x}' in samples/xsec.yaml for sample {sample['name']}")
    entry = table[x]
    return float(entry["xsec"] if isinstance(entry, dict) else entry)


def find_sample(name, era=None):
    """Find a sample by name across eras (or in one era). Returns the entry."""
    for e in ([era] if era else eras()):
        data = load_era(e)
        if name in data["samples"]:
            s = dict(data["samples"][name])
            s["lumi"] = data.get("lumi")
            s["goldenJSON"] = data.get("goldenJSON")
            return s
    raise HellError(f"sample '{name}' not found" + (f" in era {era}" if era else ""))


def filelist_path(era, name):
    return os.path.join(FILES_DIR, era, f"{name}.txt")


def _das_files(dataset, instance=None):
    q = f"file dataset={dataset}" + (f" instance={instance}" if instance else "")
    try:
        out = subprocess.run(["dasgoclient", "--query", q], capture_output=True, text=True, check=True).stdout
    except FileNotFoundError:
        raise HellError("dasgoclient not found: source a CMSSW environment or /cvmfs/cms.cern.ch/cmsset_default.sh")
    except subprocess.CalledProcessError as e:
        raise HellError(f"dasgoclient failed for {dataset}: {e.stderr.strip()}")
    return sorted(l.strip() for l in out.splitlines() if l.strip())


def scan(sample, redirector="root://xrootd-cms.infn.it/", prefer_eos=True, force=False):
    """Build and cache the file list of a sample. Returns the list of paths."""
    path = filelist_path(sample["era"], sample["name"])
    if os.path.exists(path) and not force:
        return files(sample)
    if "path" in sample:
        pats = sample["path"] if isinstance(sample["path"], list) else [sample["path"]]
        found = []
        for p in pats:
            if os.path.isdir(p):
                found += sorted(glob.glob(os.path.join(p, "**", "*.root"), recursive=True))
            else:
                found += sorted(glob.glob(p))
    elif "das" in sample:
        lfns = _das_files(sample["das"], sample.get("instance"))
        found = []
        for lfn in lfns:
            eos = "/eos/cms" + lfn
            found.append(eos if (prefer_eos and os.path.exists(eos)) else redirector + lfn)
    else:
        raise HellError(f"sample {sample['name']} has neither 'das' nor 'path'")
    if not found:
        warn(f"no files found for sample {sample['name']}")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write(f"# {sample.get('das', sample.get('path'))}\n")
        f.write("\n".join(found) + ("\n" if found else ""))
    log(f"{sample['name']}: {len(found)} files -> {path}")
    return found


def files(sample, max_files=-1, first=0):
    path = filelist_path(sample["era"], sample["name"])
    if not os.path.exists(path):
        return scan(sample)[first:(None if max_files < 0 else first + max_files)]
    with open(path) as f:
        lst = [l.strip() for l in f if l.strip() and not l.startswith("#")]
    return lst[first:(None if max_files < 0 else first + max_files)]


def resolve_inputs(input_files, sample_name, max_files=-1, first=0, era=None):
    """Explicit files win over the sample; returns (files, sample_entry_or_None)."""
    if input_files:
        return list(input_files), (find_sample(sample_name, era) if sample_name else None)
    if not sample_name:
        return [], None
    s = find_sample(sample_name, era)
    return files(s, max_files, first), s


def das_query(pattern, instance=None):
    q = f"dataset={pattern}" + (f" instance={instance}" if instance else "")
    try:
        out = subprocess.run(["dasgoclient", "--query", q], capture_output=True, text=True, check=True).stdout
    except FileNotFoundError:
        raise HellError("dasgoclient not found: source a CMSSW environment or /cvmfs/cms.cern.ch/cmsset_default.sh")
    return sorted(l.strip() for l in out.splitlines() if l.strip())


def suggest_name(dataset):
    """A readable sample name from a DAS dataset name: primary dataset without tune/energy/generator tail."""
    primary = dataset.strip("/").split("/")[0]
    for tail in ("_TuneCP5", "_Tune", "_13p6TeV", "_13TeV"):
        if tail in primary:
            primary = primary.split(tail)[0]
    return primary.replace("-", "_")
