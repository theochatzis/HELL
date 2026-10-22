<p align="center">
  <img src="docs/images/Fiery Likelihood Lab HELL Logo.png" alt="HARP" width="100%">
</p>

# HELL - High Energy Likelihood Laboratory

*We forge the analysis in HELL.*

HELL is a C++ analysis framework for the CMS experiment working on NanoAOD-like ntuples (central NanoAOD or
privately produced). It covers the full chain of an analysis: skimming, analysis trees, histogramming with
systematic variations, machine learning (PyTorch), fits (RooFit workspaces and Higgs Combine datacards) and
validation. The physics runs in modular C++ with its own CMSSW-like data model and event loop; python wraps
sequences of modules into job configurations and drives batch processing, ML training and fits.

The first analysis built on it is **SingleTopMass**: the top quark mass in single-top t-channel events
(`configs/SingleTopMass`, `analyses/SingleTopMass`).

```
[HELL] Welcome to HELL.
hell run configs/SingleTopMass/skim_cfg.py sample=TbarBQtoLNu_tch era=2022 maxEvents=10000
hell hist configs/SingleTopMass/hist.yaml
hell unleash-systematics configs/SingleTopMass/analysis_cfg.py --local inputFiles=skim.root era=2022
hell fit configs/SingleTopMass/fit.yaml --strategy 2
hell navigate skim.root
hell pray
```

