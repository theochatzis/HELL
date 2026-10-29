# python/hell

The python layer of HELL: configuration of C++ module sequences, sample registry, orchestration (local,
HTCondor, CRAB), histogram and fit configurations, plotting, ML tools and navigation. Python never runs
physics: the event loop, the histogrammer and the datacard writers are C++ (`hell-run`, `hell-hist`,
`hell-datacard`). Every module has a docstring with the details; this page is the map. Heavy imports (ROOT,
torch) happen inside functions so that `hell --help` is instant.

| Module | What it does | Command / example |
|---|---|---|
| `cli.py` | The `hell` command: argparse sub-commands, each with `--help` and an example. Dispatches to the modules below. | `hell --help`, `hell run --help` |
| `__init__.py` | Package constants (`HELL_BASE`, `HELL_BUILD`), `log/warn/error` with the `[HELL]` prefix, `HellError` (user-facing errors, printed without traceback), the banner. | `from hell import log, HellError` |
| `config.py` | `Process`, `Module`, `VarParsing`, `load_config`: python configs in the CMSSW spirit, turned into the JSON that `hell-run` executes. | `process.goodJets = Module("JetSelector", src="jets", minPt=30)` |
| `runner.py` | Runs a python config through `hell-run`: resolves `sample=`/`inputFiles=`, writes `work/<cfg>.json`, calls the executable. `apply_options(process, options)` is what configs call. | `hell run configs/SingleTopMass/skim_cfg.py sample=TbarBQtoLNu_tch era=2022 maxEvents=5000` |
| `samples.py` | Sample registry: `samples/<era>.yaml`, `samples/xsec.yaml`, cached file lists from DAS or local paths, `find_sample`, `files`, `cross_section`, `resolve_inputs`. | `hell samples list --era 2022`, `hell samples files --era 2022 TbarBQtoLNu_tch`, `hell samples das '/TbarBQ*/Run3Summer22NanoAODv12*/NANOAODSIM'` |
| `build.py` | `hell build` (cmake + make, incremental, reconfigures after a commit) and `hell env` (environment and build report). | `hell build --clean -j 16`, `hell env` |
| `histcfg.py` | Turns `hist.yaml` (processes, colours, regions, variables, systematics) into the JSON of `hell-hist`: resolves sample files, cross sections and variation trees. | `hell hist configs/SingleTopMass/hist.yaml -o hists.root --process tch` |
| `plot.py` | Stacked data/MC plots with ratio from a `hell-hist` file, CMS TDR style; `stack_with_ratio` is the shared drawing function. | `hell plot hists.root -o plots --logy --extra "Work in progress"` |
| `tdr.py` | Loads the C++ `hell::tdr` style library (`Plot/`) into PyROOT: `lumi_text`, `ratio_canvas`, `legend`, `color`. | `from hell import tdr; T = tdr.load()` |
| `rootutils.py` | `quiet_root()`: ROOT in batch mode, info chatter off, the NanoAOD "no dictionary for class edm::" warnings filtered. Use it instead of `import ROOT` in tools. | `from hell.rootutils import quiet_root; ROOT = quiet_root()` |
| `validation.py` | Data/MC validation of objects and scale factors: sums `objVal*` histograms per process, metrics (yield ratio, chi2/ndf, KS, pulls) with and without scale factors, plots and a markdown/JSON report. | `hell validate configs/SingleTopMass/validation.yaml` |
| `merge.py` | Merges HELL files (ROOT `TFileMerger`) and checks that the bookkeeping counts add up. | `hell merge -o merged/tch.root /eos/.../TbarBQtoLNu_tch/*.root`, `hell merge --tag skim_v1` |
| `bookkeeping.py` | Harvests `hell/counts`, runs and lumi sections of produced files per sample into a JSON (sum of weights for the normalisation, processed lumi sections for brilcalc). | `hell bookkeep /eos/.../skims/2022 -o samples/bookkeeping/2022.json` |
| `batch/condor.py` | HTCondor tasks: packs the framework, writes `jobs.json`/`job.sh`/`job.sub` for run, hist, train or arbitrary commands; check, resubmit, merge per sample. | `hell submit run configs/SingleTopMass/skim_cfg.py --sample A B --era 2022 --files-per-job 10 --output-dir /eos/... --tag skim_v1`, `hell check --tag skim_v1` |
| `batch/crab.py` | CRAB configs for skims of DAS datasets: `scriptExe` wrapper, dummy PSet, lumi mask for data. | `hell submit run configs/SingleTopMass/skim_cfg.py --backend crab --sample Muon_2022C --era 2022 --tag skim_v1 --output-dir /store/user/you/HELL` |
| `fit/combine.py` | Writes the `hell-datacard` JSON from `fit.yaml` (template or parametric), combines cards, runs `combine` with the chosen strategy (needs `HELL_COMBINE_SETUP`). | `hell fit configs/SingleTopMass/fit.yaml --datacards`, `hell fit ... --parametric --strategy 2` |
| `ml/data.py` | Reads analysis trees into arrays with a preselection, features, spectators and weights; builds classifier or regressor datasets (class weights equalised). | configured by `ml_*.yaml` |
| `ml/models.py` | The MLP (PyTorch) and the layer export used by the C++ `MLPScoreProducer`. | `make_mlp(n_inputs, n_outputs, hidden=[64, 64])` |
| `ml/train.py` | Training loop (cross-entropy or Huber loss), input scaler, best epoch on the test loss. | `hell train configs/SingleTopMass/ml_classifier.yaml --epochs 40` |
| `ml/validate.py` | ROC/AUC, score distributions train vs test (KS), score vs spectators (mass sculpting), residuals and response for regressors; JSON report. | produced by `hell train` |
| `ml/export.py` | Exports the trained model as JSON (weights + scaler, evaluated in C++ without libtorch) and TorchScript. | model=`models/st_classifier.json` in `analysis_cfg.py` |
| `ml/run.py` | The `hell train` entry point: data, training, export, validation in one go. | `hell train test/ml_test.yaml --max-events 20000` |
| `navigator/rootfile.py` | Navigate a ROOT file: structure, bookkeeping, cutflow, branches grouped into tables with the module that wrote them, provenance chain, sample values. | `hell navigate skim.root --branch 'Jet_*' --entries 3` |
| `navigator/code.py` | Module types with doc, file:line and parameters; module graph of a configuration (text or HTML). | `hell navigate code --grep Jet`, `hell navigate code --config configs/SingleTopMass/analysis_cfg.py inputFiles=skim.root era=2022 --html page.html` |
| `navigator/model.py` | The data model from the ROOT dictionary of `libHellDataFormats`. | `hell navigate model Jet Muon` |

Related: `bin/hell` (the launcher), `scripts/make_banner.py` (the banner art), `test/run_quick_test.sh`
(`hell pray`). Documentation of the concepts and the YAML formats is in `docs/tools/`.
