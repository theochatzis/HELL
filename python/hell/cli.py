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
    parser = subparsers.add_parser(name, help=help_, description=help_, epilog="Example:\n" + example, formatter_class=Formatter, **kw)
    return parser


def build_parser():
    parser = argparse.ArgumentParser(
        prog="hell",
        formatter_class=Formatter,
        description="HELL - High Energy Likelihood Laboratory. We forge the analysis in HELL.",
        epilog="Example:\n  hell run configs/SingleTopMass/skim_cfg.py sample=TbarBQtoLNu_tch era=2022 maxEvents=1000\n"
               "  hell hist configs/SingleTopMass/hist.yaml\n  hell fit configs/SingleTopMass/fit.yaml --strategy 2\n"
               "  hell navigate skim.root\n  hell pray\n\nEvery subcommand has its own --help with an example.")
    parser.add_argument("--version", action="version", version=f"HELL {__version__}")
    subparsers = parser.add_subparsers(dest="command", metavar="command")

    run_command = sub(subparsers, "run", "Run a python configuration through hell-run (skim, analysis tree, quick histograms).",
            "  hell run configs/SingleTopMass/skim_cfg.py sample=TbarBQtoLNu_tch era=2022 maxEvents=5000 outputFile=test.root\n"
            "  hell run configs/SingleTopMass/analysis_cfg.py inputFiles=skim.root era=2022 variation=jesUp")
    run_command.add_argument("config", help="python configuration defining `process`")
    run_command.add_argument("options", nargs="*", help="key=value options of the config (see `hell run cfg.py help`)")
    run_command.add_argument("--workdir", default=None, help="where the JSON is written (default ./work)")
    run_command.add_argument("--dry-run", action="store_true", help="write the JSON and print the path, do not run")
    run_command.add_argument("--dump", action="store_true", help="hell-run --dump: resolved modules, parameters, products")
    run_command.add_argument("--tag", default=None, help="suffix of the JSON file name (parallel runs of one config)")

    skim_command = sub(subparsers, "skim", "Alias of `hell run` for skim configurations (same options).",
            "  hell skim configs/SingleTopMass/skim_cfg.py sample=Muon_2022C era=2022")
    skim_command.add_argument("config"); skim_command.add_argument("options", nargs="*"); skim_command.add_argument("--workdir", default=None)
    skim_command.add_argument("--dry-run", action="store_true"); skim_command.add_argument("--dump", action="store_true"); skim_command.add_argument("--tag", default=None)

    hist_command = sub(subparsers, "hist", "Histogram analysis trees with configurable cuts, variables, processes, colours, lumi and systematics.",
            "  hell hist configs/SingleTopMass/hist.yaml -o hists/2022.root\n  hell hist configs/SingleTopMass/hist.yaml --process tch --process tt -n 10000")
    hist_command.add_argument("config", help="YAML histogram configuration")
    hist_command.add_argument("-o", "--output", default=None)
    hist_command.add_argument("--process", action="append", default=None, help="only these processes (repeatable)")
    hist_command.add_argument("--lumi", type=float, default=None, help="override the luminosity (pb^-1)")
    hist_command.add_argument("-n", "--max-events", type=int, default=None, help="at most N events per sample (tests)")
    hist_command.add_argument("--dry-run", action="store_true")

    unleash_systematics_command = sub(subparsers, "unleash-systematics", "Run (or submit) the analysis config once per object-level variation listed in the config/YAML.",
            "  hell unleash-systematics configs/SingleTopMass/analysis_cfg.py --variations jesUp jesDown jerUp jerDown "
            "--sample TbarBQtoLNu_tch --era 2022 --output-dir /eos/user/x/HELL/trees/2022 --tag trees_v1\n"
            "  hell unleash-systematics configs/SingleTopMass/analysis_cfg.py --local inputFiles=skim.root era=2022")
    unleash_systematics_command.add_argument("config"); unleash_systematics_command.add_argument("options", nargs="*", help="key=value config options (local mode)")
    unleash_systematics_command.add_argument("--variations", nargs="+", default=["jesRegrouped_TotalUp", "jesRegrouped_TotalDown", "jerUp", "jerDown"])
    unleash_systematics_command.add_argument("--local", action="store_true", help="run locally instead of submitting to condor")
    unleash_systematics_command.add_argument("--sample", nargs="+", default=[]); unleash_systematics_command.add_argument("--era", default="")
    unleash_systematics_command.add_argument("--output-dir", default=None); unleash_systematics_command.add_argument("--tag", default=None)
    unleash_systematics_command.add_argument("--files-per-job", type=int, default=10); unleash_systematics_command.add_argument("--dry-run", action="store_true")

    plot_command = sub(subparsers, "plot", "Stacked data/MC plots from a hell-hist output.", "  hell plot hists/2022.root -o plots/2022 --logy")
    plot_command.add_argument("histfile"); plot_command.add_argument("-o", "--outdir", default="plots"); plot_command.add_argument("--region", action="append", default=None)
    plot_command.add_argument("--variable", action="append", default=None); plot_command.add_argument("--logy", action="store_true"); plot_command.add_argument("--format", default="png")
    plot_command.add_argument("--lumi-label", default="", help="right-hand text (default from the lumi of the file)")
    plot_command.add_argument("--extra", default="Preliminary", help="text under CMS: Preliminary, Work in progress, '' for none")

    fit_command = sub(subparsers, "fit", "Write Combine datacards (template or parametric) from histograms and run combine.",
            "  hell fit configs/SingleTopMass/fit.yaml --datacards           # only write cards\n"
            "  hell fit configs/SingleTopMass/fit.yaml --strategy 2          # cards + combine\n"
            "  hell fit configs/SingleTopMass/fit.yaml --parametric --strategy 1 -- --robustFit 1")
    fit_command.add_argument("config"); fit_command.add_argument("--strategy", type=int, default=1, help="Minuit strategy (0, 1, 2)")
    fit_command.add_argument("--datacards", action="store_true", help="only write the datacards"); fit_command.add_argument("--parametric", action="store_true")
    fit_command.add_argument("--dry-run", action="store_true", help="print the combine command without running it")
    fit_command.add_argument("--workdir", default="work"); fit_command.add_argument("extra", nargs="*", help="extra combine arguments after --")

    train_command = sub(subparsers, "train", "Train a PyTorch classifier/regressor on analysis trees and validate it (ROC, overtraining, residuals).",
            "  hell train configs/SingleTopMass/ml_classifier.yaml --epochs 20\n  hell train configs/SingleTopMass/ml_regressor.yaml --output-dir models")
    train_command.add_argument("config"); train_command.add_argument("--output-dir", default=None); train_command.add_argument("--epochs", type=int, default=None)
    train_command.add_argument("--max-events", type=int, default=None, help="per sample (quick tests)")

    samples_command = sub(subparsers, "samples", "Sample registry: create era files from DAS, list eras and samples, build file lists, show cross sections.",
             "  hell samples create --era 2022 --lumi 7980.4 --golden-json /eos/user/c/cmsdqm/www/CAF/certification/Collisions22/Cert_Collisions2022_355100_362760_Golden.json \\\n"
             "      '/TbarBQtoLNu-t-channel-4FS_TuneCP5_13p6TeV_powheg-madspin-pythia8/Run3Summer22NanoAODv12-130X_mcRun3_2022_realistic_v5*/NANOAODSIM' --process tch\n"
             "  hell samples create --era 2022 '/Muon/Run2022[CD]-22Sep2023-v1/NANOAOD'      # data: isData, process data\n"
             "  hell samples create --era 2022 --from-file datasets.txt --dry-run\n"
             "  hell samples list --era 2022\n  hell samples files --era 2022 TbarBQtoLNu_tch      # cache the file list\n"
             "  hell samples das '/TbarBQtoLNu-t-channel*/Run3Summer22NanoAODv12*/NANOAODSIM'\n  hell samples xsec TbarBQtoLNu_tch")
    samples_command.add_argument("action", choices=["list", "files", "das", "xsec", "show", "create"]); samples_command.add_argument("names", nargs="*", help="sample names, or DAS patterns for das/create")
    samples_command.add_argument("--lumi", type=float, default=None, help="create: luminosity in pb^-1 of a new era file")
    samples_command.add_argument("--golden-json", default=None, help="create: certification JSON of a new era file")
    samples_command.add_argument("--campaign", default=None, help="create: MC campaign note of a new era file")
    samples_command.add_argument("--process", default=None, help="create: process name for all created entries")
    samples_command.add_argument("--xsec", default=None, help="create: cross-section key (or number in pb) for all created MC entries")
    samples_command.add_argument("--name", default=None, help="create: sample name (single dataset only)")
    samples_command.add_argument("--from-file", default=None, help="create: read dataset names from a text file instead of DAS")
    samples_command.add_argument("--output", default=None, help="create: write to this file instead of samples/<era>.yaml")
    samples_command.add_argument("--dry-run", action="store_true", help="create: print the entries, write nothing")
    samples_command.add_argument("--era", default=None); samples_command.add_argument("--force", action="store_true", help="rebuild cached file lists")
    samples_command.add_argument("--instance", default=None, help="DBS instance for private datasets, e.g. prod/phys03")

    submit_command = sub(subparsers, "submit", "Submit HELL jobs to HTCondor (or CRAB for skims over DAS datasets).",
             "  hell submit run configs/SingleTopMass/skim_cfg.py --sample TbarBQtoLNu_tch TTtoLNu2Q --era 2022 "
             "--files-per-job 10 --output-dir /eos/user/x/HELL/skims/2022 --tag skim_v1\n"
             "  hell submit hist configs/SingleTopMass/hist.yaml --output-dir /eos/user/x/HELL/hists --tag hist_v1\n"
             "  hell submit train configs/SingleTopMass/ml_classifier.yaml --output-dir /eos/user/x/HELL/models --tag train_v1\n"
             "  hell submit cmd jobs.txt --output-dir /eos/user/x/HELL/misc --tag misc_v1   # id | command | output per line\n"
             "  hell submit run configs/SingleTopMass/skim_cfg.py --backend crab --sample Muon_2022C --era 2022 --tag skim_v1 --output-dir /store/user/you/HELL/skims")
    submit_command.add_argument("kind", choices=["run", "hist", "train", "cmd"]); submit_command.add_argument("config")
    submit_command.add_argument("--backend", choices=["condor", "crab"], default="condor")
    submit_command.add_argument("--sample", nargs="+", default=[]); submit_command.add_argument("--era", default="")
    submit_command.add_argument("--files-per-job", type=int, default=10); submit_command.add_argument("--max-files", type=int, default=-1)
    submit_command.add_argument("--process", nargs="+", default=None, help="hist: processes (default all in the YAML)")
    submit_command.add_argument("--output-dir", required=True); submit_command.add_argument("--tag", required=True)
    submit_command.add_argument("--variation", default="nominal"); submit_command.add_argument("--opt", action="append", default=[], help="extra key=value options for the config")
    submit_command.add_argument("--flavour", default="workday"); submit_command.add_argument("--cpus", type=int, default=1); submit_command.add_argument("--memory", default="2000MB")
    submit_command.add_argument("--gpu", action="store_true", help="request a GPU (training)"); submit_command.add_argument("--retries", type=int, default=2)
    submit_command.add_argument("--proxy", action="store_true", help="ship the VOMS proxy (remote xrootd inputs)")
    submit_command.add_argument("--site", default="T3_CH_CERNBOX", help="CRAB storage site"); submit_command.add_argument("--dry-run", action="store_true")

    for name, help_, example in (("check", "Check the outputs of a condor task; writes resubmit.txt.", "  hell check --tag skim_v1"),
                            ("resubmit", "Resubmit the failed jobs of a condor task.", "  hell resubmit --tag skim_v1"),
                            ("status", "condor_q of a task plus the output check.", "  hell status --tag skim_v1")):
        task_command = sub(subparsers, name, help_, example)
        task_command.add_argument("--tag", required=True)
        task_command.add_argument("--dry-run", action="store_true")

    merge_command = sub(subparsers, "merge", "Merge HELL files (hadd-like, with bookkeeping consistency check), per sample of a task or explicit files.",
            "  hell merge --tag skim_v1\n  hell merge -o merged/tch.root /eos/user/x/HELL/skims/2022/TbarBQtoLNu_tch/*.root")
    merge_command.add_argument("inputs", nargs="*"); merge_command.add_argument("-o", "--output", default=None); merge_command.add_argument("--tag", default=None); merge_command.add_argument("--force", action="store_true")

    bookkeep_command = sub(subparsers, "bookkeep", "Harvest event counts, sums of weights, runs and lumi sections of produced files into a JSON.",
            "  hell bookkeep /eos/user/x/HELL/skims/2022 -o samples/bookkeeping/2022.json")
    bookkeep_command.add_argument("directory"); bookkeep_command.add_argument("-o", "--output", default=None); bookkeep_command.add_argument("--no-lumis", action="store_true")

    navigate_command = sub(subparsers, "navigate", "Navigate ROOT files (trees, branches, bookkeeping, provenance), the code (modules) or the data model.",
            "  hell navigate skim.root                       # file content, HELL bookkeeping and provenance chain\n"
            "  hell navigate skim.root --branch 'Jet_*' --entries 3\n"
            "  hell navigate code                            # module types, where defined, parameters\n"
            "  hell navigate code --config configs/SingleTopMass/analysis_cfg.py inputFiles=skim.root [--html page.html]\n"
            "  hell navigate model                           # the HELL data model (hell::Jet, ...)")
    navigate_command.add_argument("target", help="ROOT file, 'code' or 'model'"); navigate_command.add_argument("options", nargs="*", help="config key=value (with --config)")
    navigate_command.add_argument("--branch", default="*", help="branch glob"); navigate_command.add_argument("--entries", type=int, default=0, help="print values of the first N entries")
    navigate_command.add_argument("--tree", default=None); navigate_command.add_argument("--no-provenance", action="store_true")
    navigate_command.add_argument("--config", default=None, help="code: python config to draw the module graph of"); navigate_command.add_argument("--html", default=None)
    navigate_command.add_argument("--grep", default=None, help="code: filter module types")

    validate_command = sub(subparsers, "validate", "Data/MC validation of object reconstruction and scale factors: plots and agreement metrics (yield ratio, chi2, KS, pulls).",
             "  hell validate configs/SingleTopMass/validation.yaml\n  hell validate configs/SingleTopMass/validation.yaml -o validation/2022_v2 --max-hists 10")
    validate_command.add_argument("config", help="YAML validation configuration"); validate_command.add_argument("-o", "--output", default=None)
    validate_command.add_argument("--max-hists", type=int, default=None, help="only the first N histograms (tests)")

    build_command = sub(subparsers, "build", "Configure (cmake) and compile the C++ part. Reconfigures automatically after a commit so the provenance hash is current.",
             "  hell build                 # incremental\n  hell build --clean -j 16   # from scratch\n  hell build --type Debug --verbose\n  hell build --target HellCore")
    build_command.add_argument("-j", "--jobs", type=int, default=None); build_command.add_argument("--clean", action="store_true", help="remove the build directory first")
    build_command.add_argument("--reconfigure", action="store_true", help="re-run cmake"); build_command.add_argument("--type", default=None, help="CMake build type (RelWithDebInfo, Debug, Release)")
    build_command.add_argument("--target", default=None, help="make target"); build_command.add_argument("--verbose", action="store_true", help="full cmake/make output")

    env_command = sub(subparsers, "env", "Report the environment (ROOT, python, torch, build state, git HEAD vs built version). The environment is loaded with `source env/setup.sh`.",
             "  source env/setup.sh        # load (a command cannot change your shell)\n  hell env                   # what is loaded and whether the build is current")

    corrections_command = sub(subparsers, "corrections", "Central POG corrections of an era (samples/corrections.yaml): resolved files and names, existence check, content of a JSON.",
                              "  hell corrections --era 2022\n  hell corrections --era 2022 --list btag          # corrections in the b-tag JSON with their inputs\n  hell corrections --era 2022 --list jerc --grep Regrouped")
    corrections_command.add_argument("--era", required=True); corrections_command.add_argument("--list", default=None, help="file key to list (btag, jerc, jetId, jetVetoMap, muon, electron, electronHlt, pileup)")
    corrections_command.add_argument("--grep", default=None)

    lumi_command = sub(subparsers, "lumi", "Integrated luminosity with brilcalc from the master certification list (samples/certification.yaml), per trigger if asked; can update the era file.",
                       "  hell lumi --era 2022\n  hell lumi --era 2022 --trigger HLT_IsoMu24 HLT_Ele30_WPTight_Gsf\n"
                       "  hell lumi --era 2022 --json samples/bookkeeping/2022_processed.json    # processed lumi sections from hell bookkeep\n"
                       "  hell lumi --era 2022 --update                                          # write lumi: into samples/2022.yaml\n  hell lumi --era 2022 --dry-run")
    lumi_command.add_argument("--era", required=True); lumi_command.add_argument("--json", default=None, help="lumi-section JSON instead of the Golden JSON")
    lumi_command.add_argument("--trigger", nargs="*", default=[], help="HLT paths (recorded lumi with the path active)")
    lumi_command.add_argument("--unit", default="/pb"); lumi_command.add_argument("--update", action="store_true", help="write the recorded lumi into samples/<era>.yaml")
    lumi_command.add_argument("--dry-run", action="store_true", help="print the brilcalc commands only")
    lumi_command.add_argument("--no-golden", action="store_true", help="with --json: do not intersect with the Golden JSON of the era")

    list_modules_command = sub(subparsers, "list-modules", "List the available C++ module types.", "  hell list-modules")
    list_modules_command.add_argument("--json", action="store_true")

    dump_command = sub(subparsers, "dump", "Print the JSON a python configuration produces.", "  hell dump configs/SingleTopMass/skim_cfg.py sample=TbarBQtoLNu_tch era=2022")
    dump_command.add_argument("config"); dump_command.add_argument("options", nargs="*")

    issue_command = sub(subparsers, "issue", "Open a new issue/progress note in docs/issues from the template.",
            "  hell issue new \"JES variation does not propagate to MET\" --area systematics\n  hell issue list")
    issue_command.add_argument("action", choices=["new", "list"]); issue_command.add_argument("title", nargs="?", default=None); issue_command.add_argument("--area", default="general")

    pray_command = sub(subparsers, "pray", "Run the quick end-to-end test (test/run_quick_test.sh) and hope. Steps can be selected; later steps use the outputs of earlier ones in test/out.",
             "  hell pray                  # everything, from a clean test/out\n  hell pray skim             # only the skims\n  hell pray hist validate    # histograms and validation on the existing trees\n  hell pray --list")
    pray_command.add_argument("steps", nargs="*", choices=["skim", "trees", "bookkeep", "hist", "validate", "fit", "ml", "navigate"], help="steps to run (default all)")
    pray_command.add_argument("--list", action="store_true", help="list the steps")
    sub(subparsers, "version", "Print the version and build information.", "  hell version")
    return parser


