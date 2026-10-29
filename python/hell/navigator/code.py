"""Navigate the code: module types available (from hell-run --list-modules),
the module graph of a configuration (producers/consumers), and where each
module type is defined (grep of HELL_REGISTER_MODULE)."""
import glob
import json
import os
import re
import subprocess

from hell import HELL_BASE, HellError, generated_by, log
from hell.runner import find_executable, work_json_path
from hell.config import load_config, Process


def module_types():
    exe = find_executable("hell-run")
    out = subprocess.run([exe, "--list-modules", "--json"], capture_output=True, text=True)
    if out.returncode != 0:
        raise HellError(out.stderr)
    return json.loads(out.stdout)


def definitions():
    """Module type -> (file, line) of the class definition and the python usage example in the header comment."""
    defs = {}
    for path in glob.glob(os.path.join(HELL_BASE, "Modules", "*", "plugins", "*.cc")):
        with open(path) as f:
            text = f.read()
        for m in re.finditer(r"HELL_REGISTER_MODULE\(\s*(?:\w+::)?(\w+)\s*,", text):
            name = m.group(1)
            cm = re.search(r"class\s+" + name + r"\s*:", text)
            line = text[:cm.start()].count("\n") + 1 if cm else text[:m.start()].count("\n") + 1
            params = sorted(set(re.findall(r'ps\.get(?:<[^>]+>)?\("([^"]+)"', text[cm.start():m.start()] if cm else text)))
            defs[name] = {"file": os.path.relpath(path, HELL_BASE), "line": line, "parameters": params}
    return defs


def config_graph(cfg_path, cfg_args=()):
    """Run hell-run --dump on the config and return the module/product graph."""
    process = load_config(cfg_path, cfg_args)
    if not isinstance(process, Process):
        raise HellError("not a Process config")
    if not process.input["files"]:
        raise HellError("the config has no input files; give sample=... or inputFiles=... (needed to resolve branches)")
    jp = work_json_path(cfg_path, tag="navigate")
    process.dump(jp)
    out = subprocess.run([find_executable("hell-run"), jp, "--dump", "-n", "0"], capture_output=True, text=True)
    if out.returncode != 0:
        raise HellError("hell-run --dump failed:\n" + out.stderr + out.stdout)
    j = json.loads(out.stdout[out.stdout.index("{"):])
    return j


def report_types(pattern=None):
    types = module_types()
    defs = definitions()
    lines = [f"[HELL] {len(types)} module types"]
    for t in types:
        if pattern and pattern.lower() not in t["type"].lower() and pattern.lower() not in t["doc"].lower():
            continue
        d = defs.get(t["type"], {})
        lines.append(f"  {t['type']:26s} {t['doc']}")
        if d:
            lines.append(f"  {'':26s} {d['file']}:{d['line']}   parameters: {', '.join(d['parameters'])}")
    return "\n".join(lines)


def report_graph(cfg_path, cfg_args=()):
    g = config_graph(cfg_path, cfg_args)
    lines = [f"[HELL] module graph of {cfg_path} (process {g.get('process')})", ""]
    producers = {}
    for m in g["path"]:
        for p in m["produces"]:
            producers[p["label"]] = m["label"]
    for m in g["path"]:
        prods = ", ".join(f"{p['label']} <{p['type']}>" for p in m["produces"])
        lines.append(f"{m['label']:22s} {m['type']:24s} [{m['kind']}]")
        if m["consumesBranches"]:
            lines.append(f"{'':22s} reads branches: {', '.join(m['consumesBranches'][:10])}{' ...' if len(m['consumesBranches']) > 10 else ''}")
        if m["consumesProducts"]:
            lines.append(f"{'':22s} consumes: " + ", ".join(f"{c} (from {producers.get(c, producers.get(c.split(':')[0], '?'))})" for c in m["consumesProducts"]))
        if prods:
            lines.append(f"{'':22s} produces: {prods}")
        ps = {k: v["value"] for k, v in m["parameters"].items() if not v["default"]}
        if ps:
            lines.append(f"{'':22s} parameters: {json.dumps(ps)}")
    lines.append("")
    lines.append(f"{len(g['branches'])} input branches read in total.")
    return "\n".join(lines)


