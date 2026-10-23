"""The `hell` command. Every subcommand has --help with usage AND an example."""
import argparse
import glob
import json
import os
import subprocess
import sys

from hell import HELL_BASE, HellError, __version__, log, warn, error

EXAMPLES = {}


class Formatter(argparse.RawDescriptionHelpFormatter):
    pass


def sub(subparsers, name, help_, example, **kw):
    EXAMPLES[name] = example
    p = subparsers.add_parser(name, help=help_, description=help_, epilog="Example:\n" + example, formatter_class=Formatter, **kw)
    return p


def build_parser():
    p = argparse.ArgumentParser(
        prog="hell",
        formatter_class=Formatter,
        description="HELL - High Energy Likelihood Laboratory. We forge the analysis in HELL.",
        epilog="Example:\n  hell run configs/SingleTopMass/skim_cfg.py sample=TbarBQtoLNu_tch era=2022 maxEvents=1000\n"
               "  hell hist configs/SingleTopMass/hist.yaml\n  hell fit configs/SingleTopMass/fit.yaml --strategy 2\n"
               "  hell navigate skim.root\n  hell pray\n\nEvery subcommand has its own --help with an example.")
    p.add_argument("--version", action="version", version=f"HELL {__version__}")
    s = p.add_subparsers(dest="command", metavar="command")

    r = sub(s, "run", "Run a python configuration through hell-run (skim, analysis tree, quick histograms).",
            "  hell run configs/SingleTopMass/skim_cfg.py sample=TbarBQtoLNu_tch era=2022 maxEvents=5000 outputFile=test.root\n"
            "  hell run configs/SingleTopMass/analysis_cfg.py inputFiles=skim.root era=2022 variation=jesUp")
    r.add_argument("config", help="python configuration defining `process`")
    r.add_argument("options", nargs="*", help="key=value options of the config (see `hell run cfg.py help`)")
    r.add_argument("--workdir", default=None, help="where the JSON is written (default ./work)")
    r.add_argument("--dry-run", action="store_true", help="write the JSON and print the path, do not run")
    r.add_argument("--dump", action="store_true", help="hell-run --dump: resolved modules, parameters, products")
    r.add_argument("--tag", default=None, help="suffix of the JSON file name (parallel runs of one config)")

    r = sub(s, "skim", "Alias of `hell run` for skim configurations (same options).",
            "  hell skim configs/SingleTopMass/skim_cfg.py sample=Muon_2022C era=2022")
    r.add_argument("config"); r.add_argument("options", nargs="*"); r.add_argument("--workdir", default=None)
    r.add_argument("--dry-run", action="store_true"); r.add_argument("--dump", action="store_true"); r.add_argument("--tag", default=None)

    h = sub(s, "hist", "Histogram analysis trees with configurable cuts, variables, processes, colours, lumi and systematics.",
            "  hell hist configs/SingleTopMass/hist.yaml -o hists/2022.root\n  hell hist configs/SingleTopMass/hist.yaml --process tch --process tt -n 10000")
    h.add_argument("config", help="YAML histogram configuration")
    h.add_argument("-o", "--output", default=None)
    h.add_argument("--process", action="append", default=None, help="only these processes (repeatable)")
    h.add_argument("--lumi", type=float, default=None, help="override the luminosity (pb^-1)")
    h.add_argument("-n", "--max-events", type=int, default=None, help="at most N events per sample (tests)")
    h.add_argument("--dry-run", action="store_true")

    u = sub(s, "unleash-systematics", "Run (or submit) the analysis config once per object-level variation listed in the config/YAML.",
            "  hell unleash-systematics configs/SingleTopMass/analysis_cfg.py --variations jesUp jesDown jerUp jerDown "
            "--sample TbarBQtoLNu_tch --era 2022 --output-dir /eos/user/x/HELL/trees/2022 --tag trees_v1\n"
            "  hell unleash-systematics configs/SingleTopMass/analysis_cfg.py --local inputFiles=skim.root era=2022")
    u.add_argument("config"); u.add_argument("options", nargs="*", help="key=value config options (local mode)")
    u.add_argument("--variations", nargs="+", default=["jesUp", "jesDown", "jerUp", "jerDown"])
    u.add_argument("--local", action="store_true", help="run locally instead of submitting to condor")
    u.add_argument("--sample", nargs="+", default=[]); u.add_argument("--era", default="")
    u.add_argument("--output-dir", default=None); u.add_argument("--tag", default=None)
    u.add_argument("--files-per-job", type=int, default=10); u.add_argument("--dry-run", action="store_true")

    pl = sub(s, "plot", "Stacked data/MC plots from a hell-hist output.", "  hell plot hists/2022.root -o plots/2022 --logy")
    pl.add_argument("histfile"); pl.add_argument("-o", "--outdir", default="plots"); pl.add_argument("--region", action="append", default=None)
    pl.add_argument("--variable", action="append", default=None); pl.add_argument("--logy", action="store_true"); pl.add_argument("--format", default="png")
    pl.add_argument("--lumi-label", default="", help="right-hand text (default from the lumi of the file)")
    pl.add_argument("--extra", default="Preliminary", help="text under CMS: Preliminary, Work in progress, '' for none")

    f = sub(s, "fit", "Write Combine datacards (template or parametric) from histograms and run combine.",
            "  hell fit configs/SingleTopMass/fit.yaml --datacards           # only write cards\n"
            "  hell fit configs/SingleTopMass/fit.yaml --strategy 2          # cards + combine\n"
            "  hell fit configs/SingleTopMass/fit.yaml --parametric --strategy 1 -- --robustFit 1")
    f.add_argument("config"); f.add_argument("--strategy", type=int, default=1, help="Minuit strategy (0, 1, 2)")
    f.add_argument("--datacards", action="store_true", help="only write the datacards"); f.add_argument("--parametric", action="store_true")
    f.add_argument("--dry-run", action="store_true", help="print the combine command without running it")
    f.add_argument("--workdir", default="work"); f.add_argument("extra", nargs="*", help="extra combine arguments after --")

    t = sub(s, "train", "Train a PyTorch classifier/regressor on analysis trees and validate it (ROC, overtraining, residuals).",
            "  hell train configs/SingleTopMass/ml_classifier.yaml --epochs 20\n  hell train configs/SingleTopMass/ml_regressor.yaml --output-dir models")
    t.add_argument("config"); t.add_argument("--output-dir", default=None); t.add_argument("--epochs", type=int, default=None)
    t.add_argument("--max-events", type=int, default=None, help="per sample (quick tests)")

    sm = sub(s, "samples", "Sample registry: list eras and samples, query DAS, build file lists, show cross sections.",
             "  hell samples list --era 2022\n  hell samples files --era 2022 TbarBQtoLNu_tch      # cache the file list\n"
             "  hell samples das '/TbarBQtoLNu-t-channel*/Run3Summer22NanoAODv12*/NANOAODSIM'\n  hell samples xsec TbarBQtoLNu_tch")
    sm.add_argument("action", choices=["list", "files", "das", "xsec", "show"]); sm.add_argument("names", nargs="*")
    sm.add_argument("--era", default=None); sm.add_argument("--force", action="store_true", help="rebuild cached file lists")
    sm.add_argument("--instance", default=None, help="DBS instance for private datasets, e.g. prod/phys03")

    sb = sub(s, "submit", "Submit HELL jobs to HTCondor (or CRAB for skims over DAS datasets).",
             "  hell submit run configs/SingleTopMass/skim_cfg.py --sample TbarBQtoLNu_tch TTtoLNu2Q --era 2022 "
             "--files-per-job 10 --output-dir /eos/user/x/HELL/skims/2022 --tag skim_v1\n"
             "  hell submit hist configs/SingleTopMass/hist.yaml --output-dir /eos/user/x/HELL/hists --tag hist_v1\n"
             "  hell submit train configs/SingleTopMass/ml_classifier.yaml --output-dir /eos/user/x/HELL/models --tag train_v1\n"
             "  hell submit cmd jobs.txt --output-dir /eos/user/x/HELL/misc --tag misc_v1   # id | command | output per line\n"
             "  hell submit run configs/SingleTopMass/skim_cfg.py --backend crab --sample Muon_2022C --era 2022 --tag skim_v1 --output-dir /store/user/you/HELL/skims")
    sb.add_argument("kind", choices=["run", "hist", "train", "cmd"]); sb.add_argument("config")
    sb.add_argument("--backend", choices=["condor", "crab"], default="condor")
    sb.add_argument("--sample", nargs="+", default=[]); sb.add_argument("--era", default="")
    sb.add_argument("--files-per-job", type=int, default=10); sb.add_argument("--max-files", type=int, default=-1)
    sb.add_argument("--process", nargs="+", default=None, help="hist: processes (default all in the YAML)")
    sb.add_argument("--output-dir", required=True); sb.add_argument("--tag", required=True)
    sb.add_argument("--variation", default="nominal"); sb.add_argument("--opt", action="append", default=[], help="extra key=value options for the config")
    sb.add_argument("--flavour", default="workday"); sb.add_argument("--cpus", type=int, default=1); sb.add_argument("--memory", default="2000MB")
    sb.add_argument("--gpu", action="store_true", help="request a GPU (training)"); sb.add_argument("--retries", type=int, default=2)
    sb.add_argument("--proxy", action="store_true", help="ship the VOMS proxy (remote xrootd inputs)")
    sb.add_argument("--site", default="T3_CH_CERNBOX", help="CRAB storage site"); sb.add_argument("--dry-run", action="store_true")

    for name, help_, ex in (("check", "Check the outputs of a condor task; writes resubmit.txt.", "  hell check --tag skim_v1"),
                            ("resubmit", "Resubmit the failed jobs of a condor task.", "  hell resubmit --tag skim_v1"),
                            ("status", "condor_q of a task plus the output check.", "  hell status --tag skim_v1")):
        c = sub(s, name, help_, ex); c.add_argument("--tag", required=True); c.add_argument("--dry-run", action="store_true")

    m = sub(s, "merge", "Merge HELL files (hadd-like, with bookkeeping consistency check), per sample of a task or explicit files.",
            "  hell merge --tag skim_v1\n  hell merge -o merged/tch.root /eos/user/x/HELL/skims/2022/TbarBQtoLNu_tch/*.root")
    m.add_argument("inputs", nargs="*"); m.add_argument("-o", "--output", default=None); m.add_argument("--tag", default=None); m.add_argument("--force", action="store_true")

    b = sub(s, "bookkeep", "Harvest event counts, sums of weights, runs and lumi sections of produced files into a JSON.",
            "  hell bookkeep /eos/user/x/HELL/skims/2022 -o samples/bookkeeping/2022.json")
    b.add_argument("directory"); b.add_argument("-o", "--output", default=None); b.add_argument("--no-lumis", action="store_true")

    n = sub(s, "navigate", "Navigate ROOT files (trees, branches, bookkeeping, provenance), the code (modules) or the data model.",
            "  hell navigate skim.root                       # file content, HELL bookkeeping and provenance chain\n"
            "  hell navigate skim.root --branch 'Jet_*' --entries 3\n"
            "  hell navigate code                            # module types, where defined, parameters\n"
            "  hell navigate code --config configs/SingleTopMass/analysis_cfg.py inputFiles=skim.root [--html page.html]\n"
            "  hell navigate model                           # the HELL data model (hell::Jet, ...)")
    n.add_argument("target", help="ROOT file, 'code' or 'model'"); n.add_argument("options", nargs="*", help="config key=value (with --config)")
    n.add_argument("--branch", default="*", help="branch glob"); n.add_argument("--entries", type=int, default=0, help="print values of the first N entries")
    n.add_argument("--tree", default=None); n.add_argument("--no-provenance", action="store_true")
    n.add_argument("--config", default=None, help="code: python config to draw the module graph of"); n.add_argument("--html", default=None)
    n.add_argument("--grep", default=None, help="code: filter module types")

    va = sub(s, "validate", "Data/MC validation of object reconstruction and scale factors: plots and agreement metrics (yield ratio, chi2, KS, pulls).",
             "  hell validate configs/SingleTopMass/validation.yaml\n  hell validate configs/SingleTopMass/validation.yaml -o validation/2022_v2 --max-hists 10")
    va.add_argument("config", help="YAML validation configuration"); va.add_argument("-o", "--output", default=None)
    va.add_argument("--max-hists", type=int, default=None, help="only the first N histograms (tests)")

    lm = sub(s, "list-modules", "List the available C++ module types.", "  hell list-modules")
    lm.add_argument("--json", action="store_true")

    d = sub(s, "dump", "Print the JSON a python configuration produces.", "  hell dump configs/SingleTopMass/skim_cfg.py sample=TbarBQtoLNu_tch era=2022")
    d.add_argument("config"); d.add_argument("options", nargs="*")

    i = sub(s, "issue", "Open a new issue/progress note in docs/issues from the template.",
            "  hell issue new \"JES variation does not propagate to MET\" --area systematics\n  hell issue list")
    i.add_argument("action", choices=["new", "list"]); i.add_argument("title", nargs="?", default=None); i.add_argument("--area", default="general")

    sub(s, "pray", "Run the quick end-to-end test (skim -> analysis tree -> histograms -> datacards) and hope.", "  hell pray")
    sub(s, "version", "Print the version and build information.", "  hell version")
    return p


