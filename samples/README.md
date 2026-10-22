# Samples

- `<era>.yaml`: the samples of an era (DAS dataset or local path, process name, cross-section key, data flag), the luminosity and the Golden JSON.
- `xsec.yaml`: cross sections in pb with their source. Every number must be traceable.
- `files/<era>/<sample>.txt`: cached file lists (not in git). Made by `hell samples files --era 2022 [names]`
  with `dasgoclient`; files present under `/eos/cms` are used directly, the others through xrootd.
- `bookkeeping/<era>.json` (optional, made by `hell bookkeep`): sums of weights, event counts, runs and processed lumi sections of produced files.

```
hell samples list --era 2022
hell samples das '/TbarBQtoLNu-t-channel*/Run3Summer22NanoAODv12*/NANOAODSIM'
hell samples files --era 2022 TbarBQtoLNu_tch
hell samples xsec TbarBQtoLNu_tch
```
