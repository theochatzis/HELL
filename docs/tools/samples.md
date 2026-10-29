# Samples, cross sections and bookkeeping

- `samples/<era>.yaml`: samples of an era with `das` (or `path`), `process`, `xsec` (key of `xsec.yaml` or a
  number), `isData`; plus `lumi` (pb^-1) and `goldenJSON`.
- `samples/xsec.yaml`: cross sections in pb with precision and source. Verify against XSDB/TOP before results.
- `samples/files/<era>/<sample>.txt`: cached file lists (`hell samples files --era 2022 [names]`), EOS paths
  when the files are on `/eos/cms`, xrootd otherwise.
- `hell samples create --era 2022 [--lumi --golden-json --campaign] '<DAS pattern>' [--process X] [--xsec KEY]`:
  queries DAS, appends the entries to `samples/<era>.yaml` and placeholder cross-section keys to `xsec.yaml`
  (to be filled by hand; a null cross section is refused at normalisation time). `--from-file` takes dataset
  names from a text file, `--dry-run` only prints. See `samples/README.md` for the full procedure.
- `samples/certification.yaml`: master list of Golden JSONs, run ranges, normtag. `hell lumi --era 2022
  [--trigger ...] [--json processed.json] [--update]` runs brilcalc in the cvmfs BRIL environment and records
  the result in `samples/lumi/<era>.json`; `hell bookkeep` writes the processed lumi sections of the data
  samples as `<output>_processed.json` for exactly this; `hell lumi --json` intersects them with the Golden
  JSON of the era first (the copied `LuminosityBlocks` trees list uncertified sections too, whose events the
  skim rejected).
- `hell samples list|das|xsec|show`.
- `hell bookkeep <dir> -o samples/bookkeeping/<era>.json`: per sample, files, events in/out, `genEventCount`,
  `genEventSumw(2)`, positive/negative sums, runs, processed lumi sections (Golden-JSON format, ready for
  `brilcalc lumi -i`).

`apply_options(process, options)` in a config resolves `sample=` to files, `isData`, `era`, cross section and
Golden JSON through this registry.
