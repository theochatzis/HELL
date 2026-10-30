# Skimming

The skim turns NanoAOD into smaller files, fast, with no corrections. It keeps what the analysis needs later and
everything required to normalise and certify the sample:
- the `Events` tree with the kept branches (`TreeWriter` with `keep`/`drop` globs; counters follow automatically),
- the `Runs` tree of every input (per-run `genEventCount`, `genEventSumw`, `genEventSumw2`, LHE/PS sums) and the
  `LuminosityBlocks` tree (run, lumi section) so that the Golden JSON can be applied later and the luminosity
  recomputed with `brilcalc` on exactly the processed lumi sections,
- `hell/counts`, `hell/cutflow`, `hell/provenance`,
- `genDists/`: gen-level distributions of all events before any selection, signed and |w| weighted, for the
  positive reweighting and normalisation checks (`GenDistributionAnalyzer`).

The full procedure for an era, from DAS to the production, is in `analyses/SingleTopMass/skim.md`.
Configuration: `configs/SingleTopMass/skim_cfg.py` (objects from `objects_cff.py`, loose thresholds, one lepton,
at least one jet, Golden JSON for data).
```
hell run configs/SingleTopMass/skim_cfg.py sample=TbarBQtoLNu_tch era=2022 maxEvents=10000 outputFile=skim.root
hell navigate skim.root
```
Speed: 5-20k events/s per core on lxplus reading from EOS (only the enabled branches are read, lazily).

## Batch
- CRAB is the recommended way to skim the central datasets: it runs at the sites holding the data, scales
  to full datasets, applies the Golden JSON as the lumi mask for data and reports the processed lumi
  sections. `hell submit run configs/SingleTopMass/skim_cfg.py --backend crab --sample A B --era 2022 --tag skim_v1 --output-dir /store/user/$USER/HELL/skims`
  inside a CMSSW area with `crab.sh` sourced (the client only; the job runs HELL from the LCG view). The
  in-job Golden JSON filter is off for CRAB jobs.
- HTCondor for private or local inputs, and for every step after the skim: `hell submit run configs/SingleTopMass/skim_cfg.py --sample A B --era 2022 --files-per-job 10 --output-dir /eos/user/$USER/HELL/skims/2022 --tag skim_v1 [--proxy]`
  then `hell check/resubmit/merge --tag skim_v1`. Outputs land in `<output-dir>/<sample>/<jobid>.root`; for
  data the Golden JSON of the era is shipped with the job and applied by `GoldenJSONFilter` inside it.
See [batch.md](batch.md) and `analyses/SingleTopMass/skim.md`.

After production: `hell bookkeep <dir> -o samples/bookkeeping/2022.json` records sums of weights, counts, runs
and processed lumi sections per sample.
