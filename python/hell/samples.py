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

from hell import HELL_BASE, HellError, generated_by, log, warn

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
    if not data.get("goldenJSON"):   # fall back to the master certification list
        try:
            from hell.lumi import golden_json
            data["goldenJSON"] = golden_json(era)
        except HellError:
            pass
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
    value = entry["xsec"] if isinstance(entry, dict) else entry
    if value is None:
        raise HellError(f"cross section '{x}' in samples/xsec.yaml is not filled yet (sample {sample['name']})")
    return float(value)


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


def _dasgoclient():
    import shutil
    exe = shutil.which("dasgoclient") or "/cvmfs/cms.cern.ch/common/dasgoclient"
    if not os.path.exists(exe):
        raise HellError("dasgoclient not found: source /cvmfs/cms.cern.ch/cmsset_default.sh or a CMSSW environment")
    return exe


def das(query, instance=None):
    """Run a dasgoclient query and return the output lines. Needs a VOMS proxy (voms-proxy-init --voms cms --rfc)."""
    full = query + (f" instance={instance}" if instance else "")
    # the cvmfs dasgoclient wrapper runs the system python: the LCG PYTHONHOME/PYTHONPATH must not leak into it
    env = {k: v for k, v in os.environ.items() if k not in ("PYTHONHOME", "PYTHONPATH")}
    result = subprocess.run([_dasgoclient(), "--query", full], capture_output=True, text=True, env=env)
    if result.returncode != 0 or "X509_USER_PROXY" in result.stderr:
        message = (result.stderr or result.stdout).strip()
        if "X509" in message or "proxy" in message.lower():
            raise HellError("DAS needs a grid proxy: run `voms-proxy-init --voms cms --rfc` and export X509_USER_PROXY if it is not in /tmp")
        raise HellError(f"dasgoclient failed for '{full}': {message}")
    return sorted(line.strip() for line in result.stdout.splitlines() if line.strip())


def _das_files(dataset, instance=None):
    return das(f"file dataset={dataset}", instance)


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
    return das(f"dataset={pattern}", instance)


def is_data_dataset(dataset):
    return dataset.rstrip("/").endswith("/NANOAOD")


def suggest_name(dataset):
    """A readable sample name from a DAS dataset name.
    MC: the primary dataset without the tune/energy/generator tail (TbarBQtoLNu-t-channel-4FS -> TbarBQtoLNu_t_channel_4FS).
    Data: <primary dataset>_<run era> (/Muon/Run2022C-22Sep2023-v1/NANOAOD -> Muon_2022C)."""
    parts = dataset.strip("/").split("/")
    primary = parts[0]
    if is_data_dataset(dataset) and len(parts) > 1:
        import re
        match = re.match(r"Run(\d{4}[A-Z]?)", parts[1])
        return f"{primary}_{match.group(1)}" if match else primary
    for tail in ("_TuneCP5", "_Tune", "_13p6TeV", "_13TeV"):
        if tail in primary:
            primary = primary.split(tail)[0]
    return primary.replace("-", "_")


def create_entries(datasets, process=None, xsec=None, name=None):
    """Sample entries (dicts) for DAS dataset names. MC entries get an xsec key `<name>_13p6TeV` to be filled
    in samples/xsec.yaml unless `xsec` is given; the process defaults to `data` for data and the name for MC."""
    entries = {}
    for dataset in datasets:
        data = is_data_dataset(dataset)
        sample_name = name if (name and len(datasets) == 1) else suggest_name(dataset)
        entry = {"das": dataset, "process": process or ("data" if data else sample_name)}
        if data:
            entry["isData"] = True
        else:
            entry["xsec"] = xsec or f"{sample_name}_13p6TeV"
        entries[sample_name] = entry
    return entries


def write_era(era, entries, lumi=None, golden_json=None, campaign=None, output=None, dry_run=False):
    """Create samples/<era>.yaml or append new entries to it (existing names and datasets are kept untouched).
    Returns (added names, skipped names)."""
    import datetime
    path = output or os.path.join(SAMPLES_DIR, f"{era}.yaml")
    existing = {}
    if os.path.exists(path):
        with open(path) as f:
            existing = (yaml.safe_load(f) or {}).get("samples", {}) or {}
    known_das = {s.get("das") for s in existing.values()}
    added, skipped = [], []
    lines = []
    if not os.path.exists(path):
        lines += [f"# {generated_by()}",
                  f"# Samples of the {era} era. lumi, goldenJSON and the cross sections in samples/xsec.yaml are filled by",
                  "# hand and verified. process: the name used by the histogrammer.",
                  f'era: "{era}"',
                  f"lumi: {lumi if lumi is not None else 0.0}   # pb^-1, from brilcalc on the processed lumi sections",
                  f"goldenJSON: {golden_json or ''}",
                  f"campaign: {campaign or ''}",
                  "samples:"]
    lines.append(f"  # --- {datetime.date.today().isoformat()}: {generated_by()}")
    for sample_name, entry in entries.items():
        if sample_name in existing or entry["das"] in known_das:
            skipped.append(sample_name)
            continue
        lines.append(f"  {sample_name}:")
        lines.append(f"    das: {entry['das']}")
        lines.append(f"    process: {entry['process']}")
        if entry.get("isData"):
            lines.append("    isData: true")
        else:
            lines.append(f"    xsec: {entry['xsec']}")
        added.append(sample_name)
    if dry_run or not added:
        return added, skipped, "\n".join(lines)
    with open(path, "a") as f:
        f.write(("" if os.path.getsize(path) == 0 or open(path).read().endswith("\n") else "\n") + "\n".join(lines) + "\n")
    # placeholder cross-section keys, so that the normalisation fails loudly until they are filled
    xsec_path = os.path.join(SAMPLES_DIR, "xsec.yaml")
    table = load_xsec()
    missing = [entries[n]["xsec"] for n in added if not entries[n].get("isData") and entries[n]["xsec"] not in table]
    if missing and not output:
        with open(xsec_path, "a") as f:
            for key in missing:
                f.write(f"{key}: {{xsec: null, precision: \"\", source: \"FILL ME (XSDB / TOP PAG)\", note: \"added by hell samples create\"}}\n")
    return added, skipped, "\n".join(lines)
