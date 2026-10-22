# Framework skeleton: design, first working chain

**Id**: HELL-0001  **Area**: framework  **Opened**: 2026-10-21
**Status**: done
**Impact**: baseline of everything that follows

## Context
HELL is started as the single framework for the SingleTopMass analysis and later ones: C++ modular core with a
CMSSW-like data model, python configurations, skim / analysis tree / histograms / ML / fit chain, condor and
CRAB, bookkeeping and provenance in every file. References studied: HARP (module/plugin design, python->JSON
configs, condor tarball), Reco2Pico and SkimRDFAnalysisBase (Runs-tree bookkeeping, provenance strings,
navigator, normalisation 1/sum w), TopNanoAnalysis (sample lists from DAS, cross-section YAML, positive
reweighting from gen distributions, skim content), the old 2016 analysis code (selection, neutrino
reconstruction and regression, parametric mtop fit with Combine).

## What was done
- Core: `Forge` event loop, `Event` product store, `Producer/Filter/Analyzer`, plugin factory, lazy typed
  branch readers with automatic type conversion, `HistService`, `OutputTree` (skim copy + flat tables),
  `Bookkeeper` (counts, cutflow, Runs/LuminosityBlocks copy), `Provenance` (JSON tree with parent chain).
- DataFormats: Candidate, Jet, Lepton (base), Muon, Electron, MET, GenParticle, EventInfo, Weights, TopCandidate; dictionary; persistence.
- Modules: Nano readers, selectors (jets, muons and electrons with iso categories and vetoes, b-tag WPs), filters
  (trigger, flags, collection size, Golden JSON, prescale), weights (gen + muR/muF/ISR/FSR, histogram lookup,
  collector), `TopReconstructor`, `SingleTopVariables`, `TreeWriter`, `GenDistributionAnalyzer`,
  `KinematicsAnalyzer`, `ObjectValidationAnalyzer` (distributions with and without scale factors), `MLPScoreProducer`, `JetVariation`.
- Tools: `hell-hist` (TTreeFormula histogrammer with weight and tree systematics, normalisation from the
  files), `hell-datacard` (template cards; parametric RooFit workspace with mtop-dependent signal).
- Python: `hell` CLI (run, hist, unleash-systematics, plot, fit, train, samples, submit/check/resubmit/merge,
  bookkeep, navigate, validate (data/MC metrics: yield ratio, chi2, KS, pulls), issue, pray), configs, samples registry, condor and CRAB backends, ML (data, MLP,
  training, validation, JSON/TorchScript export), navigators (file, code, model).
- Configs: `configs/SingleTopMass/{objects_cff,skim_cfg,analysis_cfg}.py`, `hist.yaml`, `fit.yaml`, `ml_*.yaml`;
  `samples/2022.yaml`, `samples/xsec.yaml`.

## Findings / numbers
Quick test (`hell pray`, Run3Summer22 NanoAODv12, 2 files of TbarBQtoLNu t-channel, 1 file each of ttbar
semileptonic, W+2jets, t-channel mt=173.5, Muon 2022C):

| Step | Result |
|---|---|
| skim TbarBQtoLNu_tch | 4130 -> 1294 events, 244 branches copied, ~4k ev/s; `sumw_in` = `genEventSumw` = 96837.74 |
| skim W+2jets (1 file) | 717696 -> 85167 events, 18k ev/s |
| analysis tree | 1294 -> 739 events (1 lepton, no veto, 2-3 jets), nominal/jesUp/jesDown 739/745/724 |
| histograms | 4 processes x 3 regions x 4 variables with 3 systematics: 336 histograms in 3 s |
| datacards | template (2 regions, Asimov data) and parametric workspace written |
| ML | 3-class MLP, AUC 0.79/0.77/0.67 on 8 variables; C++ vs PyTorch scores agree within 1.2e-7 |
| data | Golden JSON filter loads 362 runs; 50000 -> 12915 skimmed events |

## Conclusion / decision
The skeleton runs end to end. Decisions taken:
- weight systematics as columns, object systematics as one job per variation (`variation=`);
- normalisation from `Runs.genEventSumw` propagated through `hell/counts` and the copied Runs tree;
- provenance as a concatenating TTree of JSON records including the parents' records;
- ML models exported to JSON and evaluated in C++ without libtorch.
Open (next issues): correctionlib JES/JER sources and SFs; positive reweighting tool; gen-level neutrino;
parametric model shapes; CRAB end-to-end test; multi-threading of hell-hist.

Reproduce: `source env/setup.sh; hell pray` (outputs in `test/out/`).