def cmd_run(args):
    from hell.runner import run_config
    extra = ["--dump"] if args.dump else []
    return run_config(args.config, args.options, args.workdir, args.dry_run, extra, args.tag)


def cmd_hist(args):
    from hell.histcfg import write
    from hell.runner import find_executable
    stem = os.path.splitext(os.path.basename(args.config))[0]
    tag = ("_" + "_".join(args.process)) if args.process else ""
    jp = write(args.config, os.path.join("work", f"hist_{stem}{tag}.json"), output=args.output, processes=args.process, lumi=args.lumi)
    cmd = [find_executable("hell-hist"), jp] + (["-n", str(args.max_events)] if args.max_events else [])
    log(" ".join(cmd))
    return 0 if args.dry_run else subprocess.call(cmd)


def cmd_unleash(args):
    log("Unleashing systematics. May the variations be merciful.")
    if args.local:
        from hell.runner import run_config
        rc = 0
        for v in ["nominal"] + args.variations:
            opts = [o for o in args.options if not o.startswith(("variation=", "outputFile="))]
            out = [o.split("=", 1)[1] for o in args.options if o.startswith("outputFile=")]
            out = (out[0] if out else "analysis.root").replace(".root", f"_{v}.root")
            rc |= run_config(args.config, opts + [f"variation={v}", f"outputFile={out}"], dry_run=args.dry_run, tag=v)
        return rc
    from hell.batch import condor
    if not (args.sample and args.era and args.output_dir and args.tag):
        raise HellError("condor mode needs --sample, --era, --output-dir and --tag (or use --local)")
    jobs, extra_files = {}, []
    for v in ["nominal"] + args.variations:
        new_jobs, files = condor.jobs_for_run(args.config, args.sample, args.era, args.output_dir, args.files_per_job, options=args.options, variation=v)
        jobs.update(new_jobs)
        extra_files += [f for f in files if f not in extra_files]
    taskdir = condor.make_task(args.tag, jobs, {"kind": "run", "config": args.config, "output_dir": args.output_dir, "variations": args.variations}, extra_files=extra_files)
    return condor.submit(taskdir, args.dry_run)


