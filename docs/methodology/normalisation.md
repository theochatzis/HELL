# Normalisation of simulated samples

Weight of a simulated event: `w = L * sigma * genWeight * (scale factors) / sum_i genWeight_i`, where the sum
runs over **all generated events** of the sample, before any selection.

Where each number comes from in HELL:
- `L`: `samples/<era>.yaml: lumi` (pb^-1). To be computed with `brilcalc` on the processed lumi sections
  (`hell bookkeep` writes them in Golden-JSON format) with the normtag of the era.
- `sigma`: `samples/xsec.yaml`, with precision and source per entry.
- `sum genWeight`: the NanoAOD `Runs` tree (`genEventSumw`, per run entry). `hell-run` copies the Runs tree
  of every input file and adds the sums to `hell/counts: genEventSumw`; both survive skims, merges and the
  analysis step. The histogrammer reads `genEventSumw` from the files of each sample.
- Consistency: in a job that processes all events of its inputs, `sumw_in` (sum of the processed genWeights)
  equals `genEventSumw`; `hell bookkeep` checks it. For derived files (`events_in < genEventCount`) only
  `genEventSumw` is meaningful.
- Negative weights (aMC@NLO, Powheg at NLO): the sums are signed, so the normalisation is correct; the
  statistical power is reduced. See [positive_reweighting.md](positive_reweighting.md).

Data: weight 1, Golden JSON applied in the skim (`GoldenJSONFilter`) or by CRAB.