def cmd_run(a):
    from hell.runner import run_config
    extra = ["--dump"] if a.dump else []
    return run_config(a.config, a.options, a.workdir, a.dry_run, extra, a.tag)


def cmd_hist(a):
    from hell.histcfg import write
    from hell.runner import find_executable
    stem = os.path.splitext(os.path.basename(a.config))[0]
    tag = ("_" + "_".join(a.process)) if a.process else ""
    jp = write(a.config, os.path.join("work", f"hist_{stem}{tag}.json"), output=a.output, processes=a.process, lumi=a.lumi)
    cmd = [find_executable("hell-hist"), jp] + (["-n", str(a.max_events)] if a.max_events else [])
    log(" ".join(cmd))
    return 0 if a.dry_run else subprocess.call(cmd)


def cmd_unleash(a):
    log("Unleashing systematics. May the variations be merciful.")
    if a.local:
        from hell.runner import run_config
        rc = 0
        for v in ["nominal"] + a.variations:
            opts = [o for o in a.options if not o.startswith(("variation=", "outputFile="))]
            out = [o.split("=", 1)[1] for o in a.options if o.startswith("outputFile=")]
            out = (out[0] if out else "analysis.root").replace(".root", f"_{v}.root")
            rc |= run_config(a.config, opts + [f"variation={v}", f"outputFile={out}"], dry_run=a.dry_run, tag=v)
        return rc
    from hell.batch import condor
    if not (a.sample and a.era and a.output_dir and a.tag):
        raise HellError("condor mode needs --sample, --era, --output-dir and --tag (or use --local)")
    jobs = {}
    for v in ["nominal"] + a.variations:
        jobs.update(condor.jobs_for_run(a.config, a.sample, a.era, a.output_dir, a.files_per_job, options=a.options, variation=v))
    taskdir = condor.make_task(a.tag, jobs, {"kind": "run", "config": a.config, "output_dir": a.output_dir, "variations": a.variations})
    return condor.submit(taskdir, a.dry_run)


