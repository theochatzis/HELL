"""Run a python configuration through hell-run (locally)."""
import os
import shutil
import subprocess

from hell import HELL_BASE, HellError, log
from hell.config import load_config, Process
from hell import samples as S


def apply_options(process, options):
    """Fill process input/output/job from VarParsing options and the sample registry."""
    files, sample = S.resolve_inputs(options.inputFiles, options.sample, options.maxFiles, options.firstFile, options.era or None)
    if files:
        process.input["files"] = files
    if sample:
        process.job["sample"] = sample["name"]
        process.job["isData"] = bool(sample.get("isData", False))
        process.job["era"] = sample.get("era", "")
        process.job["goldenJSON"] = sample.get("goldenJSON") or ""
        process.job["lumi"] = sample.get("lumi") or 0.0
        if not sample.get("isData"):
            process.job["crossSection"] = S.cross_section(sample)
    if options.isData:
        process.job["isData"] = True
    if options.era:
        process.job["era"] = options.era
    if options.crossSection > 0:
        process.job["crossSection"] = options.crossSection
    if options.goldenJSON:
        process.job["goldenJSON"] = "" if options.goldenJSON.lower() == "none" else options.goldenJSON
    if process.job.get("era"):   # central corrections of the era (samples/corrections.yaml), if listed
        from hell.corrections import corrections
        try:
            process.job["corrections"] = corrections(process.job["era"])
        except HellError:
            pass
    process.input["maxEvents"] = options.maxEvents
    process.input["skipEvents"] = options.skipEvents
    if options.outputFile:
        process.output["file"] = options.outputFile
    process.job["variation"] = options.variation
    process.options["logLevel"] = options.logLevel
    return process


def work_json_path(cfg_path, workdir=None, tag=None):
    stem = os.path.splitext(os.path.basename(cfg_path))[0]
    workdir = workdir or os.path.join(os.getcwd(), "work")
    os.makedirs(workdir, exist_ok=True)
    return os.path.join(workdir, f"{stem}{('_' + tag) if tag else ''}.json")


def find_executable(name):
    exe = shutil.which(name)
    if exe:
        return exe
    cand = os.path.join(os.environ.get("HELL_BUILD", os.path.join(HELL_BASE, "build")), "bin", name)
    if os.path.exists(cand):
        return cand
    raise HellError(f"{name} not found: build HELL and source env/setup.sh")


def run_config(cfg_path, cfg_args=(), workdir=None, dry_run=False, extra_args=(), tag=None):
    process = load_config(cfg_path, cfg_args)
    if not isinstance(process, Process):
        raise HellError(f"{cfg_path} does not define a hell.config.Process")
    if not process.input["files"]:
        raise HellError("no input files: give inputFiles=... or sample=...")
    json_path = work_json_path(cfg_path, workdir, tag)
    process.dump(json_path)
    log(f"configuration written to {json_path}")
    cmd = [find_executable("hell-run"), json_path] + list(extra_args)
    log(" ".join(cmd))
    if dry_run:
        print(process.summary())
        return 0
    return subprocess.call(cmd)
