# Issues and progress log

One folder per topic: a problem, a study, a decision, a feature. The README of the folder has the status, the
numbers, the plots (in `plots/`) and the exact commands to reproduce them. This is both the memory of the
project (what was tried, what was fixed) and the raw material of the analysis note.

```
hell issue new "JES variation does not propagate to MET" --area systematics
hell issue list
```
Folders are numbered `HELL-NNNN-<slug>`. Areas (free text): framework, skim, objects, systematics, ml, fit,
physics, samples, batch. Status values: open, in progress, done, wontfix.

| Issue | Status | Title |
|---|---|---|
| [HELL-0001](HELL-0001-framework_skeleton/README.md) | done | Framework skeleton: design, first working chain |
| [HELL-0002](HELL-0002-central_corrections/README.md) | done | Central corrections through correctionlib (working points, JES/JER, pileup, lepton and b-tag SFs) |
