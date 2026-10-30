"""Configuration of HELL jobs, in the spirit of CMSSW python configs.

    from hell.config import Process, Module, VarParsing

    options = VarParsing()
    options.register("sample", "", str, "sample name from samples/<era>.yaml")
    options.parse()

    process = Process("SKIM", files=[...], output="skim.root", isData=False, era="2022")
    process.jets = Module("NanoJetReader", btagger="btagPNetB")
    process.goodJets = Module("JetSelector", src="jets", minPt=30)
    process.path = ["jets", "goodJets"]

A config file is a python script defining `process`. `hell run cfg.py key=value ...`
executes it, dumps the JSON to work/ and runs hell-run on it.
"""
import copy
import json
import os
import sys

from hell import HellError


class Module:
    """A module instance: type plus parameters. Attributes are parameters."""

    def __init__(self, type_, **params):
        object.__setattr__(self, "_type", type_)
        object.__setattr__(self, "_params", dict(params))

    @property
    def type(self):
        return self._type

    def __getattr__(self, name):
        try:
            return self._params[name]
        except KeyError:
            raise AttributeError(f"module of type {self._type} has no parameter '{name}'")

    def __setattr__(self, name, value):
        self._params[name] = value

    def clone(self, **overrides):
        m = Module(self._type, **copy.deepcopy(self._params))
        for k, v in overrides.items():
            m._params[k] = v
        return m

    def to_dict(self):
        d = {"@type": self._type}
        for k, v in self._params.items():
            d[k] = v.to_dict() if isinstance(v, Module) else v
        return d

    def __repr__(self):
        return f"Module({self._type!r}, {', '.join(f'{k}={v!r}' for k, v in self._params.items())})"


class Process:
    """A job: input, output, job context, modules and the ordered path."""

    _reserved = {"name", "input", "output", "job", "options", "path", "_modules"}

    def __init__(self, name, files=None, treeName="Events", maxEvents=-1, skipEvents=0, output="hell_output.root",
                 isData=False, era="", sample="", variation="nominal", crossSection=-1.0, copyRuns=True, copyLumis=True,
                 logLevel="INFO", reportEvery=10000):
        object.__setattr__(self, "_modules", {})
        self.name = name
        self.input = {"files": list(files or []), "treeName": treeName, "maxEvents": int(maxEvents), "skipEvents": int(skipEvents)}
        self.output = {"file": output, "copyRuns": copyRuns, "copyLumis": copyLumis}
        self.job = {"isData": bool(isData), "era": era, "sample": sample, "variation": variation, "crossSection": float(crossSection)}
        self.options = {"logLevel": logLevel, "reportEvery": int(reportEvery), "pluginDirs": []}
        self.path = []

    def __setattr__(self, name, value):
        if isinstance(value, Module):
            self._modules[name] = value
        elif name in self._reserved:
            object.__setattr__(self, name, value)
        else:
            raise HellError(f"process attribute '{name}' must be a Module (got {type(value).__name__})")

    def __getattr__(self, name):
        try:
            return self._modules[name]
        except KeyError:
            raise AttributeError(f"process has no module '{name}'")

    def modules(self):
        return dict(self._modules)

    def add_to_path(self, *labels):
        for l in labels:
            if l not in self._modules:
                raise HellError(f"cannot add '{l}' to the path: no such module")
            self.path.append(l)

    def to_dict(self):
        missing = [l for l in self.path if l not in self._modules]
        if missing:
            raise HellError(f"path refers to unknown modules: {missing}")
        from hell import generated_by
        return {
            "generatedBy": generated_by(),
            "process": self.name,
            "input": self.input,
            "output": self.output,
            "job": self.job,
            "options": self.options,
            "modules": {k: v.to_dict() for k, v in self._modules.items()},
            "path": list(self.path),
        }

    def to_json(self, indent=2):
        return json.dumps(self.to_dict(), indent=indent)

    def dump(self, path):
        os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
        with open(path, "w") as f:
            f.write(self.to_json())
        return path

    def summary(self):
        lines = [f"process {self.name}: {len(self.input['files'])} input files -> {self.output['file']}"]
        for l in self.path:
            lines.append(f"  {l:24s} {self._modules[l].type}")
        return "\n".join(lines)


