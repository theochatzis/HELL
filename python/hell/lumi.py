"""Integrated luminosity with brilcalc, from the master certification list (samples/certification.yaml).

    hell lumi --era 2022                                  # Golden JSON of the era, run range of the era
    hell lumi --era 2022 --trigger HLT_IsoMu24 HLT_Ele30_WPTight_Gsf   # per trigger path (recorded lumi with the path)
    hell lumi --era 2022 --json samples/bookkeeping/2022_processed.json   # on the processed lumi sections (hell bookkeep),
                                                                          # intersected with the Golden JSON first
    hell lumi --era 2022 --update                         # write the result into samples/2022.yaml: lumi

brilcalc runs in the BRIL environment from cvmfs (brilws-docker). Results go to samples/lumi/<era>.json with
the command line, the JSON, the normtag and the per-trigger values, so that the number in the era file is
traceable.
"""
import json
import os
import re
import subprocess

import yaml

from hell import HELL_BASE, HellError, generated_by, log, warn

CERTIFICATION = os.path.join(HELL_BASE, "samples", "certification.yaml")


def load_certification():
    if not os.path.exists(CERTIFICATION):
        raise HellError(f"{CERTIFICATION} not found")
    with open(CERTIFICATION) as f:
        return yaml.safe_load(f)


def era_certification(era):
    cert = load_certification()
    eras = cert.get("eras", {})
    if str(era) not in eras:
        raise HellError(f"era '{era}' is not in samples/certification.yaml; known: {', '.join(eras)}")
    entry = dict(eras[str(era)])
    entry.setdefault("normtag", cert.get("normtag"))
    entry.setdefault("brilEnv", cert.get("brilEnv"))
    return entry


def golden_json(era):
    return era_certification(era).get("goldenJSON")


def _ranges_to_set(ranges):
    out = set()
    for first, last in ranges:
        out.update(range(int(first), int(last) + 1))
    return out


def _set_to_ranges(sections):
    ranges = []
    for ls in sorted(sections):
        if ranges and ls == ranges[-1][1] + 1:
            ranges[-1][1] = ls
        else:
            ranges.append([ls, ls])
    return ranges


def intersect_with_golden(processed_json, golden, output):
    """Keep only the certified lumi sections of a processed-lumi JSON (the LuminosityBlocks trees of the
    skims list every lumi section of the input files, also the uncertified ones whose events were rejected).
    Writes the intersection to `output` and returns (path, n_processed, n_kept)."""
    for path in (processed_json, golden):
        if not os.path.exists(path):
            raise HellError(f"lumi-section JSON not found: {path}")
    with open(processed_json) as f:
        processed = json.load(f)
    with open(golden) as f:
        certified = json.load(f)
    kept, n_processed, n_kept = {}, 0, 0
    for run, ranges in processed.items():
        sections = _ranges_to_set(ranges)
        n_processed += len(sections)
        good = sections & _ranges_to_set(certified.get(str(run), []))
        n_kept += len(good)
        if good:
            kept[str(run)] = _set_to_ranges(good)
    with open(output, "w") as f:
        json.dump(kept, f)
    return output, n_processed, n_kept


def brilcalc_command(era, lumi_json=None, trigger=None, unit="/pb"):
    entry = era_certification(era)
    cmd = ["brilcalc", "lumi", "-u", unit, "-i", lumi_json or entry["goldenJSON"]]
    if entry.get("normtag"):
        cmd += ["--normtag", entry["normtag"]]
    if entry.get("runRange"):
        cmd += ["--begin", str(entry["runRange"][0]), "--end", str(entry["runRange"][1])]
    if trigger:
        cmd += ["--hltpath", trigger if trigger.endswith("*") else trigger + "_v*"]
    return cmd, entry


def parse_brilcalc(output):
    """Recorded and delivered luminosity from the brilcalc summary table. With --hltpath the summary has one
    row per path version (HLT_IsoMu24_v13, _v14, ...): the rows are summed and the versions returned."""
    lines = output.splitlines()
    recorded = delivered = 0.0
    versions = []
    found = False
    for i, line in enumerate(lines):
        if not line.startswith("#Summary"):
            continue
        for row in lines[i + 1:]:
            cells = [c.strip() for c in row.strip().strip("|").split("|")]
            if len(cells) >= 2 and re.match(r"^[0-9.]+$", cells[-1]) and re.match(r"^[0-9.]+$", cells[-2]):
                recorded += float(cells[-1]); delivered += float(cells[-2]); found = True
                if cells[0].startswith("HLT_"):
                    versions.append(cells[0])
            elif found and row.startswith("+"):
                continue
        break
    if not found:
        raise HellError("could not parse the brilcalc summary:\n" + output[-2000:])
    return recorded, delivered, versions