def cmd_plot(a):
    from hell.plot import plot
    plot(a.histfile, a.outdir, a.region, a.variable, a.format, a.logy, a.lumi_label, a.extra)
    return 0


def cmd_fit(a):
    from hell.fit import combine as C
    cfg = C.load(a.config)
    cards = C.write_datacards(cfg, parametric=a.parametric, workdir=a.workdir)
    if a.datacards:
        return 0
    card = cards[0] if len(cards) == 1 else C.combine_cards(cards, cfg.get("output", "cards/hell") + "_combined.txt", a.dry_run)
    return C.run_combine(card, cfg, a.strategy, a.extra, a.dry_run)


def cmd_train(a):
    from hell.ml.run import train_from_yaml
    train_from_yaml(a.config, a.output_dir, a.epochs, a.max_events)
    return 0


def cmd_samples(a):
    from hell import samples as S
    if a.action == "list":
        for era in ([a.era] if a.era else S.eras()):
            d = S.load_era(era)
            print(f"[HELL] era {era}: lumi {d.get('lumi')} pb^-1, {len(d['samples'])} samples")
            for n, s in d["samples"].items():
                kind = "data" if s.get("isData") else f"MC xsec={S.cross_section(s)} pb"
                cached = os.path.exists(S.filelist_path(era, n))
                print(f"  {n:40s} process={s.get('process'):10s} {kind:28s} {'files cached' if cached else ''}")
        return 0
    if a.action == "files":
        if not a.era:
            raise HellError("--era is required")
        d = S.load_era(a.era)
        for n in (a.names or d["samples"].keys()):
            S.scan(d["samples"][n], force=a.force)
        return 0
    if a.action == "das":
        for pat in a.names:
            for ds in S.das_query(pat, a.instance):
                print(f"  {S.suggest_name(ds):40s} {ds}")
        return 0
    if a.action in ("xsec", "show"):
        for n in a.names:
            s = S.find_sample(n, a.era)
            print(json.dumps(dict(s, xsec_pb=S.cross_section(s)), indent=1))
        return 0