def cmd_plot(args):
    from hell.plot import plot
    plot(args.histfile, args.outdir, args.region, args.variable, args.format, args.logy, args.lumi_label, args.extra)
    return 0


def cmd_fit(args):
    from hell.fit import combine as C
    cfg = C.load(args.config)
    cards = C.write_datacards(cfg, parametric=args.parametric, workdir=args.workdir)
    if args.datacards:
        return 0
    card = cards[0] if len(cards) == 1 else C.combine_cards(cards, cfg.get("output", "cards/hell") + "_combined.txt", args.dry_run)
    return C.run_combine(card, cfg, args.strategy, args.extra, args.dry_run)


def cmd_train(args):
    from hell.ml.run import train_from_yaml
    train_from_yaml(args.config, args.output_dir, args.epochs, args.max_events)
    return 0


def cmd_samples(args):
    from hell import samples as S
    if args.action == "list":
        for era in ([args.era] if args.era else S.eras()):
            era_data = S.load_era(era)
            print(f"[HELL] era {era}: lumi {era_data.get('lumi')} pb^-1, {len(era_data['samples'])} samples")
            for sample_name, sample in era_data["samples"].items():
                kind = "data" if sample.get("isData") else f"MC xsec={S.cross_section(sample)} pb"
                cached = os.path.exists(S.filelist_path(era, sample_name))
                print(f"  {sample_name:40s} process={sample.get('process'):10s} nano=v{sample.get('nanoVersion', '?'):3s} {kind:28s} {'files cached' if cached else ''}")
        return 0
    if args.action == "files":
        if not args.era:
            raise HellError("--era is required")
        era_data = S.load_era(args.era)
        for sample_name in (args.names or era_data["samples"].keys()):
            S.scan(era_data["samples"][sample_name], force=args.force)
        return 0
    if args.action == "das":
        for pattern in args.names:
            for dataset in S.das_query(pattern, args.instance):
                print(f"  {S.suggest_name(dataset):40s} {dataset}")
        return 0
    if args.action == "create":
        if not args.era:
            raise HellError("--era is required")
        datasets = []
        if args.from_file:
            with open(args.from_file) as dataset_file:
                datasets += [line.strip() for line in dataset_file if line.strip() and not line.startswith("#")]
        for pattern in args.names:
            datasets += S.das_query(pattern, args.instance)
        if not datasets:
            raise HellError("no datasets: give DAS patterns or --from-file")
        xsec = float(args.xsec) if args.xsec and args.xsec.replace(".", "", 1).isdigit() else args.xsec
        entries = S.create_entries(datasets, args.process, xsec, args.name)
        golden = args.golden_json
        if golden is None:
            from hell.lumi import golden_json
            try:
                golden = golden_json(args.era)
            except HellError:
                golden = None
        added, skipped, text = S.write_era(args.era, entries, args.lumi, golden, args.campaign, args.output, args.dry_run)
        print(text)
        if skipped:
            warn(f"already present, skipped: {', '.join(skipped)}")
        if args.dry_run:
            log(f"dry run: {len(added)} entries would be added to samples/{args.era}.yaml")
        else:
            log(f"{len(added)} entries added to {args.output or f'samples/{args.era}.yaml'}; fill the cross sections in samples/xsec.yaml and verify lumi/goldenJSON")
        return 0
    if args.action in ("xsec", "show"):
        for sample_name in args.names:
            sample = S.find_sample(sample_name, args.era)
            print(json.dumps(dict(sample, xsec_pb=S.cross_section(sample)), indent=1))
        return 0


