"""Navigate a ROOT file: structure, trees and branches (type, doc, size),
HELL bookkeeping (counts, cutflow), provenance chain, and sample values."""
import fnmatch
import json

from hell import HellError


def open_file(path):
    import ROOT
    ROOT.gErrorIgnoreLevel = ROOT.kError
    f = ROOT.TFile.Open(path)
    if not f or f.IsZombie():
        raise HellError(f"cannot open {path}")
    return f


def structure(d, prefix=""):
    out = []
    for k in d.GetListOfKeys():
        o = k.ReadObj()
        name = prefix + k.GetName()
        if o.InheritsFrom("TDirectory"):
            out.append((name + "/", "TDirectory", ""))
            out += structure(o, name + "/")
        else:
            info = ""
            if o.InheritsFrom("TTree"):
                info = f"{o.GetEntries()} entries, {len(o.GetListOfBranches())} branches"
            elif o.InheritsFrom("TH1"):
                info = f"{o.GetEntries():.0f} entries, integral {o.Integral():.6g}"
            elif o.InheritsFrom("TObjString"):
                info = str(o.GetString())[:60]
            out.append((name, k.GetClassName(), info))
    return out


def branches(tree, pattern="*"):
    out = []
    for b in tree.GetListOfBranches():
        name = b.GetName()
        if not fnmatch.fnmatch(name, pattern):
            continue
        leaf = b.GetLeaf(name) or b.GetListOfLeaves().At(0)
        tname = leaf.GetTypeName() if leaf else "?"
        count = leaf.GetLeafCount().GetName() if leaf and leaf.GetLeafCount() else ""
        out.append({"name": name, "type": tname + (f"[{count}]" if count else ""), "doc": b.GetTitle() if b.GetTitle() != name else "",
                    "zipbytes": b.GetZipBytes(), "totbytes": b.GetTotBytes()})
    return out


def tables(blist):
    """Group NanoAOD-style branches into tables: Jet_pt -> table Jet, column pt."""
    t = {}
    for b in blist:
        n = b["name"]
        if "_" in n and not n.startswith("HLT_") and not n.startswith("Flag_") and not n.startswith("L1_"):
            tab, col = n.split("_", 1)
        elif n.startswith("n") and n[1:2].isupper():
            tab, col = n[1:], "(counter)"
        else:
            tab, col = "(event)", n
        t.setdefault(tab, []).append(dict(b, column=col))
    return t


def counts(f):
    h = f.Get("hell/counts")
    if not h:
        return {}
    return {h.GetXaxis().GetBinLabel(i): h.GetBinContent(i) for i in range(1, h.GetNbinsX() + 1) if h.GetXaxis().GetBinLabel(i)}


def cutflow(f):
    h = f.Get("hell/cutflow")
    if not h:
        return []
    return [(h.GetXaxis().GetBinLabel(i), h.GetBinContent(i)) for i in range(1, h.GetNbinsX() + 1) if h.GetXaxis().GetBinLabel(i)]


def provenance(f):
    t = f.Get("hell/provenance")
    if not t:
        return []
    out = []
    for e in t:
        try:
            out.append(json.loads(str(e.json)))
        except Exception:
            pass
    return out


def branch_writer_map(prov_records):
    """Branch name -> (module label, module type) using the products recorded in the provenance."""
    m = {}
    for rec in prov_records:
        for mod in rec.get("products", {}).get("path", []):
            if mod.get("type") == "TreeWriter":
                prods = mod.get("parameters", {}).get("products", {}).get("value", {}) or {}
                for label, table in prods.items():
                    m[table] = (label, "product")
            for p in mod.get("produces", []):
                m.setdefault(p["label"], (mod["label"], mod["type"]))
    return m


def sample_values(tree, names, n=3):
    out = []
    for i in range(min(n, tree.GetEntries())):
        tree.GetEntry(i)
        row = {}
        for nme in names:
            try:
                v = getattr(tree, nme)
                row[nme] = list(v)[:4] if hasattr(v, "__len__") and not isinstance(v, str) else v
            except Exception:
                row[nme] = "?"
        out.append(row)
    return out


def report(path, pattern="*", entries=0, show_provenance=True, tree_name=None):
    f = open_file(path)
    lines = [f"[HELL] navigating {path}", ""]
    st = structure(f)
    lines.append("Structure:")
    for name, cls, info in st:
        lines.append(f"  {name:36s} {cls:14s} {info}")
    c = counts(f)
    if c:
        lines += ["", "HELL bookkeeping (hell/counts):"]
        for k, v in c.items():
            lines.append(f"  {k:16s} {v:.6g}")
        if c.get("genEventSumw"):
            lines.append(f"  -> normalisation weight for lumi L and cross section s: L * s / {c['genEventSumw']:.6g}")
    cf = cutflow(f)
    if cf:
        lines += ["", "Cutflow:"]
        prev = None
        for lab, v in cf:
            eff = f"{100 * v / prev:6.1f}%" if prev else ""
            lines.append(f"  {lab:28s} {v:12.0f} {eff}")
            prev = v if v > 0 else prev
    trees = [name.rstrip("/") for name, cls, _ in st if cls == "TTree" and not name.startswith("hell/")]
    prov = provenance(f)
    writers = branch_writer_map(prov)
    for tn in trees:
        if tree_name and tn != tree_name:
            continue
        t = f.Get(tn)
        bl = branches(t, pattern)
        if not bl:
            continue
        lines += ["", f"Tree {tn}: {t.GetEntries()} entries, {len(t.GetListOfBranches())} branches" + (f" (showing {pattern})" if pattern != "*" else "")]
        for tab, cols in tables(bl).items():
            w = writers.get(tab)
            origin = f"  <- {w[0]} ({w[1]})" if w else ""
            lines.append(f"  [{tab}]{origin}")
            for b in cols:
                lines.append(f"    {b['name']:34s} {b['type']:16s} {b['zipbytes'] / 1024:8.1f} kB  {b['doc']}")
        if entries > 0:
            names = [b["name"] for b in bl][:12]
            lines.append(f"  first {entries} entries of {', '.join(names)}:")
            for row in sample_values(t, names, entries):
                lines.append("    " + ", ".join(f"{k}={v}" for k, v in row.items()))
    if show_provenance and prov:
        lines += ["", f"Provenance: {len(prov)} record(s)"]
        for i, r in enumerate(prov):
            sw = r.get("software", {})
            job = r.get("job", {})
            lines.append(f"  [{i}] {sw.get('framework')} {sw.get('version')} ({sw.get('git')}) process={job.get('process')} sample={job.get('sample')} variation={job.get('variation')} by {r.get('user')}@{r.get('host')} at {r.get('time')}")
            lines.append(f"      command: {' '.join(r.get('command', []))}")
            lines.append(f"      inputs: {len(r.get('inputs', {}).get('files', []))} files; parents: {len(r.get('parents', []))} record(s)")
            path_mods = r.get("products", {}).get("path", [])
            if path_mods:
                lines.append("      path: " + " -> ".join(f"{m['label']}" for m in path_mods))
            for parent in r.get("parents", [])[:3]:
                pj = parent.get("job", {}); psw = parent.get("software", {})
                lines.append(f"        parent: {psw.get('framework')} {psw.get('version')} process={pj.get('process')} sample={pj.get('sample')} from {parent.get('_file', '?')}")
    f.Close()
    return "\n".join(lines)
