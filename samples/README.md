# Samples

- `<era>.yaml`: the samples of an era (DAS dataset or local path, process name, cross-section key, data flag), the luminosity and the Golden JSON.
- `xsec.yaml`: cross sections in pb with their source. Every number must be traceable.
- `certification.yaml`: the master list of Golden JSONs per era (with the run range of sub-eras such as 2022
  and 2022EE), the normtag and the BRIL environment. `hell samples create` takes the Golden JSON from it, the
  registry falls back to it, and `hell lumi` computes the luminosity from it:
  `hell lumi --era 2022 [--trigger HLT_IsoMu24] [--json <processed lumi sections from hell bookkeep>] [--update]`
  writes `samples/lumi/<era>.json` (command, JSON, normtag, recorded and delivered per trigger) and with
  `--update` the `lumi:` line of the era file.
- `files/<era>/<sample>.txt`: cached file lists (not in git). Made by `hell samples files --era 2022 [names]`
  with `dasgoclient`; files present under `/eos/cms` are used directly, the others through xrootd.
- `bookkeeping/<era>.json` (optional, made by `hell bookkeep`): sums of weights, event counts, runs and processed lumi sections of produced files.

## Creating or extending an era file
```
voms-proxy-init --voms cms --rfc                       # DAS needs a grid proxy
hell samples create --era 2022 --lumi 7980.4 \
    --golden-json /eos/user/c/cmsdqm/www/CAF/certification/Collisions22/Cert_Collisions2022_355100_362760_Golden.json \
    --campaign Run3Summer22NanoAODv12-130X_mcRun3_2022_realistic_v5 \
    '/TbarBQtoLNu-t-channel-4FS_TuneCP5_13p6TeV_powheg-madspin-pythia8/Run3Summer22NanoAODv12-130X_mcRun3_2022_realistic_v5*/NANOAODSIM' --process tch
hell samples create --era 2022 '/TTtoLNu2Q_TuneCP5_13p6TeV_powheg-pythia8/Run3Summer22NanoAODv12*/NANOAODSIM' --process tt
hell samples create --era 2022 '/Muon/Run2022[CD]-22Sep2023-v1/NANOAOD'           # data: isData, process data
hell samples create --era 2022 --from-file datasets.txt --dry-run                  # without DAS, from a list
```
Every call queries DAS for the patterns, names the samples (`TbarBQtoLNu_t_channel_4FS`, `Muon_2022C`), appends
the entries to `samples/<era>.yaml` (a new file gets the `era`, `lumi`, `goldenJSON`, `campaign` header; existing
names and datasets are never touched) and adds placeholder keys to `samples/xsec.yaml` with `xsec: null`. Then by
hand: fill the cross sections with their source (the normalisation refuses a null value), rename the samples or
processes if you like, and verify `lumi` and `goldenJSON`. `--process`, `--xsec` and `--name` set those fields
for the created entries.

```
hell samples list --era 2022
hell samples das '/TbarBQtoLNu-t-channel*/Run3Summer22NanoAODv12*/NANOAODSIM'
hell samples files --era 2022 TbarBQtoLNu_tch
hell samples xsec TbarBQtoLNu_tch
```
