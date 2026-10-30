# Central corrections through correctionlib

**Id**: HELL-0002  **Area**: objects  **Opened**: 2026-10-29
**Status**: done (first version)
**Impact**: every correction and scale factor of the analysis comes from the POG JSONs of the era; no number typed in configs

## Context
Working points, JES/JER, pileup and lepton scale factors were placeholders or hand-typed values. The POG
correctionlib JSONs on cvmfs (jsonpog-integration) are the reference; TopNanoAnalysis read the b-tag working
points and the JEC from them. HELL now does it for everything.

## What was done
- `samples/corrections.yaml`: per era the POG directory and the correction names (2022, 2022EE); `hell corrections --era 2022`.
- `Corrections/` library: `CorrectionService` (files opened once, lookup by name), `EraCorrections` (the job's view).
- Modules: `JetEnergyCorrector` (JER hybrid smearing, `jes<Source>Up/Down` and `jerUp/Down` variations, optional JEC
  re-application, MET propagation), `PileupWeightProducer`, `MuonScaleFactorProducer`, `ElectronScaleFactorProducer`,
  `BTagEfficiencyAnalyzer` + `BTagScaleFactorProducer` (fixed working point), `JetVetoMapFilter`; `BTagger` reads the
  working points from the JSON; `JetSelector` can use the central jet id (NanoAODv13+).
- `objects_cff.jets()` and `objects_cff.weights()` book the chain; `analysis_cfg.py` uses them.

## Findings / numbers
Quick test, TbarBQtoLNu t-channel 2022 (2 NanoAOD files), analysis selection:

| quantity | value |
|---|---|
| JER smearing factor `Jet_scale`, nominal | mean 1.008, rms 0.056 |
| `jesRegrouped_TotalUp` | mean jet scale 1.036 |
| pileup weight | mean 0.987, rms 0.49 |
| muon id, trigger weights | 0.991, 0.981 |
| electron id, trigger weights | 0.982, 0.980 |
| `Muon_scaleFactor` (id x iso x trigger, leading muon) | mean 0.948 |
| events in the analysis tree: nominal / jesUp / jerDown | 695 / 714 / 694 |

## Conclusion / decision
Corrections are centralised; a new era or POG version is a change of `samples/corrections.yaml`. Open:
b-tag efficiency maps per era (run `analysis_cfg.py btagEfficiencyMaps=True` on the MC, merge), the
2022EE validation of the names, muon scale/smearing and electron scale/smearing (not applied), MET
unclustered variation, data JEC re-application per run era.

Reproduce: `hell pray`, or the commands in `docs/tools/corrections.md`.
