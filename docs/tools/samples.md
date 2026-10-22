# Samples, cross sections and bookkeeping

- `samples/<era>.yaml`: samples of an era with `das` (or `path`), `process`, `xsec` (key of `xsec.yaml` or a
  number), `isData`; plus `lumi` (pb^-1) and `goldenJSON`.
- `samples/xsec.yaml`: cross sections in pb with precision and source. Verify against XSDB/TOP before results.
- `samples/files/<era>/<sample>.txt`: cached file lists (`hell samples files --era 2022 [names]`), EOS paths
  when the files are on `/eos/cms`, xrootd otherwise.
- `hell samples list|das|xsec|show`.
- `hell bookkeep <dir> -o samples/bookkeeping/<era>.json`: per sample, files, events in/out, `genEventCount`,
  `genEventSumw(2)`, positive/negative sums, runs, processed lumi sections (Golden-JSON format, ready for
  `brilcalc lumi -i`).

`apply_options(process, options)` in a config resolves `sample=` to files, `isData`, `era`, cross section and
Golden JSON through this registry.
