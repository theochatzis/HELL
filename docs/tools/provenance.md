# Provenance and bookkeeping in output files

Every file written by `hell-run` and `hell-hist` has a `hell/` directory:

| Object | Content | Under hadd |
|---|---|---|
| `hell/counts` (TH1D, labelled bins) | events_in, events_out, sumw_in, sumw_out, sumw_pos, sumw_neg, n_pos, n_neg, genEventCount, genEventSumw, genEventSumw2, files | adds |
| `hell/cutflow`, `hell/cutflow_w` | events (weighted) passing each module of the path | adds |
| `hell/provenance` (TTree, branch `json`) | one record per job: software (version, git hash, build time, ROOT), time, host, user, cwd, command line, full configuration, job context, input files (and skipped ones), products (modules with their parameters, consumed branches/products, produced products), bookkeeping, and the `parents` records read from the input files | concatenates |
| `hell/hell_tag` (TObjString) | "HELL <version> <git>" | first kept |
| `Runs`, `LuminosityBlocks` (hell-run) | copies of the inputs' trees | concatenate |
| `hell/hist_config` (hell-hist) | the histogram configuration JSON (processes, colours, lumi, ...) | first kept |

Read back in python:
```python
import ROOT, json
f = ROOT.TFile.Open("skim.root")
for e in f.Get("hell/provenance"):
    rec = json.loads(str(e.json)); print(rec["software"], rec["job"], len(rec["parents"]))
```
or `hell navigate skim.root`. Re-run `cmake` after committing so that the git hash is current; a `-dirty`
suffix means uncommitted changes were present at build time.
