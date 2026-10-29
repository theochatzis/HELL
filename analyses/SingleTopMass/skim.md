# SingleTopMass: from DAS to skims, step by step

Everything below is done once per era (2022 first, then 2022EE). Commands run from the repository root
after `source env/setup.sh`. Files written by the tools start with the command that made them, so each step
can be redone later. The version you produce with is what `git describe --tags` says: tag a pre-release
(`HELL_v1.0_pre1`) before a real production and put the tag in the output path.

## 0. Prerequisites

```bash
source env/setup.sh                         # LCG view, hell on the PATH
hell build                                  # warning-free, picks up a new git version
voms-proxy-init --voms cms --rfc            # DAS, xrootd and CRAB need a grid proxy (valid ~ 1 week with -valid 192:00)
export X509_USER_PROXY=$(voms-proxy-info -path)
```
For CRAB, additionally a CMSSW area with `cmsenv` and `source /cvmfs/cms.cern.ch/crab3/crab.sh`.

## 1. Certification: the Golden JSON of the era

`samples/certification.yaml` is the master list: Golden JSON, run range (2022 is split into 2022 = runs
355100 to 359021 and 2022EE = 359022 to 362760, sharing one file), normtag, BRIL environment. Check it
against https://cms-service-dqmdc.web.cern.ch/CAF/certification/ before a production; the paths there are
the latest files, the ones used by the TOP recommendations are listed in the twiki of the era.

## 2. Samples: the era file from DAS

Find the datasets (the campaign for 2022 is `Run3Summer22NanoAODv12-130X_mcRun3_2022_realistic_v5`, for
2022EE `Run3Summer22EENanoAODv12-130X_mcRun3_2022_realistic_postEE_v6`; data are the `22Sep2023` re-recos):
```bash
hell samples das '/TbarBQtoLNu-t-channel*/Run3Summer22NanoAODv12*/NANOAODSIM'
hell samples das '/TT*to*_TuneCP5_13p6TeV_powheg-pythia8/Run3Summer22NanoAODv12*/NANOAODSIM'
hell samples das '/Muon/Run2022[CD]-22Sep2023-v1/NANOAOD'
```
Create the entries (process names are what the histogram configuration groups by: `tch`, `tt`, `tw`,
`wjets`, `dy`, `data`):
```bash
hell samples create --era 2022 --campaign Run3Summer22NanoAODv12-130X_mcRun3_2022_realistic_v5 \
    '/TbarBQtoLNu-t-channel-4FS_TuneCP5_13p6TeV_powheg-madspin-pythia8/Run3Summer22NanoAODv12*/NANOAODSIM' \
    '/TBbarQtoLNu-t-channel-4FS_TuneCP5_13p6TeV_powheg-madspin-pythia8/Run3Summer22NanoAODv12*/NANOAODSIM' --process tch
hell samples create --era 2022 '/TTtoLNu2Q_TuneCP5_13p6TeV_powheg-pythia8/Run3Summer22NanoAODv12*/NANOAODSIM' \
                               '/TTto2L2Nu_TuneCP5_13p6TeV_powheg-pythia8/Run3Summer22NanoAODv12*/NANOAODSIM' --process tt
hell samples create --era 2022 '/TWminustoLNu2Q_TuneCP5_13p6TeV_powheg-pythia8/Run3Summer22NanoAODv12*/NANOAODSIM' \
                               '/TbarWplustoLNu2Q_TuneCP5_13p6TeV_powheg-pythia8/Run3Summer22NanoAODv12*/NANOAODSIM' --process tw
hell samples create --era 2022 '/WtoLNu-2Jets_TuneCP5_13p6TeV_amcatnloFXFX-pythia8/Run3Summer22NanoAODv12*/NANOAODSIM' --process wjets
hell samples create --era 2022 '/DYto2L-2Jets_MLL-50_TuneCP5_13p6TeV_amcatnloFXFX-pythia8/Run3Summer22NanoAODv12*/NANOAODSIM' --process dy
hell samples create --era 2022 '/Muon/Run2022[CD]-22Sep2023-v1/NANOAOD' '/EGamma/Run2022[CD]-22Sep2023-v1/NANOAOD'
hell samples list --era 2022
```
The first call of a new era writes the header (`era`, `lumi`, `goldenJSON` from the master list,
`campaign`); later calls append. Existing names and datasets are never touched. `--dry-run` shows what would
be written; `--from-file datasets.txt` works without DAS. Mass-variation samples (`*MT-171p5*` etc.) and
systematic samples (hdamp, tune) are added the same way with their own process names (`tch_mt1715`, ...).

## 3. Cross sections

`hell samples create` added keys with `xsec: null` to `samples/xsec.yaml`. Fill them with the value in pb,
the precision and the source (XSDB, the TOP PAG twiki, GenXSecAnalyzer); a null cross section is refused when
histograms are normalised. Mass and systematic variation samples take the nominal cross section.
```bash
hell samples xsec TbarBQtoLNu_t_channel_4FS        # shows the entry and the resolved value
```

## 4. File lists

The jobs need the list of files of every sample. They are cached in `samples/files/<era>/<sample>.txt`
(not in git); files present on `/eos/cms` are used directly, the others through xrootd:
```bash
hell samples files --era 2022                       # all samples of the era
hell samples files --era 2022 Muon_2022C --force    # one sample, refresh
```