def cmd_submit(args):
    from hell.batch import condor, crab
    opts = list(args.opt)
    if args.backend == "crab":
        if args.kind != "run":
            raise HellError("CRAB supports only `run` (skims over DAS datasets)")
        cfgs = crab.prepare(args.config, args.sample, args.era, args.tag, args.output_dir, args.files_per_job, args.site, options=opts, variation=args.variation)
        return crab.submit(cfgs, args.dry_run)
    extra_files = []
    if args.kind == "run":
        if not (args.sample and args.era):
            raise HellError("run: give --sample and --era")
        jobs, extra_files = condor.jobs_for_run(args.config, args.sample, args.era, args.output_dir, args.files_per_job, max_files=args.max_files, options=opts, variation=args.variation)
    elif args.kind == "hist":
        import yaml
        with open(args.config) as config_file:
            procs = args.process or [process["name"] for process in yaml.safe_load(config_file)["processes"]]
        jobs = condor.jobs_for_hist(args.config, procs, args.output_dir, opts)
    elif args.kind == "train":
        jobs = condor.jobs_for_train(args.config, args.output_dir, opts)
    else:
        jobs = condor.jobs_from_commands(args.config, args.output_dir)
    extra = 'request_gpus = 1\n' if args.gpu else ""
    taskdir = condor.make_task(args.tag, jobs, {"kind": args.kind, "config": args.config, "output_dir": args.output_dir, "samples": args.sample, "era": args.era},
                               args.flavour, args.cpus, args.memory, args.retries, extra, args.proxy, extra_files=extra_files)
    return condor.submit(taskdir, args.dry_run)


