# Positive reweighting of negative generator weights

NLO samples (aMC@NLO, Powheg with MiNLO/madspin in some setups) contain events with negative generator
weights. They are needed for the correct normalisation and shapes but they reduce the effective statistics and
make templates noisy or negative in bins, which hurts fits and ML training.

Method (developed in TopNanoAnalysis, `SingleTopAnalysis/PositiveReweighting`): choose gen-level variables
`x` that describe the kinematics the negative weights are correlated with (for single top: top pt and lepton
pt; for W+jets: W pt and eta). Fill two histograms before any selection, `H_w(x) = sum w` and
`H_abs(x) = sum |w|`. The per-bin factor `SF(x) = H_w(x) / H_abs(x)` applied to `|w|` gives a positive weight
`w' = |w| SF(x)` that reproduces the signed distribution of `x` and, as long as the chosen `x` captures the
correlation, all other distributions. The total `sum w'` equals `sum w` (keep it that way; do not add a
global factor that normalises to `sum |w|`, the normalisation must stay the signed one).

In HELL: `GenDistributionAnalyzer` in the skim path fills, for every event of the sample, the signed and
`|w|` histograms (`genDists/topPt`, `topPt_abs`, `topPt_leptonPt`, `..._abs`) so that the map can be derived
from the skims without going back to NanoAOD. Next steps: a `hell reweight` tool that builds the map
(correctionlib JSON) from the merged skims, a `PositiveWeightProducer` that applies it in the analysis step,
and validation plots (reweighted vs signed in all analysis variables, including high-pt slices).