def cmd_submit(a):
    from hell.batch import condor, crab
    opts = list(a.opt)
    if a.backend == "crab":
        if a.kind != "run":
            raise HellError("CRAB supports only `run` (skims over DAS datasets)")
        cfgs = crab.prepare(a.config, a.sample, a.era, a.tag, a.output_dir, a.files_per_job, a.site, options=opts, variation=a.variation)
        return crab.submit(cfgs, a.dry_run)
    if a.kind == "run":
        if not (a.sample and a.era):
            raise HellError("run: give --sample and --era")
        jobs = condor.jobs_for_run(a.config, a.sample, a.era, a.output_dir, a.files_per_job, max_files=a.max_files, options=opts, variation=a.variation)
    elif a.kind == "hist":
        import yaml
        with open(a.config) as f:
            procs = a.process or [p["name"] for p in yaml.safe_load(f)["processes"]]
        jobs = condor.jobs_for_hist(a.config, procs, a.output_dir, opts)
    elif a.kind == "train":
        jobs = condor.jobs_for_train(a.config, a.output_dir, opts)
    else:
        jobs = condor.jobs_from_commands(a.config, a.output_dir)
    extra = 'request_gpus = 1\n' if a.gpu else ""
    taskdir = condor.make_task(a.tag, jobs, {"kind": a.kind, "config": a.config, "output_dir": a.output_dir, "samples": a.sample, "era": a.era},
                               a.flavour, a.cpus, a.memory, a.retries, extra, a.proxy)
    return condor.submit(taskdir, a.dry_run)