def run_brilcalc(cmd, bril_env):
    # the BRIL environment defines brilcalc as a shell alias (singularity container): aliases need expanding,
    # and bash only expands an alias in a line parsed after its definition, hence the newlines.
    shell = "shopt -s expand_aliases\n" + (f"source {bril_env} >/dev/null 2>&1 || exit 1\n" if bril_env else "") + " ".join(cmd)
    log(" ".join(cmd))
    env = {k: v for k, v in os.environ.items() if k not in ("PYTHONHOME", "PYTHONPATH")}
    result = subprocess.run(shell, shell=True, executable="/bin/bash", capture_output=True, text=True, env=env)
    if result.returncode != 0:
        raise HellError(f"brilcalc failed:\n{(result.stderr or result.stdout)[-2000:]}")
    return result.stdout


def compute(era, lumi_json=None, triggers=(), unit="/pb", dry_run=False, update=False, apply_golden=True):
    results = {"generatedBy": generated_by(), "era": era, "unit": unit, "triggers": {}}
    if lumi_json and apply_golden:
        golden = golden_json(era)
        certified_json = os.path.splitext(lumi_json)[0] + "_certified.json"
        _, n_processed, n_kept = intersect_with_golden(lumi_json, golden, certified_json)
        log(f"{lumi_json}: {n_kept} of {n_processed} processed lumi sections are certified ({os.path.basename(golden)}) -> {certified_json}")
        results["processedJson"], results["certifiedSections"], results["processedSections"] = lumi_json, n_kept, n_processed
        lumi_json = certified_json
    cmd, entry = brilcalc_command(era, lumi_json, None, unit)
    results["json"] = lumi_json or entry["goldenJSON"]
    results["normtag"] = entry.get("normtag")
    results["runRange"] = entry.get("runRange")
    if dry_run:
        print(" ".join(cmd))
        for trigger in triggers:
            print(" ".join(brilcalc_command(era, lumi_json, trigger, unit)[0]))
        return results
    recorded, delivered, _ = parse_brilcalc(run_brilcalc(cmd, entry.get("brilEnv")))
    results["recorded"], results["delivered"] = recorded, delivered
    log(f"{era}: recorded {recorded:.3f} {unit}, delivered {delivered:.3f} {unit}")
    for trigger in triggers:
        tcmd, _ = brilcalc_command(era, lumi_json, trigger, unit)
        rec, deliv, versions = parse_brilcalc(run_brilcalc(tcmd, entry.get("brilEnv")))
        results["triggers"][trigger] = {"recorded": rec, "delivered": deliv, "versions": versions}
        log(f"{era} with {trigger}: recorded {rec:.3f} {unit} ({len(versions)} path versions)")
    out_dir = os.path.join(HELL_BASE, "samples", "lumi")
    os.makedirs(out_dir, exist_ok=True)
    out = os.path.join(out_dir, f"{era}{'_' + os.path.splitext(os.path.basename(lumi_json))[0] if lumi_json else ''}.json")
    with open(out, "w") as f:
        json.dump(results, f, indent=1)
    log(f"written to {out}")
    if update:
        update_era_lumi(era, recorded if unit == "/pb" else recorded * (1000.0 if unit == "/fb" else 1.0), out)
    return results


def update_era_lumi(era, lumi_pb, source):
    """Replace the `lumi:` line of samples/<era>.yaml, keeping the rest of the file untouched."""
    path = os.path.join(HELL_BASE, "samples", f"{era}.yaml")
    if not os.path.exists(path):
        raise HellError(f"{path} does not exist (hell samples create --era {era} first)")
    text = open(path).read()
    new_line = f"lumi: {lumi_pb:.3f}   # pb^-1, brilcalc, see {os.path.relpath(source, HELL_BASE)}"
    if re.search(r"^lumi:.*$", text, re.M):
        text = re.sub(r"^lumi:.*$", new_line, text, count=1, flags=re.M)
    else:
        text = text.replace("samples:", new_line + "\nsamples:", 1)
    with open(path, "w") as f:
        f.write(text)
    log(f"{path}: {new_line}")
