# HELL design

## Goals
Fast, robust, flexible, and fun. One framework for the whole analysis chain, mostly C++, with python only for
configuration and orchestration. Everything an analysis note needs (what was run, on what, with which code)
is recorded automatically.

## CMSSW to HELL dictionary
HELL follows the CMSSW way of working wherever it helps, so that the habits carry over.

### Working environment
| CMSSW | HELL | Notes |
|---|---|---|
| `cmsenv` | `source env/setup.sh` | loads the LCG view and HELL into the current shell (a program cannot do that, `cmsenv` is an alias too); `hell env` reports what is loaded |
| `scram b -j 8` | `hell build -j 8` | cmake + make, incremental; reconfigures by itself after a commit |
| `scram b clean` / fresh area | `hell build --clean` | |
| `scram tool info`, `echo $CMSSW_VERSION` | `hell env`, `hell version` | environment, build state, git HEAD vs built version |
| `BuildFile.xml` of a package | `Modules/<Package>/CMakeLists.txt`: `hell_add_plugin(Package [DEPENDS HellX])` | one line; `plugins/*.cc` are globbed like scram does, new files need no edit |
| `<use name="..."/>` | `DEPENDS HellCore HellML ...` | |
| `src/<Sub>/<Pkg>/{interface,src,plugins,python,test}` | `Core|DataFormats|Modules/<Pkg>/{interface,src,plugins}`, `python/hell`, `configs`, `test` | |
| `edmPluginDump` | `hell list-modules`, `hell navigate code` | with the one-line doc, file:line and parameters of each module |

### Configuration and running
| CMSSW | HELL | Notes |
|---|---|---|
| `cmsRun cfg.py key=value` | `hell run cfg.py key=value` | python config -> JSON -> `hell-run` |
| `cms.Process("NAME")` | `Process("NAME", ...)` | `python/hell/config.py` |
| `process.x = cms.EDProducer("Type", param=cms.double(1))` | `process.x = Module("Type", param=1.0)` | plain python values |
| `cms.Path`, `cms.Sequence` | `process.path = [...]`, python lists | a false filter stops the path |
| `_cfg.py`, `_cff.py` | same convention (`skim_cfg.py`, `objects_cff.py`) | fragments are plain imports |
| `VarParsing` | `hell.config.VarParsing` | built-in `maxEvents`, `inputFiles`, `outputFile`, `sample`, `era`, `variation` |
| `edmConfigDump` | `hell dump cfg.py ...`, `hell run ... --dump` | JSON, or resolved modules with parameters and products |
| `process.source.fileNames` | `inputFiles=`, or `sample=` resolved from `samples/<era>.yaml` | `@list.txt` and directories accepted |
| `process.maxEvents` | `maxEvents=`, `hell-run -n` | |
| `lumisToProcess` / `LumiList` | `GoldenJSONFilter` (or CRAB `lumiMask`) | can also be applied later, the lumi sections travel with the file |
| `PoolOutputModule` `outputCommands` keep/drop | `TreeWriter(keep=[...], drop=[...])` | globs on NanoAOD branch names |
| `NanoAODOutputModule` flat tables | `TreeWriter(products={label: Table})` + `FlatTable` persistence | `n<Table>`, `<Table>_<col>` with doc strings |
| `crab submit`, `multicrab.py` | `hell submit run --backend crab ...` | `hell submit` is also the HTCondor driver |
| `cmsRun` job report / `edmProvDump` | `hell/provenance` tree, `hell navigate file.root` | config, code version, inputs, products, parents |
| `edmDumpEventContent`, `edmFileUtil` | `hell navigate file.root [--branch 'Jet_*' --entries 3]` | |

### Framework
| CMSSW | HELL | Where |
|---|---|---|
| `EDProducer`, `EDFilter`, `EDAnalyzer` | `Producer`, `Filter`, `Analyzer` | `Core/interface/Module.h` |
| `edm::Event::put/get`, `edm::Handle` | `Event::put/get<T>(label)`, `getIfExists` | `Core/interface/Event.h` |
| `edm::InputTag("module", "instance")` | product label `"module"` or `"module:instance"` | `ModuleContext::produces<T>(instance)` |
| `consumes<T>()`, `produces<T>()` | `consumesArray/Value` (branches), `consumesProduct`, `produces<T>` | checked before the loop, like the CMSSW consumes validation |
| `edm::ParameterSet`, `getParameter`, `getUntrackedParameter` | `ParameterSet::get<T>(key[, default])` | unused parameters are reported (typos) |
| `DEFINE_FWK_MODULE(Type)` | `HELL_REGISTER_MODULE(Type, "doc")` | plugin `.so` dlopen'ed at start-up |
| `TFileService` | `HistService` / `HistDir` (`ctx.hists()`) | one directory per module |
| `EventSetup`, global tag | `JobContext` (data/MC, era, sample, variation) and the correction JSONs given to modules | |
| `beginJob/beginRun/endJob` | `beginJob/beginFile/endFile/endJob` | files instead of runs |
| `edm::MergeableCounterTable` in `Runs` | copied `Runs` tree + `hell/counts` | addable under hadd |
| `DataFormats/*` with dictionaries | `DataFormats/interface/*` with a ROOT dictionary | `Candidate`, `Jet`, `Muon`, `Electron`, `MET`, `GenParticle`, ... |
| `FWCore/MessageLogger` | `HELL_INFO/WARN/ERROR(ctx)` | every line starts with `[HELL]` |
| `edmPluginRefresh` | nothing to do | plugins are found in `build/lib/plugins` |

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