def cmd_merge(args):
    from hell.batch import condor
    from hell.merge import merge
    if args.tag:
        condor.merge_task(args.tag, args.force)
        return 0
    if not args.inputs or not args.output:
        raise HellError("give --tag, or input files and -o output")
    files = []
    for pattern in args.inputs:
        files += glob.glob(pattern) or [pattern]
    merge(files, args.output, args.force)
    return 0


def cmd_bookkeep(args):
    from hell.bookkeeping import harvest
    harvest(args.directory, args.output, not args.no_lumis)
    return 0


def cmd_navigate(args):
    if args.target == "code":
        from hell.navigator import code
        if args.config:
            if args.html:
                code.to_html(args.config, args.options, args.html)
            print(code.report_graph(args.config, args.options))
        else:
            print(code.report_types(args.grep))
        return 0
    if args.target == "model":
        from hell.navigator import model
        print(model.report(args.options or None))
        return 0
    from hell.navigator import rootfile
    print(rootfile.report(args.target, args.branch, args.entries, not args.no_provenance, args.tree))
    return 0


def cmd_validate(args):
    from hell.validation import validate
    validate(args.config, args.output, args.max_hists)
    return 0


def cmd_build(args):
    from hell.build import build
    return build(args.jobs, args.clean, args.reconfigure, args.type, args.target, args.verbose)


