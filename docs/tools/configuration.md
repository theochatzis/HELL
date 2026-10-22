# Configuration

A HELL job is described by a python file that defines a `process` (class `hell.config.Process`). `hell run`
executes it with the `key=value` options given on the command line, writes the JSON to `work/<cfg>[_tag].json`
and runs `hell-run` on it. The JSON is the actual input of the C++ program and is stored in the output file
(provenance), so any job can be reproduced from its output.

```python
from hell.config import Process, Module, VarParsing
from hell.runner import apply_options

options = VarParsing()                       # maxEvents, inputFiles, outputFile, sample, era, variation, ... are built in
options.register("minJets", 2, int, "minimum number of selected jets")
options.parse()

process = Process("ANALYSIS", output="analysis.root")
apply_options(process, options)              # files, isData, era, xsec from samples/<era>.yaml when sample= is given

process.jets = Module("NanoJetReader", btagger="btagPNetB")       # label = attribute name
process.goodJets = Module("JetSelector", src="jets", minPt=30)
process.nJets = Module("CollectionSizeFilter", src="goodJets", min=options.minJets)
process.path = ["jets", "goodJets", "nJets"]
```
- `Module(type, **params)`: the type is a C++ module type (`hell list-modules`); the parameters are what its
  constructor reads. Unknown parameters produce a warning at run time (typos).
- `module.clone(**overrides)` makes variants; `process.add_to_path(...)` appends to the path.
- The config directory is on `sys.path`, so shared fragments (`objects_cff.py`) are plain imports.
- `hell run cfg.py help` prints the options; `hell dump cfg.py ...` prints the JSON; `hell run ... --dump`
  prints the resolved modules, their parameters (with defaults marked), consumed branches and products.

## JSON layout
```json
{"process": "ANALYSIS",
 "input": {"files": [...], "treeName": "Events", "maxEvents": -1, "skipEvents": 0},
 "output": {"file": "analysis.root", "copyRuns": true, "copyLumis": true},
 "job": {"isData": false, "era": "2022", "sample": "TbarBQtoLNu_tch", "variation": "nominal", "crossSection": 87.2},
 "options": {"logLevel": "INFO", "reportEvery": 10000, "pluginDirs": []},
 "modules": {"jets": {"@type": "NanoJetReader", "btagger": "btagPNetB"}, ...},
 "path": ["jets", ...]}
```
Command-line overrides of `hell-run`: `-n`, `-s`, `-i file|@list|dir`, `-o`, `--variation`.
