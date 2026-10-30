"""NanoAOD versions (samples/nanoaod.yaml): settings per version and recognition from a file.

    from hell.nanoaod import settings, version_from_dataset, detect_version
    nano = settings(12)                           # dict: jetIdSource, pfMET, puppiMET, electronEtaSC, btagger, ...
    version_from_dataset("/X/Run3Summer22NanoAODv12-.../NANOAODSIM")   -> "12"
    detect_version("file.root")                   -> "12" from the branch fingerprint (None if unknown)
"""
import os
import re

import yaml

from hell import HELL_BASE, HellError

NANOAOD = os.path.join(HELL_BASE, "samples", "nanoaod.yaml")


def load():
    if not os.path.exists(NANOAOD):
        raise HellError(f"{NANOAOD} not found")
    with open(NANOAOD) as f:
        return yaml.safe_load(f)


def versions():
    return sorted(load().get("versions", {}), key=lambda v: int(v))


def settings(version):
    cfg = load()
    v = str(version)
    if v not in cfg.get("versions", {}):
        raise HellError(f"NanoAOD version '{v}' is not in samples/nanoaod.yaml; known: {', '.join(versions())}")
    out = dict(cfg.get("defaults", {}))
    out.update({k: val for k, val in cfg["versions"][v].items() if k != "fingerprint"})
    out["version"] = v
    out["fingerprint"] = cfg["versions"][v].get("fingerprint", {})
    return out


def version_from_dataset(dataset):
    """'12' from '/.../Run3Summer22NanoAODv12-.../NANOAODSIM', None if the name does not say."""
    m = re.search(r"NanoAODv(\d+)", dataset or "")
    return m.group(1) if m else None


def detect_version(path):
    """Version whose fingerprint matches the Events tree of the file (None if none or several match)."""
    from hell.rootutils import quiet_root
    ROOT = quiet_root()
    f = ROOT.TFile.Open(path)
    if not f or f.IsZombie():
        raise HellError(f"cannot open {path}")
    tree = f.Get("Events")
    names = {b.GetName() for b in tree.GetListOfBranches()} if tree else set()
    f.Close()
    matches = []
    for v, entry in load().get("versions", {}).items():
        fp = entry.get("fingerprint", {})
        if all(b in names for b in fp.get("present", [])) and not any(b in names for b in fp.get("absent", [])):
            matches.append(v)
    return matches[0] if len(matches) == 1 else None
