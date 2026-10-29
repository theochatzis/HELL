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
- HTCondor (local or xrootd files): `hell submit run configs/SingleTopMass/skim_cfg.py --sample A B --era 2022 --files-per-job 10 --output-dir /eos/user/$USER/HELL/skims/2022 --tag skim_v1 [--proxy]`
  then `hell check/resubmit/merge --tag skim_v1`. Outputs land in `<output-dir>/<sample>/<jobid>.root`.
  For data the Golden JSON of the era is shipped with the job and applied by `GoldenJSONFilter` inside it.
- CRAB (DAS datasets, Golden JSON applied by CRAB for data through the lumi mask; the in-job filter is off): `hell submit run ... --backend crab --output-dir /store/user/$USER/HELL/skims --tag skim_v1`
  inside a CMSSW area with `crab.sh` sourced. See [batch.md](batch.md).

After production: `hell bookkeep <dir> -o samples/bookkeeping/2022.json` records sums of weights, counts, runs
and processed lumi sections per sample.
