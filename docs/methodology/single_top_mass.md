# SingleTopMass: strategy and status

Goal: measure the top quark mass in single-top t-channel events (one lepton, two jets of which one b-tagged,
forward light jet), Run 3 (2022 first), as a cross-check of the ttbar measurements in a different colour
environment.

## Chain in HELL
1. Skim (`skim_cfg.py`): trigger, MET filters, Golden JSON (data), loose one-lepton and one-jet selection, kept branches,
   gen distributions, bookkeeping. CRAB or condor.
2. Analysis tree (`analysis_cfg.py`): tight leptons (isolated and non-isolated for QCD), jets cleaned, b-tag
   working points, exactly one lepton with vetoes, 2 or 3 jets, weights with variations, top reconstruction,
   `st_*` variables, optional ML score; one job per object variation.
3. Histograms (`hist.yaml`): regions 2j1b (signal, mu/e), 3j1b and 3j2b (ttbar control), 2j1b non-isolated
   (QCD control); observables `m(l nu b)`, `m(lb)`, `mT(W)`, `|eta(j')|`, ...
4. Fit (`fit.yaml`): templates per region with shape systematics, or the parametric model with `mtop` as POI
   calibrated on the mass-variation samples (166.5 ... 178.5 GeV).

## Observables and method (from the previous analysis, to be re-evaluated)
- Neutrino: W-mass constraint; previously regressed (px, py, pz) with BDTs and a chi2 fit; the HELL
  `TopReconstructor` implements the analytic solution (real part if complex), the regression is a planned
  `ml_regressor` target once the gen-level neutrino is stored.
- Signal vs background separation with a multiclass classifier on variables weakly correlated with the mass
  (`|eta(j')|`, `cos theta*`, `dEta(b, j')`, HT, lepton pt, ...); check the sculpting with the spectator plots.
- QCD from the non-isolated sideband (data minus non-QCD MC), shape fitted with a sigmoid.
- Mass extraction: parametric shapes with linear mtop dependence, simultaneous fit of the mass points,
  decorrelated shape nuisances as `param` lines; linearity and pull tests with toys.

## Status (2026-10-21)
Skeleton complete and exercised end-to-end on Run3Summer22 NanoAODv12 (`hell pray`). Next: real JES/JER
sources via correctionlib, scale factors (pileup, b-tag, lepton), positive reweighting tool, gen-level
neutrino for the regression, b-jet energy regression study, QCD estimation, parametric model shapes.
See `docs/issues/` for the running log.