def cmd_env(args):
    from hell.build import env_report
    print(env_report())
    return 0


def cmd_corrections(args):
    from hell.corrections import report, corrections, list_corrections
    print(report(args.era))
    if args.list:
        path = corrections(args.era)["files"].get(args.list)
        if not path:
            raise HellError(f"no file key '{args.list}'")
        for name, inputs in list_corrections(path, args.grep):
            print(f"  {name}: " + ", ".join(f"{n} ({t})" for n, t in inputs))
    return 0


def cmd_lumi(args):
    from hell.lumi import compute
    compute(args.era, args.json, args.trigger, args.unit, args.dry_run, args.update, not args.no_golden)
    return 0


def cmd_list_modules(args):
    from hell.runner import find_executable
    return subprocess.call([find_executable("hell-run"), "--list-modules"] + (["--json"] if args.json else []))


def cmd_dump(args):
    from hell.config import load_config
    from hell.runner import apply_options
    process = load_config(args.config, args.options)
    print(process.to_json())
    return 0


def cmd_issue(args):
    root = os.path.join(HELL_BASE, "docs", "issues")
    if args.action == "list":
        for issue_dir in sorted(glob.glob(os.path.join(root, "HELL-*"))):
            readme = os.path.join(issue_dir, "README.md")
            title, status = "", ""
            if os.path.exists(readme):
                for line in open(readme):
                    if line.startswith("# "):
                        title = line[2:].strip()
                    if line.lower().startswith("**status**"):
                        status = line.split(":", 1)[-1].strip()
            print(f"  {os.path.basename(issue_dir):40s} {status:22s} {title}")
        return 0
    if not args.title:
        raise HellError("give a title: hell issue new \"...\"")
    numbers = [int(os.path.basename(issue_dir).split("-")[1]) for issue_dir in glob.glob(os.path.join(root, "HELL-[0-9]*"))]
    number = max(numbers, default=0) + 1
    slug = "".join(char if char.isalnum() else "_" for char in args.title.lower()).strip("_")[:50]
    issue_dir = os.path.join(root, f"HELL-{number:04d}-{slug}")
    os.makedirs(os.path.join(issue_dir, "plots"), exist_ok=True)
    import datetime
    with open(os.path.join(root, "TEMPLATE.md")) as template_file:
        text = template_file.read().replace("{title}", args.title).replace("{date}", datetime.date.today().isoformat()).replace("{area}", args.area).replace("{id}", f"HELL-{number:04d}")
    with open(os.path.join(issue_dir, "README.md"), "w") as readme_file:
        readme_file.write(text)
    log(f"issue {issue_dir}/README.md opened. Document the problem, the numbers and how to reproduce.")
    return 0


