# HELL documentation

| Where | What |
|---|---|
| [design.md](design.md) | Architecture: event loop, modules, products, data model, bookkeeping, provenance, systematics |
| [code-practices/](code-practices/README.md) | The rules we follow when writing HELL code |
| [tools/](tools/) | How to use each part of the framework |
| [python/hell/README.md](../python/hell/README.md) | Map of the python modules: what each does and an example |
| [methodology/](methodology/) | Analysis methods: normalisation, systematics, positive reweighting, fit strategy, ML strategy |
| [physics/](physics/) | Physics effects and object definitions relevant for the analyses (notes for the analysis note) |
| [issues/](issues/README.md) | Progress log and issues: one folder per topic with numbers, plots and reproduction commands |
| [releases/](releases/README.md) | Versions, branches and tags (the CMSSW release logic translated to HELL), release notes |

## Tools
- [configuration.md](tools/configuration.md): python configs, `Process`, `Module`, `VarParsing`, JSON
- [modules.md](tools/modules.md): writing a C++ module (producer / filter / analyzer), products, branches, histograms
- [skimming.md](tools/skimming.md): skims with `TreeWriter`, what is kept, CRAB and condor
- [histogramming.md](tools/histogramming.md): `hell hist` YAML: regions, variables, processes, colours, lumi, systematics
- [fitting.md](tools/fitting.md): datacards, parametric workspaces, running Combine
- [mltools.md](tools/mltools.md): training, validation and applying PyTorch models in the C++ loop
- [batch.md](tools/batch.md): HTCondor and CRAB submission, check, resubmit, merge
- [samples.md](tools/samples.md): sample registry, cross sections, file lists, bookkeeping
- [plotting.md](tools/plotting.md): CMS TDR style (C++ and PyROOT), data/MC stacks
- [nanoaod_versions.md](tools/nanoaod_versions.md): selections aligned to the NanoAOD version of the input
- [corrections.md](tools/corrections.md): central POG corrections through correctionlib (working points, JES/JER, pileup, scale factors)
- [validation.md](tools/validation.md): data/MC agreement metrics of objects and scale factors
- [navigator.md](tools/navigator.md): `hell navigate` for files, code and the data model
- [provenance.md](tools/provenance.md): what every output file records and how to read it back

## Methodology
- [normalisation.md](methodology/normalisation.md): lumi x xsec / sum of weights, where each number comes from
- [systematics.md](methodology/systematics.md): weight vs object variations, how they flow to the fit
- [positive_reweighting.md](methodology/positive_reweighting.md): removing negative generator weights
- [single_top_mass.md](methodology/single_top_mass.md): the SingleTopMass analysis strategy and status

## Physics
- [objects.md](physics/objects.md): lepton, jet, b-tag, MET definitions used and why
- [top_reconstruction.md](physics/top_reconstruction.md): neutrino pz, top candidate, mass observables