class VarParsing:
    """cmsRun-style `key=value` options. Lists accept commas or @file.txt."""

    def __init__(self):
        self._options = {}
        self._order = []
        self.register("maxEvents", -1, int, "maximum number of events to process (-1: all)")
        self.register("skipEvents", 0, int, "number of events to skip")
        self.register("inputFiles", [], list, "input files (comma separated, @list.txt or directory)")
        self.register("outputFile", "", str, "output file name")
        self.register("variation", "nominal", str, "active object-level systematic variation")
        self.register("isData", False, bool, "the input is data")
        self.register("era", "", str, "data-taking era, e.g. 2022, 2022EE")
        self.register("sample", "", str, "sample name from samples/<era>.yaml (resolves files, xsec, isData)")
        self.register("maxFiles", -1, int, "use at most this many files of the sample")
        self.register("firstFile", 0, int, "index of the first file of the sample to use")
        self.register("crossSection", -1.0, float, "cross section in pb (overrides the sample registry)")
        self.register("nanoVersion", "", str, "NanoAOD version of the input (default: from the sample registry or detected from the first file); the selections follow it")
        self.register("allowNanoVersionMismatch", False, bool, "run even if the input files do not match the declared NanoAOD version")
        self.register("goldenJSON", "", str, "certification JSON for data (default: from the sample registry; 'none' disables the filter, e.g. when CRAB applies the lumi mask)")
        self.register("logLevel", "INFO", str, "DEBUG, INFO, WARNING, ERROR")

    def register(self, name, default, type_, doc=""):
        self._options[name] = {"value": default, "type": type_, "doc": doc, "default": default}
        if name not in self._order:
            self._order.append(name)

    def __getattr__(self, name):
        if name.startswith("_"):
            raise AttributeError(name)
        try:
            return self._options[name]["value"]
        except KeyError:
            raise AttributeError(f"unknown option '{name}' (register it first)")

    def _convert(self, name, text):
        t = self._options[name]["type"]
        if t is bool:
            return str(text).lower() in ("1", "true", "yes", "on")
        if t is list:
            if text.startswith("@"):
                with open(text[1:]) as f:
                    return [l.strip() for l in f if l.strip() and not l.startswith("#")]
            return [x for x in text.split(",") if x]
        return t(text)

    def parse(self, argv=None):
        argv = sys.argv[1:] if argv is None else argv
        for arg in argv:
            if arg in ("help", "--help", "-h"):
                print(self.help())
                sys.exit(0)
            if "=" not in arg:
                raise HellError(f"bad option '{arg}', expected key=value (try 'help')")
            k, v = arg.split("=", 1)
            if k not in self._options:
                raise HellError(f"unknown option '{k}'. Known: {', '.join(self._order)}")
            self._options[k]["value"] = self._convert(k, v)
        return self

    def help(self):
        lines = ["options (key=value):"]
        for k in self._order:
            o = self._options[k]
            lines.append(f"  {k:16s} {o['type'].__name__:6s} default={o['default']!r:16} {o['doc']}")
        return "\n".join(lines)

    def as_dict(self):
        return {k: self._options[k]["value"] for k in self._order}


def load_config(path, argv=()):
    """Execute a python config and return its `process` (or `job`) object."""
    path = os.path.abspath(path)
    if not os.path.exists(path):
        raise HellError(f"configuration file not found: {path}")
    saved = sys.argv
    sys.argv = [path] + list(argv)
    ns = {"__file__": path, "__name__": "__hell_config__"}
    cfg_dir = os.path.dirname(path)
    added = False
    if cfg_dir not in sys.path:
        sys.path.insert(0, cfg_dir)
        added = True
    try:
        with open(path) as f:
            code = compile(f.read(), path, "exec")
        exec(code, ns)
    finally:
        sys.argv = saved
        if added:
            sys.path.remove(cfg_dir)
    for key in ("process", "job", "config"):
        if key in ns:
            return ns[key]
    raise HellError(f"{path} does not define 'process' (or 'job'/'config')")
