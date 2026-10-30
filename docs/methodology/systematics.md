# Systematic variations

Three types of variations exist and HELL treats them in two ways.

| Type | Examples | Implementation | In the histogrammer |
|---|---|---|---|
| normalisation | lumi, cross sections, QCD normalisation | numbers in `fit.yaml: normalisation` | `lnN` in the datacard |
| weight (per-event scale factor varied) | pileup, b-tag SF, lepton ID/ISO/trigger SF, muR/muF, ISR/FSR, PDF | columns of the `Weights` product: `Weight_<name>Up/Down` are the total weight with that factor varied; produced by the `*WeightProducer` and `*ScaleFactorProducer` modules (central JSONs) in the analysis step | `type: weight`, `up: "Weight_puUp/Weight_nominal"` |
| object (kinematics of objects changed) | JES sources, JER smearing, MET unclustered, lepton scale | one analysis job per variation (`variation=jesRegrouped_TotalUp`), the `JetEnergyCorrector` applies the shift of the active variation from the JME JSON and propagates it to MET; selections and the top reconstruction run on the varied objects; output tree in `<trees>_<variation>/` | `type: tree`, `up: jesRegrouped_TotalUp` |

- `hell unleash-systematics analysis_cfg.py ...` produces the nominal and all variation trees (locally or on condor).
- Templates `<process>_<syst>Up/Down` are written by `hell hist`; `hell fit --datacards` turns the ones listed
  in `shapeSystematics` into `shape` lines.
- JES and JER come from the central JME JSON through `JetEnergyCorrector`: JER smearing of the nominal MC
  (hybrid method), `jerUp/jerDown` for the resolution scale factor variations, and `jes<Source>Up/Down` for
  any uncertainty source of the JSON (`jesRegrouped_TotalUp`, or the reduced set `jesRegrouped_Absolute`,
  `jesRegrouped_BBEC1`, ... one job each). The sources used are listed in `samples/corrections.yaml`.
- Weight systematics from the central JSONs: `pu` (LUM), `muId`, `muIso`, `muTrig` (MUO), `elId`, `elReco`,
  `elTrig` (EGM), `btagHeavy`, `btagLight` (BTV, fixed working point with MC efficiencies). They appear as
  `Weight_<name>Up/Down` columns and in `hist.yaml` as `type: weight` entries.
- Modelling variations that come as separate samples (hdamp, tune, colour reconnection, top mass) are
  separate processes in `hist.yaml` and enter the fit as alternative templates or as the mass calibration.
