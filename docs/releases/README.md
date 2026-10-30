# HELL releases

HELL follows the CMSSW release logic, translated to a framework that does not live inside CMSSW:
development of a version happens on its own branch, exact versions are tags, and `main` always holds the
latest released code. How branches, tags and porting work is in [branching.md](branching.md); one note per
release is in [notes/](notes/).

## Correspondence with CMSSW

| CMSSW | HELL | meaning |
|---|---|---|
| `master` | `main` | the reference: latest release, where releases are merged |
| `CMSSW_<maj>_<min>_X` branch | `HELL_v<maj>.<min>` branch | one version cycle: all development of that version |
| `CMSSW_<maj>_<min>_0_pre<N>` tag | `HELL_v<maj>.<min>_pre<N>` tag | a pre-release of a cycle under development |
| `CMSSW_<maj>_<min>_<patch>` tag | `HELL_v<maj>.<min>[.<patch>]` tag | a release (patch number only for fixes on a released version) |
| `SCRAM_ARCH`, external tools | `HELL_LCG_VIEW` (LCG view from cvmfs) | compiler, ROOT, python stack the release was built and tested with |
| Global tag, era | `samples/<era>.yaml`, `samples/certification.yaml`, `configs/<analysis>/objects_cff.py` | conditions: lumi, Golden JSON, cross sections, working points |
| release notes on cms-sw | `docs/releases/notes/<tag>.md` | what changed, validation, productions |
| `edmProvDump` | `hell navigate file.root` | which exact version produced a file (`git describe`, baked in at build) |

## Supported lines

| line | branch | status | LCG view (ROOT, python) | NanoAOD / eras tested | latest tag |
|---|---|---|---|---|---|
| released | `main` | reference | LCG_108 x86_64-el9-gcc13-opt (ROOT 6.36, python 3.12, torch 2.7) | Run3Summer22 NanoAODv12, 2022 data (22Sep2023) | [HELL_v0.0](notes/HELL_v0.0.md) |
| in development | `HELL_v1.0` | open | same | same; central corrections (JES/JER, pileup, lepton and b-tag SFs) from the POG JSONs | none yet |

## Feature support

Status at the latest tag:

- **used**: exercised end to end on real NanoAOD (`hell pray`) or in a production;
- **builds**: compiles and its configuration loads, not yet exercised on events;
- **planned**: on the open items list, not implemented.

| feature | HELL_v0.0 |
|---|---|
| Core event loop, products, plugins, lazy typed branch readers, bookkeeping, provenance | used |
| Data model (Jet, Muon, Electron, MET, GenParticle, EventInfo, Weights, TopCandidate) with dictionary | used |
| Skim (`TreeWriter`, gen distributions, Runs/LuminosityBlocks copy, Golden JSON) | used |
| Analysis tree (selections, manual jet id, b-tag WPs, weights with variations, top reconstruction, `st_*` variables) | used |
| Object-level systematics (`JetVariation`, one job per variation, `hell unleash-systematics`) | used with placeholder factors |
| Histogrammer `hell-hist` (weight and tree systematics, normalisation from the files) | used |
| Datacards (`hell-datacard`: templates, parametric mtop model) | used; combine not run (not in the LCG view) |
| ML tools (PyTorch MLP, validation, JSON export, `MLPScoreProducer`) | used |
| Object validation (`ObjectValidationAnalyzer`, `hell validate`) | used; scale factors all 1 |
| Plots in CMS TDR style (`Plot/`, `hell plot`) | used |
| Samples (`hell samples create` from DAS, cross sections, file lists), `hell lumi` (brilcalc), `hell bookkeep` | used (DAS path tested up to the proxy) |
| HTCondor (`hell submit/check/resubmit/merge`) | builds (dry-run tasks only) |
| CRAB (`hell submit --backend crab`) | builds (dry-run only) |
| Navigators (file, code graph, model), `hell build`, `hell env`, `hell pray` | used |
| JES/JER sources and scale factors through correctionlib, positive reweighting, gen-level neutrino, parametric fit shapes, `hell-hist` threading | planned |

When a release changes a status, update this table in the same commit as its release note.

## Release notes

Newest first. One file per tag in [notes/](notes/), named after the tag.

| tag | date | branch | summary |
|---|---|---|---|
| [HELL_v0.0](notes/HELL_v0.0.md) | 2026-10-29 | `main` | the skeleton: full chain working end to end on Run 3 NanoAOD |

## Adding a release

1. On the version branch: build, run `hell pray`, update the docs and the HELL-NNNN issue notes.
2. Copy [notes/TEMPLATE.md](notes/TEMPLATE.md) to `notes/<tag>.md` and fill it in.
3. Update the tables above (supported lines, feature support, release notes).
4. Commit, tag, merge into `main` and push (commands in [branching.md](branching.md#making-a-release)).
