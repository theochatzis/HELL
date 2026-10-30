"""Central corrections (correctionlib JSONs of the POGs) per era: samples/corrections.yaml.

    from hell.corrections import corrections
    c = corrections("2022")       # dict with resolved file paths and correction names, given to the modules
    hell corrections --era 2022   # print and check
"""
import os

import yaml

from hell import HELL_BASE, HellError, log, warn

CORRECTIONS = os.path.join(HELL_BASE, "samples", "corrections.yaml")


def load():
    if not os.path.exists(CORRECTIONS):
        raise HellError(f"{CORRECTIONS} not found")
    with open(CORRECTIONS) as f:
        return yaml.safe_load(f)


def eras():
    return sorted(load().get("eras", {}))


def corrections(era):
    """Resolved corrections of an era: {"files": {key: path}, <name>: value, "era": era}."""
    cfg = load()
    if str(era) not in cfg.get("eras", {}):
        raise HellError(f"era '{era}' is not in samples/corrections.yaml; known: {', '.join(eras())}")
    entry = dict(cfg["eras"][str(era)])
    base = cfg.get("base", "")
    files = {key: os.path.join(base, pattern.format(dir=entry["dir"])) for key, pattern in cfg.get("files", {}).items()}
    out = {"era": str(era), "files": files}
    out.update({k: v for k, v in entry.items() if k != "dir"})
    return out


def check(era):
    c = corrections(era)
    missing = [k for k, p in c["files"].items() if not os.path.exists(p)]
    return c, missing


def report(era):
    c, missing = check(era)
    lines = [f"[HELL] corrections of era {era} (samples/corrections.yaml)"]
    for key, path in c["files"].items():
        lines.append(f"  {key:14s} {path}" + ("   MISSING" if key in missing else ""))
    for key, value in c.items():
        if key in ("files", "era"):
            continue
        lines.append(f"  {key:14s} {value}")
    if missing:
        lines.append(f"[HELL] WARNING: {len(missing)} file(s) missing: is /cvmfs/cms.cern.ch mounted?")
    return "\n".join(lines)


def list_corrections(path, pattern=None):
    """Names and inputs of the corrections in a JSON (needs the correctionlib python package)."""
    try:
        import correctionlib
    except ImportError:
        raise HellError("the correctionlib python package is not available (use the LCG environment)")
    cset = correctionlib.CorrectionSet.from_file(path)
    out = []
    for name in cset:
        if pattern and pattern.lower() not in name.lower():
            continue
        out.append((name, [(i.name, i.type) for i in cset[name].inputs]))
    try:
        for name in cset.compound:
            if not pattern or pattern.lower() in name.lower():
                out.append((name + " (compound)", [(i.name, i.type) for i in cset.compound[name].inputs]))
    except Exception:
        pass
    return out
