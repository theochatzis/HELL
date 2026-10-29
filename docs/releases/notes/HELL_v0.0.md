# HELL_v0.0

- **Date:** 2026-10-29
- **Branch:** `main`
- **Environment:** LCG view `LCG_108/x86_64-el9-gcc13-opt` (ROOT 6.36.02, python 3.12.11, torch 2.7.1)
- **Previous tag:** none

The skeleton of HELL: every step of an analysis exists and runs end to end on real Run 3 NanoAOD, with the
physics content of the SingleTopMass analysis as a baseline. Details in `docs/issues/HELL-0001-framework_skeleton`.

## New

- Core: `Forge` event loop, `Event` product store, `Producer`/`Filter`/`Analyzer` plugins with a factory and
  automatic globbing of plugin sources and packages, lazy typed branch readers with automatic type conversion,
  `HistService`, `OutputTree` (skim copy and flat tables), `Bookkeeper` (`hell/counts`, cutflows, `Runs` and
  `LuminosityBlocks` copied), `Provenance` (JSON tree with the parents' records), `ParameterSet` over JSON,
  ROOT message filter for the NanoAOD dictionary warnings.
- Data model: `Candidate`, `Jet` (with PF fractions), `Lepton` base with `Muon` and `Electron`, `MET`,
  `GenParticle`, `EventInfo`, `Weights`, `TopCandidate`; dictionary and flat-table persistence.
- Modules: NanoAOD readers, `JetSelector` with the manual jet id (`Modules/Selection/interface/JetId.h`),
  `MuonSelector`, `ElectronSelector`, `LeptonMerger`, `BTagger`, trigger, flag, collection-size, Golden-JSON and
  prescale filters, `GenWeightProducer` (muR/muF, ISR/FSR), `HistogramWeightProducer`, `WeightCollector`,
  `TopReconstructor`, `SingleTopVariables`, `TreeWriter`, `GenDistributionAnalyzer`, `KinematicsAnalyzer`,
  `ObjectValidationAnalyzer` (per channel, with and without scale factors), `MLPScoreProducer`, `JetVariation`.
- Tools: `hell-hist` (TTreeFormula histogrammer, weight and tree systematics, normalisation from the files,
  negative-yield warning), `hell-datacard` (template cards, parametric mtop workspace), `Plot/` (CMS TDR style).
- Python: the `hell` command (run, hist, unleash-systematics, plot, validate, fit, train, samples, lumi, submit,
  check, resubmit, status, merge, bookkeep, navigate, build, env, list-modules, dump, issue, pray, version), each
  with `--help` and an example; configs (`Process`, `Module`, `VarParsing`); sample registry with
  `hell samples create` from DAS, cross sections, master certification list and `hell lumi` (brilcalc); condor
  (file transfer of the framework and the Golden JSON) and CRAB backends; ML tools (PyTorch MLP, validation,
  JSON/TorchScript export); navigators; banner.
- SingleTopMass configurations: objects, skim, analysis tree, histograms, validation, fit, ML; 2022 samples.
- Documentation: README, design with the CMSSW dictionary, code practices, tool guides, methodology, physics
  notes, issue log, python module map, this release documentation.

## Validation

- build: `hell build --clean`, warning-free, LCG_108.
- test: `hell pray` on two files of TbarBQtoLNu t-channel, one file each of TTtoLNu2Q, WtoLNu-2Jets,
  t-channel mt=173.5 and Muon 2022C (Run3Summer22 NanoAODv12, 22Sep2023): skims, analysis trees with JES
  variations, bookkeeping, merge, histograms, plots, validation, datacards, ML training and C++ application
  (scores agree with PyTorch within 1e-7), navigation.
- `hell lumi --era 2022`: 8086.1 pb^-1 recorded (runs 355100 to 359021, normtag_PHYSICS), trace in
  `samples/lumi/2022.json`.
- productions: none yet.

## Known limitations

- JES/JER factors are constants (placeholders); no scale factors are applied yet (all 1), so the validation
  tool's with/without comparison is trivial.
- Combine is not in the LCG view: `hell fit` writes the cards and needs `HELL_COMBINE_SETUP` to run.
- Cross sections and b-tag working points are marked "verify"; DAS queries were tested only up to the proxy
  error (no grid proxy in the development session); condor and CRAB submissions were dry runs.
- The histogrammer is single-threaded.
