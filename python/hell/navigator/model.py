"""Navigate the HELL data model through the ROOT dictionary of libHellDataFormats."""
import os

from hell import HELL_BUILD, HellError

TYPES = ["Candidate", "Jet", "Lepton", "Muon", "Electron", "MET", "GenParticle", "EventInfo", "Weights", "WeightEntry", "TopCandidate"]


def load_library():
    import ROOT
    lib = os.path.join(HELL_BUILD, "lib", "libHellDataFormats.so")
    if not os.path.exists(lib):
        raise HellError(f"{lib} not found: build HELL first")
    if ROOT.gSystem.Load(lib) < 0:
        raise HellError(f"cannot load {lib}")
    return ROOT


def describe(type_name):
    ROOT = load_library()
    cls = ROOT.TClass.GetClass(f"hell::{type_name}")
    if not cls:
        raise HellError(f"no dictionary for hell::{type_name}")
    members = []
    for m in cls.GetListOfDataMembers():
        members.append((m.GetName(), m.GetTrueTypeName()))
    bases = [b.GetName() for b in cls.GetListOfBases()] if cls.GetListOfBases() else []
    methods = [m.GetName() for m in cls.GetListOfMethods() if not m.GetName().startswith(("~", "operator")) and m.GetName() not in (type_name, "Class", "Class_Name", "Class_Version", "Dictionary", "IsA", "ShowMembers", "Streamer", "StreamerNVirtual", "DeclFileName", "ImplFileName", "ImplFileLine", "DeclFileLine", "CheckTObjectHashConsistency")]
    return {"type": f"hell::{type_name}", "bases": bases, "members": members, "methods": sorted(set(methods))}


def report(types=None):
    lines = ["[HELL] data model (DataFormats/interface)"]
    for t in (types or TYPES):
        d = describe(t)
        lines.append(f"\n{d['type']}" + (f" : {', '.join(d['bases'])}" if d["bases"] else ""))
        for name, tn in d["members"]:
            lines.append(f"  {tn:28s} {name}")
        if d["methods"]:
            lines.append(f"  methods: {', '.join(d['methods'])}")
    return "\n".join(lines)