def cmd_pray(args):
    steps = ["skim", "trees", "bookkeep", "hist", "validate", "fit", "ml", "navigate"]
    if args.list:
        print("steps of test/run_quick_test.sh (in order): " + " ".join(steps))
        return 0
    log("Praying... running test/run_quick_test.sh " + " ".join(args.steps))
    rc = subprocess.call(["bash", os.path.join(HELL_BASE, "test", "run_quick_test.sh")] + args.steps)
    log("Your prayers were heard. All forged." if rc == 0 else "The forge went cold. Check the output above.", "INFO" if rc == 0 else "ERROR")
    return rc


def cmd_version(args):
    from hell.runner import find_executable
    print(f"HELL python {__version__}, base {HELL_BASE}")
    try:
        subprocess.call([find_executable("hell-run"), "--version"])
    except HellError as err:
        warn(str(err))
    return 0


COMMANDS = {"run": cmd_run, "skim": cmd_run, "hist": cmd_hist, "unleash-systematics": cmd_unleash, "plot": cmd_plot, "fit": cmd_fit,
            "train": cmd_train, "samples": cmd_samples, "submit": cmd_submit, "merge": cmd_merge, "bookkeep": cmd_bookkeep,
            "navigate": cmd_navigate, "validate": cmd_validate, "build": cmd_build, "env": cmd_env, "lumi": cmd_lumi, "corrections": cmd_corrections, "list-modules": cmd_list_modules, "dump": cmd_dump, "issue": cmd_issue, "pray": cmd_pray, "version": cmd_version}


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    parser = build_parser()
    if not argv:
        from hell import banner
        print(banner() + "\n")
        parser.print_help()
        return 0
    # `hell run cfg.py help` -> config options help
    args = parser.parse_args(argv)
    if args.command in ("check", "resubmit", "status"):
        from hell.batch import condor
        task_actions = {"check": lambda: condor.check(args.tag) and 1 or 0,
                        "resubmit": lambda: condor.resubmit(args.tag, args.dry_run),
                        "status": lambda: condor.status(args.tag) and 1 or 0}
        try:
            return task_actions[args.command]()
        except HellError as err:
            error(str(err)); return 1
    try:
        return COMMANDS[args.command](args) or 0
    except HellError as err:
        error(str(err))
        return 1
    except KeyboardInterrupt:
        error("interrupted")
        return 130


if __name__ == "__main__":
    sys.exit(main())
