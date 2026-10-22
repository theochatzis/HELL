# Systematic variations

Three types of variations exist and HELL treats them in two ways.

| Type | Examples | Implementation | In the histogrammer |
|---|---|---|---|
| normalisation | lumi, cross sections, QCD normalisation | numbers in `fit.yaml: normalisation` | `lnN` in the datacard |
| weight (per-event scale factor varied) | pileup, b-tag SF, lepton ID/ISO/trigger SF, muR/muF, ISR/FSR, PDF | columns of the `Weights` product: `Weight_<name>Up/Down` are the total weight with that factor varied; produced by `*WeightProducer` modules in the analysis step | `type: weight`, `up: "Weight_puUp/Weight_nominal"` |
| object (kinematics of objects changed) | JES sources, JER smearing, MET unclustered, lepton scale | one analysis job per variation (`variation=jesUp`), the `JetVariation` producer applies the factor when its name matches the active variation and propagates it to MET; selections and the top reconstruction run on the varied objects; output tree in `<trees>_<variation>/` | `type: tree`, `up: jesUp` |

- `hell unleash-systematics analysis_cfg.py ...` produces the nominal and all variation trees (locally or on condor).
- Templates `<process>_<syst>Up/Down` are written by `hell hist`; `hell fit --datacards` turns the ones listed
  in `shapeSystematics` into `shape` lines.
- The factors of JES/JER are placeholders (constants) in the skeleton; the next step is to read them from the
  JERC correctionlib JSONs (per source) and to store the per-jet factors in the skim (`Jet_jesTotalUp` style
  branches, as done in TopNanoAnalysis), which `JetVariation` already supports through `branch:`.
- Modelling variations that come as separate samples (hdamp, tune, colour reconnection, top mass) are
  separate processes in `hist.yaml` and enter the fit as alternative templates or as the mass calibration.
