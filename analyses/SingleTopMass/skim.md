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

## 1. The era, its NanoAOD version and its corrections

Three files pin down what an era means; check them before anything else:
- `samples/certification.yaml`: Golden JSON, run range (2022 = runs 355100 to 359021, 2022EE = 359022 to
  362760, one file), normtag, BRIL environment. Compare with
  https://cms-service-dqmdc.web.cern.ch/CAF/certification/ and the TOP recommendations before a production.
- `samples/corrections.yaml`: the POG correctionlib JSONs of the era and the names used (JEC/JER tags, b-tag
  working points and scale factors, jet id, veto map, pileup, muon and electron scale factors).
  `hell corrections --era 2022` prints and checks them; `--list jerc --grep Regrouped` shows a JSON's content.
- `samples/nanoaod.yaml`: the NanoAOD versions and what differs between them (jet id source, `MET_` vs
  `PFMET_`, electron supercluster eta, tagger branch). Every sample declares its version (`nanoVersion`, taken
  from the dataset name by `hell samples create`); every job checks the declared version against the branches
  of its input and refuses a mismatch. If the campaign moves to a new NanoAOD version (v12 to v15 for
  example), add the version there if it is missing, then recreate the sample entries: the selections follow
  the version automatically (central jet id and `PFMET` for v15), nothing in the configs changes.
  `hell navigate <file>` prints the version it detects in a file.

A new era (2023, 2024) is therefore: an entry in each of the three files, then the steps below.


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
hell navigate test_skim.root                        # content, bookkeeping, cutflow, provenance, NanoAOD version
hell navigate test_data.root | grep -A3 golden      # the Golden JSON filter is in the path for data
```
The log starts with `input matches NanoAOD v12` (the declared version against the file) and, for v12, `jet id
from the manual recipe`; a file of another version is refused with the list of branches that do not fit.
For a quick look at a single file of a version not in the registry: `hell run ... inputFiles=<file> era=2022EE nanoVersion=15`.
What the skim keeps is defined in `configs/SingleTopMass/skim_cfg.py` (branches) and
`configs/SingleTopMass/objects_cff.py` (loose objects); gen-level distributions of all events, the `Runs` and
`LuminosityBlocks` trees, counts and provenance are always there. No corrections are applied in the skim.

## 7. Production on CRAB (recommended for the central datasets)

CRAB runs at the sites holding the data, scales to full datasets, applies the Golden JSON as the lumi mask
for data, retries failures and reports the lumi sections it processed. It needs a CMSSW area only for the
client (HELL itself runs from the LCG view inside the job):
```bash
cd <CMSSW area>/src && cmsenv && source /cvmfs/cms.cern.ch/crab3/crab.sh && cd -
TAG=$(git describe --tags)                           # e.g. HELL_v1.0_pre1; never -dirty for a production
hell submit run configs/SingleTopMass/skim_cfg.py --backend crab --era 2022 --tag skim_2022_$TAG \
    --output-dir /store/user/$USER/HELL/$TAG/skims --files-per-job 5 --site T3_CH_CERNBOX \
    --sample TbarBQtoLNu_t_channel_4FS TBbarQtoLNu_t_channel_4FS TTtoLNu2Q TTto2L2Nu TWminustoLNu2Q TbarWplustoLNu2Q \
             WtoLNu_2Jets DYto2L_2Jets_MLL_50 Muon_2022C Muon_2022D EGamma_2022C EGamma_2022D
```
One CRAB task per sample is written to `crab/work/<tag>/crab_<sample>.py` and submitted (`--dry-run` to
only write them). The in-job Golden JSON filter is switched off for CRAB since the lumi mask does the
selection. Follow up with the CRAB client:
```bash
crab status -d crab/work/skim_2022_$TAG/crab_skim_2022_${TAG}_Muon_2022C
crab resubmit -d ...                                 # failed jobs
crab report -d ...                                   # processed lumi sections (processedLumis.json) for step 9
```
The outputs are `<sample>.root` per job under `/store/user/$USER/HELL/$TAG/skims/...` on the storage site
(T3_CH_CERNBOX = `/eos/user/<u>/<user>/`), which is the input of step 9 and of the analysis step.

## 8. Production on HTCondor (private or local inputs, and everything after the skim)

Condor runs on CERN resources with the files you give it, needs no CMSSW, starts in minutes and is what the
analysis trees, histograms and trainings use. For skims it is the choice for private NanoAOD or files already
on EOS:
```bash
OUT=/eos/user/$USER/HELL/$TAG/skims/2022
hell submit run configs/SingleTopMass/skim_cfg.py --era 2022 --files-per-job 10 --output-dir $OUT --tag skim_2022_$TAG --proxy \
    --sample TbarBQtoLNu_t_channel_4FS Muon_2022C
hell status --tag skim_2022_$TAG                     # condor_q plus a check of the outputs
hell check --tag skim_2022_$TAG                      # opens every output, writes resubmit.txt
hell resubmit --tag skim_2022_$TAG                   # the failed ones
hell merge --tag skim_2022_$TAG                      # one file per sample in <output-dir>/merged/, counts checked
```
The framework is packed, one job per chunk of files is defined, the Golden JSON of the era is shipped with
the data jobs and applied inside them, `--proxy` ships your VOMS proxy for xrootd inputs. The task lives in
`~/hell_condor/<tag>/` (AFS or local disk, `$HELL_CONDOR_WORK` to change it). Outputs land in
`<output-dir>/<sample>/<jobid>.root`.

## 9. After the production: bookkeeping and the luminosity actually processed

```bash
hell bookkeep $OUT -o samples/bookkeeping/2022_$TAG.json
hell lumi --era 2022 --json samples/bookkeeping/2022_${TAG}_processed.json
```
`hell bookkeep` records per sample the files, the events in and out, `genEventSumw` (the normalisation), the
positive and negative weight sums, the runs, and the processed lumi sections; the data lumi sections are
merged into `..._processed.json` (for CRAB productions `crab report` gives the same information). `hell lumi
--json` intersects them with the Golden JSON and gives the luminosity of what was really processed: it must
agree with step 5 if everything ran, and is the number to use (`--update`) if part of the data is missing. Note that a truncated file (`maxEvents`) reports lumi
sections it only partly processed: production jobs never cut events.

## 10. Record it

Open an issue note for the production (`hell issue new "Skim production 2022 <tag>" --area samples`) with the
tag, the task names, the output paths, the bookkeeping JSON and the luminosity, and add the paths to the
release note of the tag (`docs/releases/notes/`).