def cmd_merge(a):
    from hell.batch import condor
    from hell.merge import merge
    if a.tag:
        condor.merge_task(a.tag, a.force)
        return 0
    if not a.inputs or not a.output:
        raise HellError("give --tag, or input files and -o output")
    files = []
    for i in a.inputs:
        files += glob.glob(i) or [i]
    merge(files, a.output, a.force)
    return 0


def cmd_bookkeep(a):
    from hell.bookkeeping import harvest
    harvest(a.directory, a.output, not a.no_lumis)
    return 0


def cmd_navigate(a):
    if a.target == "code":
        from hell.navigator import code
        if a.config:
            if a.html:
                code.to_html(a.config, a.options, a.html)
            print(code.report_graph(a.config, a.options))
        else:
            print(code.report_types(a.grep))
        return 0
    if a.target == "model":
        from hell.navigator import model
        print(model.report(a.options or None))
        return 0
    from hell.navigator import rootfile
    print(rootfile.report(a.target, a.branch, a.entries, not a.no_provenance, a.tree))
    return 0


def cmd_validate(a):
    from hell.validation import validate
    validate(a.config, a.output, a.max_hists)
    return 0


def cmd_list_modules(a):
    from hell.runner import find_executable
    return subprocess.call([find_executable("hell-run"), "--list-modules"] + (["--json"] if a.json else []))


