# Fitting

`hell fit configs/SingleTopMass/fit.yaml` writes Combine inputs from a histogram file and runs `combine`.

## Template (shape) datacards
`hell-datacard fit.json` writes per region `<output>_<region>.root` (`<process>`, `<process>_<syst>Up/Down`,
`data_obs`; Asimov from the MC sum if no data process exists) and `<output>_<region>.txt` with the shape
systematics found in the histogram file and the `lnN` normalisation uncertainties of the YAML, plus
`autoMCStats`. Several regions are combined with `combineCards.py`.

## Parametric workspace
`hell fit fit.yaml --parametric` builds a RooFit workspace (`hell::ParametricWorkspace`): a signal pdf whose
mean and width depend linearly on `mtop` (calibrated on the mass-variation templates listed in `massPoints`),
binned backgrounds, `data_obs`, and a datacard with `mtop flatParam` as the parameter of interest. This is the
skeleton of the old analysis' method (Bukin/CB shapes with linear mtop dependence, simultaneous fit of the
mass points, decorrelated shape nuisances); the pdf and the calibration are the parts to develop next.

## Running Combine
Combine is not part of the LCG view. Point `HELL_COMBINE_SETUP` to a script that provides `combine` (a CMSSW
area with HiggsAnalysis/CombinedLimit, or the standalone installation), then
```
hell fit fit.yaml --strategy 2 -- --robustFit 1
```
`--strategy` sets `--cminDefaultMinimizerStrategy`. `--dry-run` prints the commands. On failure:
`[HELL] Fit failed to converge. Try hell fit --strategy 2, or hell pray.`
