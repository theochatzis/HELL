# HELL design

## Goals
Fast, robust, flexible, and fun. One framework for the whole analysis chain, mostly C++, with python only for
configuration and orchestration. Everything an analysis note needs (what was run, on what, with which code)
is recorded automatically.

## Concepts (CMSSW analogy)
| CMSSW | HELL | Where |
|---|---|---|
| `cmsRun cfg.py` | `hell run cfg.py` -> JSON -> `hell-run` | `python/hell/runner.py`, `bin/hell-run.cc` |
| `cms.Process`, `cms.Path` | `hell.config.Process`, `process.path` | `python/hell/config.py` |
| `EDProducer`, `EDFilter`, `EDAnalyzer` | `Producer`, `Filter`, `Analyzer` | `Core/interface/Module.h` |
| `edm::Event::put/get` | `Event::put/get<T>(label)` | `Core/interface/Event.h` |
| `consumes<T>()`, `produces<T>()` | `ModuleContext::consumesArray/Value`, `consumesProduct`, `produces<T>` | `Core/interface/Module.h` |
| `ParameterSet` | `ParameterSet` (nlohmann::json) | `Core/interface/ParameterSet.h` |
| `DEFINE_FWK_MODULE` | `HELL_REGISTER_MODULE(type, doc)` + plugin `.so` | `Core/interface/ModuleFactory.h` |
| `TFileService` | `HistService` / `HistDir` | `Core/interface/HistService.h` |
| `EventSetup` / job info | `JobContext` (data/MC, era, sample, variation) | `Core/interface/JobContext.h` |
| framework job report | `hell/counts`, `hell/cutflow`, `hell/provenance` | `Core/interface/Bookkeeper.h`, `Provenance.h` |

## Event loop (Forge)
`Forge::initialize()` loads the plugins, opens the input chain (bad files are skipped and recorded), creates the
output file, builds the modules of the path in order (each with its own `ModuleContext`), validates that every
consumed product is produced earlier, and books the cutflow. `Forge::run()` loops over the entries: for each one
the registry is told the new entry, the event is cleared, the framework reads run/lumi/event/genWeight for the
bookkeeping, and the modules run in order. A filter returning false ends the event. On every file change the
branch readers are re-bound, output trees are re-pointed, the Runs/LuminosityBlocks trees of the new file are
copied and its provenance records harvested.

## Reading branches
Branches are read lazily through `BranchRegistry` (`Core/interface/BranchRegistry.h`): a branch is read from
disk the first time a module touches it in an entry, so early filters avoid reading heavy collections. The
on-disk type is discovered at run time and converted to the requested type (NanoAOD changes `UChar_t`/`Int_t`/
`Short_t` between versions). Only the registered branches are enabled on the chain.

## Products and the data model
Products are owned by the `Event` and exchanged by label (`"jets"`, or `"goodMuons:veto"` for a module with
several products). The data model in `DataFormats/interface/` is a set of plain structs with ROOT dictionaries. Muons and
electrons are distinct types (`Muon`, `Electron`) with their own identification members; they share the base
`Lepton`, and the merged `LeptonCollection` of selected leptons holds that base part with `index` pointing
back to the flavour collection.
Anything with a persistence converter (`DataFormats/src/Persistence.cc`) can be written to a tree as a flat
table: `n<Table>`, `<Table>_<column>`, with a doc string per column that becomes the branch title.

## Output files
Every file produced by `hell-run`, `hell-hist` contains a `hell/` directory:
- `counts`: labelled bins, hadd-addable (events in/out, sums of weights, genEventCount/Sumw/Sumw2, files).
- `cutflow`, `cutflow_w`: per module of the path.
- `provenance`: TTree with one JSON record per job (configuration, software version and git hash, command line,
  inputs, products, bookkeeping) including the records of the parent files. Concatenates under hadd.
`hell-run` also copies the `Runs` and `LuminosityBlocks` trees of the inputs.

## Systematics
Two kinds, see [methodology/systematics.md](methodology/systematics.md): weight variations are columns of the
`Weights` product; object variations are separate jobs (`JobContext::variation`) in which the variation
producers (`JetVariation`) apply their factor, everything else unchanged.

## Why not RDataFrame
The skim must be a plain, fast, predictable copy of branches with bookkeeping; the analysis needs stateful
objects (top candidates, ML scores, variations) that are natural in a module loop and awkward as lambdas. Flat
analysis trees written by HELL are however perfectly usable with RDataFrame for quick studies.

## Naming
Namespace `hell`. Libraries `libHell<Package>`, plugins `libHell<Package>Plugin.so`, executables `hell-<verb>`.
Module labels in configs are lower camel case (`goodJets`); products carry the module label. Histogram and
branch names are stable identifiers: add, do not rename.
