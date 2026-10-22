# Navigator

`hell navigate <file.root>`: structure of the file, HELL bookkeeping (counts, normalisation hint, cutflow),
every tree with its branches grouped into tables (type, size, doc string, and which module wrote the table),
and the provenance chain (software version, git hash, command, inputs, path of modules, parents). Options:
`--branch 'Jet_*'`, `--entries 3` (print values), `--tree Events`, `--no-provenance`.

`hell navigate code`: module types with their one-line doc, where they are defined (file:line) and their
parameters; `--grep X` filters. `hell navigate code --config cfg.py key=value [--html page.html]`: the module
graph of a configuration (who reads which branches, who consumes what from whom, products and their types,
non-default parameters), as text or as a self-contained HTML page with a clickable graph.

`hell navigate model [Jet Lepton ...]`: the data model from the ROOT dictionary (members, types, methods).