def to_html(cfg_path, cfg_args, out_path):
    g = config_graph(cfg_path, cfg_args)
    defs = definitions()
    for m in g["path"]:
        m["definition"] = defs.get(m["type"], {})
    html = "<!-- " + generated_by() + " -->\n" + HTML.replace("/*__DATA__*/null", json.dumps(g)).replace("__TITLE__", os.path.basename(cfg_path))
    with open(out_path, "w") as f:
        f.write(html)
    log(f"navigator page written to {out_path}")
    return out_path


HTML = """<!doctype html><html><head><meta charset="utf-8"><title>HELL navigator: __TITLE__</title>
<style>body{font-family:sans-serif;margin:0;display:flex;height:100vh}#list{width:260px;overflow:auto;border-right:1px solid #ccc;padding:8px}
#main{flex:1;overflow:auto;padding:12px}.mod{cursor:pointer;padding:3px 6px;border-radius:4px}.mod:hover{background:#eee}.sel{background:#fde;}
.kind{font-size:11px;color:#888}pre{background:#f6f6f6;padding:8px;overflow:auto}svg text{font-size:11px}h2{margin-top:0}</style></head>
<body><div id="list"><b>[HELL] __TITLE__</b><div id="mods"></div></div><div id="main"><h2>Select a module</h2>
<svg id="graph" width="900" height="420"></svg><div id="detail"></div></div>
<script>const data=/*__DATA__*/null;const path=data.path;const producers={};path.forEach(m=>m.produces.forEach(p=>producers[p.label]=m.label));
function prod(c){return producers[c]||producers[c.split(':')[0]]||'?'}
const list=document.getElementById('mods');path.forEach((m,i)=>{const d=document.createElement('div');d.className='mod';d.innerHTML=m.label+' <span class=kind>'+m.type+'</span>';d.onclick=()=>show(i);list.appendChild(d)});
function show(i){document.querySelectorAll('.mod').forEach((e,j)=>e.classList.toggle('sel',i==j));const m=path[i];
const up=[...new Set(m.consumesProducts.map(prod))];const down=path.filter(o=>o.consumesProducts.some(c=>prod(c)==m.label)).map(o=>o.label);
const svg=document.getElementById('graph');svg.innerHTML='';const W=900;const node=(x,y,t,c)=>{svg.innerHTML+=`<rect x="${x-70}" y="${y-14}" width="140" height="28" rx="6" fill="${c}" stroke="#555"/><text x="${x}" y="${y+4}" text-anchor="middle">${t}</text>`};
const edge=(x1,y1,x2,y2)=>{svg.innerHTML+=`<line x1="${x1}" y1="${y1}" x2="${x2}" y2="${y2}" stroke="#888" marker-end="url(#a)"/>`};
svg.innerHTML='<defs><marker id="a" markerWidth="8" markerHeight="8" refX="8" refY="4" orient="auto"><path d="M0,0 L8,4 L0,8" fill="#888"/></marker></defs>';
up.forEach((u,k)=>{const x=80+k*160;node(x,60,u,'#dde');edge(x,74,W/2,186)});node(W/2,200,m.label,'#fcc');down.forEach((d,k)=>{const x=80+k*160;edge(W/2,214,x,326);node(x,340,d,'#dfd')});
let h='<h2>'+m.label+' <span class=kind>'+m.type+' ['+m.kind+']</span></h2>';if(m.definition&&m.definition.file)h+='<p>defined in <code>'+m.definition.file+':'+m.definition.line+'</code></p>';
h+='<p><b>reads branches:</b> '+(m.consumesBranches.join(', ')||'-')+'</p><p><b>consumes:</b> '+(m.consumesProducts.map(c=>c+' (from '+prod(c)+')').join(', ')||'-')+'</p>';
h+='<p><b>produces:</b> '+(m.produces.map(p=>p.label+' &lt;'+p.type+'&gt;').join(', ')||'-')+'</p><pre>'+JSON.stringify(m.parameters,null,1)+'</pre>';document.getElementById('detail').innerHTML=h}
if(path.length)show(0);</script></body></html>"""