## 5. Luminosity

```bash
hell lumi --era 2022                                        # Golden JSON, run range of the era
hell lumi --era 2022 --trigger HLT_IsoMu24 HLT_Ele30_WPTight_Gsf --update
```
brilcalc runs in the BRIL container from cvmfs (takes a minute per call). The result, with the command, the
JSON, the normtag and the per-trigger values (HLT path versions summed), is written to
`samples/lumi/2022.json`; `--update` puts the recorded luminosity into `samples/2022.yaml` with a pointer
to that file. For 2022 pre-EE: 8086.1 pb^-1 recorded.

## 6. Test the skim locally

```bash
hell run configs/SingleTopMass/skim_cfg.py sample=TbarBQtoLNu_t_channel_4FS era=2022 maxFiles=1 maxEvents=20000 outputFile=test_skim.root
hell run configs/SingleTopMass/skim_cfg.py sample=Muon_2022C era=2022 maxFiles=1 maxEvents=20000 outputFile=test_data.root
hell navigate test_skim.root                        # content, bookkeeping, cutflow, provenance
hell navigate test_data.root | grep -A3 golden      # the Golden JSON filter is in the path for data
```
What the skim keeps is defined in `configs/SingleTopMass/skim_cfg.py` (branches) and
`configs/SingleTopMass/objects_cff.py` (loose objects); gen-level distributions of all events, the `Runs` and
`LuminosityBlocks` trees, counts and provenance are always there. No corrections are applied in the skim.

## 7. Production on HTCondor

One task per era (or per sample group), ten files per job, outputs under a path that carries the version:
```bash
TAG=$(git describe --tags)                           # e.g. HELL_v1.0_pre1; never -dirty for a production
OUT=/eos/user/$USER/HELL/$TAG/skims/2022
hell submit run configs/SingleTopMass/skim_cfg.py --era 2022 --files-per-job 10 --output-dir $OUT --tag skim_2022_$TAG --proxy \
    --sample TbarBQtoLNu_t_channel_4FS TBbarQtoLNu_t_channel_4FS TTtoLNu2Q TTto2L2Nu TWminustoLNu2Q TbarWplustoLNu2Q \
             WtoLNu_2Jets DYto2L_2Jets_MLL_50 Muon_2022C Muon_2022D EGamma_2022C EGamma_2022D
```
What happens: the framework is packed, one job per chunk of files is defined, the Golden JSON of the era
is copied into the task and shipped with the data jobs (applied inside the job), `--proxy` ships your VOMS
proxy for xrootd inputs, and `condor_submit` is called (`--dry-run` to only prepare). The task lives in
`~/hell_condor/<tag>/` (AFS or local disk, `$HELL_CONDOR_WORK` to change it). Then:
```bash
hell status --tag skim_2022_$TAG                     # condor_q plus a check of the outputs
hell check --tag skim_2022_$TAG                      # opens every output, writes resubmit.txt
hell resubmit --tag skim_2022_$TAG                   # the failed ones
hell merge --tag skim_2022_$TAG                      # one file per sample in <output-dir>/merged/, counts checked
```
Outputs land in `<output-dir>/<sample>/<jobid>.root`; the downstream steps (`hell run analysis_cfg.py`,
`hell hist`) read either the job files or the merged ones.

## 8. Production on CRAB (alternative for the DAS datasets)

```bash
cd <CMSSW area>/src && cmsenv && source /cvmfs/cms.cern.ch/crab3/crab.sh && cd -
hell submit run configs/SingleTopMass/skim_cfg.py --backend crab --era 2022 --tag skim_2022_$TAG \
    --output-dir /store/user/$USER/HELL/$TAG/skims --files-per-job 5 --site T3_CH_CERNBOX \
    --sample TbarBQtoLNu_t_channel_4FS Muon_2022C
```
One CRAB task per sample is written to `crab/work/<tag>/crab_<sample>.py` and submitted (`--dry-run` to
only write them). CRAB applies the Golden JSON as the lumi mask for data, so the in-job filter is switched
off; the output files are `<sample>.root` per job under the CRAB output path. `crab status -d crab/work/<tag>/crab_<request>`
and `crab resubmit` are used as usual; the files are then bookkept and merged like the condor ones.

## 9. After the production: bookkeeping and the luminosity actually processed

```bash
hell bookkeep $OUT -o samples/bookkeeping/2022_$TAG.json
hell lumi --era 2022 --json samples/bookkeeping/2022_${TAG}_processed.json
```
`hell bookkeep` records per sample the files, the events in and out, `genEventSumw` (the normalisation), the
positive and negative weight sums, the runs, and the processed lumi sections; the data lumi sections are
merged into `..._processed.json`. `hell lumi --json` intersects them with the Golden JSON and gives the
luminosity of what was really processed: it must agree with step 5 if everything ran, and is the number to
use (`--update`) if part of the data is missing. Note that a truncated file (`maxEvents`) reports lumi
sections it only partly processed: production jobs never cut events.

## 10. Record it

Open an issue note for the production (`hell issue new "Skim production 2022 <tag>" --area samples`) with the
tag, the task names, the output paths, the bookkeeping JSON and the luminosity, and add the paths to the
release note of the tag (`docs/releases/notes/`).