## Contents
- [Setup and build](#setup-and-build)
- [Five-minute tour](#five-minute-tour)
- [How it works](#how-it-works)
- [Directory layout](#directory-layout)
- [Documentation](#documentation)
- [General Notes (useful links)](#general-notes)

## Setup and build
On lxplus (EL9). The environment is an LCG view from cvmfs (ROOT 6.36, gcc 13, PyTorch, uproot, correctionlib).
```bash
git clone <this repository> HELL
cd HELL
source env/setup.sh              # HELL_ENV=lcg (default) or HELL_ENV=native
mkdir -p build && cd build
cmake .. && make -j8
cd ..
hell version                     # python + C++ versions
hell pray                        # end-to-end quick test (needs /eos/cms, ~3 minutes)
```
Always `source env/setup.sh` in a new shell (it sets `HELL_BASE`, the plugin path, `PATH`, `PYTHONPATH`).
Re-run `cmake ..` after a `git commit` so that the git hash written into the output files (provenance) is current.

For CRAB and remote files: `source /cvmfs/cms.cern.ch/crab3/crab.sh; voms-proxy-init --voms cms` inside a CMSSW area.
For Combine: set `HELL_COMBINE_SETUP` to a script that puts `combine` on the PATH (standalone Combine or a CMSSW area).

## Five-minute tour
```bash
# Samples: registry in samples/2022.yaml, cross sections in samples/xsec.yaml
hell samples list --era 2022
hell samples files --era 2022 TbarBQtoLNu_tch         # file list from DAS (cached in samples/files/)

# 1. Skim NanoAOD (fast, no corrections): loose selection, kept branches, Runs/LuminosityBlocks copied,
#    gen distributions of all events, bookkeeping and provenance in the file
hell run configs/SingleTopMass/skim_cfg.py sample=TbarBQtoLNu_tch era=2022 maxEvents=20000 outputFile=skim.root
hell navigate skim.root                                # what is in it, counts, cutflow, provenance chain

# 2. Analysis tree: objects, top reconstruction, variables, weights (+variations), one job per object variation
hell run configs/SingleTopMass/analysis_cfg.py inputFiles=skim.root era=2022 sample=TbarBQtoLNu_tch outputFile=trees/2022/TbarBQtoLNu_tch/t.root
hell unleash-systematics configs/SingleTopMass/analysis_cfg.py --local inputFiles=skim.root era=2022 sample=TbarBQtoLNu_tch

# 3. Histograms: cuts, variables, processes/colours, lumi and systematics from a YAML
hell hist configs/SingleTopMass/hist.yaml -o hists.root
hell plot hists.root -o plots

# 3b. Validation: data/MC agreement of objects and scale factors (yield ratio, chi2, KS, pulls, with/without SF)
hell validate configs/SingleTopMass/validation.yaml

# 4. Fit: Combine datacards (templates) or a RooFit workspace with an mtop-dependent signal (parametric)
hell fit configs/SingleTopMass/fit.yaml --datacards
hell fit configs/SingleTopMass/fit.yaml --parametric --strategy 2

# 5. ML: PyTorch classifier/regressor with validation, applied in the C++ loop afterwards
hell train configs/SingleTopMass/ml_classifier.yaml
hell run configs/SingleTopMass/analysis_cfg.py inputFiles=skim.root era=2022 model=models/st_classifier.json

# Batch (HTCondor; CRAB for skims over DAS datasets)
hell submit run configs/SingleTopMass/skim_cfg.py --sample TbarBQtoLNu_tch TTtoLNu2Q --era 2022 --files-per-job 10 \
     --output-dir /eos/user/$USER/HELL/skims/2022 --tag skim_v1
hell check --tag skim_v1; hell resubmit --tag skim_v1; hell merge --tag skim_v1
hell bookkeep /eos/user/$USER/HELL/skims/2022 -o samples/bookkeeping/2022.json
```
Every command and every executable answers `--help` with the options **and an example**.
Config files answer `hell run cfg.py help` with their options.

## How it works
- A **job** is a python config that builds a `Process`: input files, output file, job context (data/MC, era,
  sample, active systematic variation) and an ordered **path** of **modules**. The python is turned into JSON and
  executed by the C++ event loop `hell-run` (`Core/interface/Forge.h`).
- **Modules** are C++ plugins of three kinds, as in CMSSW: `Producer` (puts products into the event), `Filter`
  (stops the path for the event) and `Analyzer` (histograms, trees). They exchange products by label through
  the `Event` and declare what they read (`consumesArray/Value`, `consumesProduct`) and produce (`produces<T>`).
- The **data model** (`DataFormats/`) is ours: `hell::Jet`, `hell::Muon` and `hell::Electron` (sharing the base `hell::Lepton`), `hell::MET`, `hell::GenParticle`,
  `hell::EventInfo`, `hell::Weights`, `hell::TopCandidate`. Only the `Nano*Reader` producers touch NanoAOD
  branch names; branch types are converted automatically (NanoAOD changes them between versions).
- Every output file carries its **bookkeeping** (`hell/counts`, `hell/cutflow`, copied `Runs` and
  `LuminosityBlocks` trees) and its **provenance** (`hell/provenance`: config, code version, inputs, products and
  the provenance of the parents). Both survive `hadd` / `hell merge`.
- **Systematics**: weight variations are branches (`Weight_<name>Up/Down`); object variations (JES, JER) are one
  job per variation (`variation=jesUp`), produced by `hell unleash-systematics`. The histogrammer and the
  datacards know both kinds.
- **Normalisation**: lumi x xsec / sum(genEventSumw), with the sum read from the files (all generated events,
  taken from the NanoAOD Runs tree), the cross section from `samples/xsec.yaml` and the lumi from `samples/<era>.yaml`.

## Directory layout
| Directory | Content |
|---|---|
| `Core/` | framework kernel: event loop (Forge), Event, modules, branch readers, bookkeeping, provenance, output trees |
| `DataFormats/` | the HELL data model and its persistence (flat tables) and ROOT dictionary |
| `Modules/<Package>/plugins/` | module plugins: Nano readers, Selection, Weights, Top, Output, Analyzers, ML, Systematics |
| `Hist/`, `Fit/` | histogrammer library (`hell-hist`), datacard/workspace writers (`hell-datacard`) |
| `bin/` | `hell` CLI and the C++ executables' sources |
| `python/hell/` | config, samples, runner, histcfg, plot, merge, bookkeeping, batch (condor, crab), ml, fit, navigator, cli |
| `configs/SingleTopMass/` | the analysis configurations (objects, skim, analysis tree, hist, fit, ML) |
| `analyses/SingleTopMass/` | analysis-specific notes, studies and scripts |
| `samples/` | sample registry per era, cross sections, cached file lists |
| `condor/`, `crab/` | batch templates and work areas |
| `docs/` | documentation: design, code practices, tools, methodology, physics, issues |
| `test/` | `run_quick_test.sh` (`hell pray`) and test configurations |

## Documentation
Start at [docs/README.md](docs/README.md). In particular:
[design](docs/design.md), [code practices](docs/code-practices/README.md), [writing a module](docs/tools/modules.md),
[configuration](docs/tools/configuration.md), [skimming](docs/tools/skimming.md), [histogramming](docs/tools/histogramming.md),
[systematics](docs/methodology/systematics.md), [normalisation and bookkeeping](docs/methodology/normalisation.md),
[fitting](docs/tools/fitting.md), [ML tools](docs/tools/mltools.md), [batch](docs/tools/batch.md), [navigator](docs/tools/navigator.md),
[issues and progress](docs/issues/README.md).

## General Notes

### Finding the available samples
You should always use the recommended samples (see useful links below).
To find samples you can use [DAS](https://cmsweb.cern.ch/das/) or inside CMSSW the `dasgoclient` command.
UL2018 MC, example for signal:
```
dasgoclient -query="dataset=/ST*t-channel*/*106X_upgrade2018_realistic_v16_L1v1*/NANOAOD*"
```
you can replace the ST_t-channel for any process you need. The `106X_upgrade2018_realistic_v16_L1v1` is the [recommended global tag](https://twiki.cern.ch/twiki/bin/viewauth/CMS/PdmVRun2LegacyAnalysis)

UL2018 Data, Single Muon: `dataset=/*SingleMuon*/*UL*2018*/NANOAOD`; EGamma: `dataset=/*EGamma*/*UL*2018*/NANOAOD`.
In HELL: `hell samples das '/TbarBQtoLNu-t-channel*/Run3Summer22NanoAODv12*/NANOAODSIM'`.

### Useful links
- [Twiki : Recommendations of analyses for Run2](https://twiki.cern.ch/twiki/bin/viewauth/CMS/PdmVRun2LegacyAnalysis)
- [Twiki : Recommendations of analyses for Run3](https://twiki.cern.ch/twiki/bin/view/CMS/PdmVRun3Analysis)
- [Analysis corrections central documentation](https://cms-analysis-corrections.docs.cern.ch/)
- Event info:
  - [Workbook NanoAOD](https://twiki.cern.ch/twiki/bin/view/CMSPublic/WorkBookNanoAOD): Useful for checking quickly the collections
  - [NanoAOD Infos](https://cms-nanoaod-integration.web.cern.ch/)
- [Central Analysis Tools CMS - CAT docs site](https://cms-analysis.docs.cern.ch/guidelines/frameworks/frameworks/)
- [cms-nanoAOD Gitlab repositories](https://gitlab.cern.ch/cms-nanoAOD) : Useful for POGs corrections JSONs and handling them w/ `correctionlib`
  - Space with JSONs from POGs: `/cvmfs/cms.cern.ch/rsync/cms-nanoAOD/jsonpog-integration/POG/[BTV/EGM/JME/LUM/MUO/TAU]`
  - For examples of implementations you can check `/cvmfs/cms.cern.ch/rsync/cms-nanoAOD/jsonpog-integration/POG/examples`
  - [Link with JSONs content per POG](https://cms-nanoaod-integration.web.cern.ch/commonJSONSFs/)
- [Good lumisections Twiki](https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuideGoodLumiSectionsJSONFile#Introduction)
  - Latest recommendation (double check with the link above):
    - Run 3 data: on lxplus at `/eos/user/c/cmsdqm/www/CAF/certification/`
    - 2018 data: `/afs/cern.ch/cms/CAF/CMSCOMM/COMM_DQM/certification/Collisions18/13TeV/Legacy_2018/Cert_314472-325175_13TeV_Legacy2018_Collisions18_JSON.txt`
    - 2017 data: `/afs/cern.ch/cms/CAF/CMSCOMM/COMM_DQM/certification/Collisions17/13TeV/Legacy_2017/Cert_294927-306462_13TeV_UL2017_Collisions17_GoldenJSON.txt`
    - 2016 data: `/afs/cern.ch/cms/CAF/CMSCOMM/COMM_DQM/certification/Collisions16/13TeV/Legacy_2016/Cert_271036-284044_13TeV_Legacy2016_Collisions16_JSON.txt`
  - [JSON files from Lumisections Certification](https://cms-service-dqmdc.web.cern.ch/CAF/certification/)
- [Higgs combine official site](https://cms-analysis.github.io/HiggsAnalysis-CombinedLimit/#within-cmssw-recommended-for-cms-users)
- [TopMassAndProperties](https://twiki.cern.ch/twiki/bin/view/CMS/TOPMassProperties)
- [TopModelingGenerators](https://twiki.cern.ch/twiki/bin/viewauth/CMS/TopModGen) : has link to request new samples
- [XSEC DB Tool Github](https://github.com/cms-sw/xsecdb/tree/master/scripts)

### Python in HEP
- [HSF training page](https://hepsoftwarefoundation.org/activities/training.html)
- [scikit hep - HSF](https://hsf-training.github.io/hsf-training-scikit-hep-webpage/aio/index.html)
  - [iminuit](https://github.com/scikit-hep/iminuit) : Minimization using Minuit with Python.
- [RDataframe analysis tutorials](https://root.cern.ch/doc/master/group__tutorial__dataframe.html)
- [CAT site](https://cms-analysis.docs.cern.ch/)
- [cool commenting ASCII art](https://patorjk.com/software/taag/#p=display&f=Doom&t=Muon)

### Objects stuff
General:
- [CMS Physics Objects Intro](https://cms-opendata-workshop.github.io/workshop2024-lesson-physics-objects/): Very basic description of physics objects - including also what NanoAOD branches are available.

Top:
- [Top pT reweighting](https://twiki.cern.ch/twiki/bin/view/CMS/TopPtReweighting)

Trigger:
- [Trigger coordination website](https://cms-info.web.cern.ch/coordination/trigger-coordination/); list of triggers (Run2) [here](https://twiki.cern.ch/twiki/bin/viewauth/CMS/HLTPathsRunIIList#2018)
- [CMS HLT info : paths](https://cmshltinfo-dev.app.cern.ch/summary?search=Ele&year=2022&paths=true&prescaled=false&stream-types=Physics,Scouting,Parking) : Tool for finding paths in different run periods.
- [CMS HLT Info : menus](https://cmshltinfo-dev.app.cern.ch/menus#iss=https%3A%2F%2Fauth.cern.ch%2Fauth%2Frealms%2Fcern) : Tool for finding menus in different run periods.
- [ConfDB browser online](https://hlt-config-editor-confdbv3.app.cern.ch/#iss=https%3A%2F%2Fauth.cern.ch%2Fauth%2Frealms%2Fcern&iss=https%3A%2F%2Fauth.cern.ch%2Fauth%2Frealms%2Fcern) : configuration database (confDB), content of HLT menus.

JetsMET:
- [JetMET Twiki](https://twiki.cern.ch/twiki/bin/view/CMS/JetMET)
- [Twiki : JEC Uncertainties Data/MC](https://twiki.cern.ch/twiki/bin/view/CMS/JECDataMC)
- [AK4PFPuppi_cff for Nano](https://github.com/cms-sw/cmssw/blob/4c67638e0cf38f6cd61f701325865d9448a1f0a6/PhysicsTools/NanoAOD/python/jetsAK4_Puppi_cff.py)
- [Type1 Correction MET CMSSW code](https://github.com/cms-sw/cmssw/blob/2f2cacbe9c25c8cebc88c2578207453555a974d9/PhysicsTools/NanoAOD/python/jetsAK4_Puppi_cff.py#L197C1-L210C2)
- [JERC site](https://cms-jerc.web.cern.ch/)
- CMS-Talk posts relevant for b-jet regression:
  - [Twiki : Run2 B-jet energy regression](https://twiki.cern.ch/twiki/bin/view/CMS/HiggsWG/BJetRegression)
  - https://cms-talk.web.cern.ch/t/indico-event-reminder-jet-energy-resolution-and-corrections-meeting-01-10-2024-14-00-europe-zurich/51199
  - https://cms-talk.web.cern.ch/t/calibration-of-post-regression-b-jet-pt-in-hig-24-010/49924/3
  - [MC truth PNet regression](https://indico.cern.ch/event/1450094/contributions/6104603/attachments/2920379/5125887/2024_09_03_PNetRegCalibration-MCTruth-pT15Gev.pdf)
  - [MET significance intro](https://twiki.cern.ch/twiki/pub/CMSPublic//WorkBookMetAnalysis/tai_20150119_met_significance.pdf)

BTV: [BTV wiki](https://btv-wiki.docs.cern.ch/)

EGamma:
- [Twiki : EGamma](https://twiki.cern.ch/twiki/bin/view/CMS/EgammaPOG), [Github page](https://cms-egamma.github.io/)
- [EGamma trigger DPNote](https://cds.cern.ch/record/2855389)
- [CMSDAS CERN 2020 EGamma Objects (Short Exercise) Twiki](https://twiki.cern.ch/twiki/bin/view/CMS/SWGuideCMSDataAnalysisSchoolCERN2020EgammaShortExercise)

Muon:
- [Twiki : Muon](https://twiki.cern.ch/twiki/bin/view/CMS/MuonPOG), [Muon Wiki Docs](https://muon-wiki.docs.cern.ch/)
- [Muon trigger DPNote](https://cds.cern.ch/record/2888302/files/DP2024_005.pdf?version=1)
- [Muon SFs](https://muon-wiki.docs.cern.ch/guidelines/corrections/#medium-pt-prompt-muons)

## Acknowledgements
Design inspired by CMSSW and by the author's earlier frameworks HARP (CMSSW-like C++ with python configs)
and Reco2Pico / SkimRDFAnalysisBase (bookkeeping, provenance strings, navigator) and TopNanoAnalysis.
The framework is developed with the help of Claude (Anthropic) through Claude Code: code, documentation
and tests are written together with it and reviewed by the author.