def cmd_dump(a):
    from hell.config import load_config
    from hell.runner import apply_options
    p = load_config(a.config, a.options)
    print(p.to_json())
    return 0


def cmd_issue(a):
    root = os.path.join(HELL_BASE, "docs", "issues")
    if a.action == "list":
        for d in sorted(glob.glob(os.path.join(root, "HELL-*"))):
            readme = os.path.join(d, "README.md")
            title, status = "", ""
            if os.path.exists(readme):
                for line in open(readme):
                    if line.startswith("# "):
                        title = line[2:].strip()
                    if line.lower().startswith("**status**"):
                        status = line.split(":", 1)[-1].strip()
            print(f"  {os.path.basename(d):40s} {status:22s} {title}")
        return 0
    if not a.title:
        raise HellError("give a title: hell issue new \"...\"")
    nums = [int(os.path.basename(d).split("-")[1]) for d in glob.glob(os.path.join(root, "HELL-[0-9]*"))]
    num = max(nums, default=0) + 1
    slug = "".join(c if c.isalnum() else "_" for c in a.title.lower()).strip("_")[:50]
    d = os.path.join(root, f"HELL-{num:04d}-{slug}")
    os.makedirs(os.path.join(d, "plots"), exist_ok=True)
    import datetime
    with open(os.path.join(root, "TEMPLATE.md")) as f:
        text = f.read().replace("{title}", a.title).replace("{date}", datetime.date.today().isoformat()).replace("{area}", a.area).replace("{id}", f"HELL-{num:04d}")
    with open(os.path.join(d, "README.md"), "w") as f:
        f.write(text)
    log(f"issue {d}/README.md opened. Document the problem, the numbers and how to reproduce.")
    return 0


def cmd_pray(a):
    log("Praying... running test/run_quick_test.sh")
    rc = subprocess.call(["bash", os.path.join(HELL_BASE, "test", "run_quick_test.sh")])
    log("Your prayers were heard. All forged." if rc == 0 else "The forge went cold. Check the output above.", "INFO" if rc == 0 else "ERROR")
    return rc


def cmd_version(a):
    from hell.runner import find_executable
    print(f"HELL python {__version__}, base {HELL_BASE}")
    try:
        subprocess.call([find_executable("hell-run"), "--version"])
    except HellError as e:
        warn(str(e))
    return 0


COMMANDS = {"run": cmd_run, "skim": cmd_run, "hist": cmd_hist, "unleash-systematics": cmd_unleash, "plot": cmd_plot, "fit": cmd_fit,
            "train": cmd_train, "samples": cmd_samples, "submit": cmd_submit, "merge": cmd_merge, "bookkeep": cmd_bookkeep,
            "navigate": cmd_navigate, "validate": cmd_validate, "list-modules": cmd_list_modules, "dump": cmd_dump, "issue": cmd_issue, "pray": cmd_pray, "version": cmd_version}


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    parser = build_parser()
    if not argv:
        print("[HELL] Welcome to HELL.\n")
        parser.print_help()
        return 0
    # `hell run cfg.py help` -> config options help
    a = parser.parse_args(argv)
    if a.command in ("check", "resubmit", "status"):
        from hell.batch import condor
        fn = {"check": lambda: condor.check(a.tag) and 1 or 0, "resubmit": lambda: condor.resubmit(a.tag, a.dry_run), "status": lambda: condor.status(a.tag) and 1 or 0}[a.command]
        try:
            return fn()
        except HellError as e:
            error(str(e)); return 1
    try:
        return COMMANDS[a.command](a) or 0
    except HellError as e:
        error(str(e))
        return 1
    except KeyboardInterrupt:
        error("interrupted")
        return 130


if __name__ == "__main__":
    sys.exit(main())
