# Object validation: data/MC agreement and scale factors

Two pieces:

1. `ObjectValidationAnalyzer` (module, in `analysis_cfg.py` as `objVal`, placed after the weight producers):
   per collection the distributions a POG recommendation touches (pt, eta, phi, isolation, dxy, dz, SIP3D,
   charge, id level, isolation category, multiplicity; jets: b-tag score and working point, jet id, raw factor,
   lepton cleaning dR, leading pt, HT; event: MET, vertices, rho). Each weighted histogram has a `_noSF` twin
   filled with the event weight divided by the scale factors carried by the objects of the collection, and the
   mean scale factor is profiled versus pt and eta (`scaleFactor_vs_pt`, `scaleFactor_vs_eta`).
2. `hell validate configs/SingleTopMass/validation.yaml`: sums the histograms per process (MC normalised to
   lumi x xsec / genEventSumw from the files, data added), draws stacked plots with ratio panels, and computes
   per distribution: data/MC yield ratio, chi2/ndf (data Poisson and MC statistical errors), KS distance and
   p-value (effective sample sizes), maximum |pull| and the number of bins with |pull| > 3. The markdown report
   (`validation.md`) shows the nominal and `_noSF` metrics side by side; a distribution is flagged when
   KS p < 0.01 or a bin pulls beyond 3 sigma. `normalizeShapes: true` compares shapes (MC scaled to the data
   yield) and reports the absolute yield ratio separately; set it to false to test the normalisation itself.

How to read it: after a scale factor is introduced (lepton ID/ISO/trigger, b-tagging, pileup), the nominal
columns should improve with respect to the `_noSF` ones in the variables the factor depends on (pt, eta,
number of vertices, b-tag multiplicity), and the yield ratio should move towards one. If the nominal is worse
than `_noSF`, the factor is applied wrongly (wrong era, wrong working point, double application). The profile
of the mean scale factor versus pt and eta must look like the POG map.

Outputs: `<output>/validation.md`, `validation.json` (all numbers, for tracking across versions), `plots/`.
Quick test: `hell validate test/validation_test.yaml`.
