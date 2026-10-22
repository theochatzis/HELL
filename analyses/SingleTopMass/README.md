# SingleTopMass

Top quark mass in single-top t-channel events. Configurations: `configs/SingleTopMass/`. Methodology and
status: `docs/methodology/single_top_mass.md`. Objects: `docs/physics/objects.md`. Running log: `docs/issues/`.

This directory holds analysis-specific studies and scripts (one sub-directory per study, each with a README
and the commands used), as opposed to the framework tools in `python/hell` and the module code in `Modules/`.

```
hell run configs/SingleTopMass/skim_cfg.py sample=TbarBQtoLNu_tch era=2022 maxEvents=10000
hell run configs/SingleTopMass/analysis_cfg.py inputFiles=skim.root era=2022 sample=TbarBQtoLNu_tch
hell hist configs/SingleTopMass/hist.yaml
hell fit configs/SingleTopMass/fit.yaml --datacards
hell train configs/SingleTopMass/ml_classifier.yaml
```
