# Code practices

The point of this repository is an analysis toolkit that is fast, robust and flexible, but also fun.
These are the rules we follow, in simple sentences.

## General
1. The physics runs in C++. Python configures, orchestrates and plots. No physics logic in python.
2. One module does one thing. If a module needs an option that changes what it is, write a second module.
3. Everything is configured, nothing is hard-coded: thresholds, branch names, file paths, colours, lumi.
4. Every number that enters a result is traceable: cross sections carry their source, every file carries its provenance.
5. Stable names. Branches, histograms, products and parameters are identifiers used by other people and configs: add new ones, do not rename old ones.
6. Fail loudly and early: throw `std::runtime_error` on bad configuration, in the constructor, with the module label in the message. Never fail silently.
7. Warn once, not per event.
8. No global state in modules. Several instances of a module type must coexist.
9. Units: GeV, cm, radians. Pseudorapidity is `eta`, not `y`.
10. Every executable and every `hell` subcommand has `--help` that shows the usage **and an example**.
11. Names say what things are. Use full descriptive names you can search for (`sample_name`, `run_command`,
    `era_data`), not single letters or abbreviations whose meaning is not obvious at the place they are used.
    Short names are fine only for loop indices and mathematics (`i`, `pt`, `eta`).

## C++
1. C++17, compiled with `-Wall -Wextra -Wpedantic`; no warnings in our code (`hell build` prints them).
2. Headers in `interface/`, implementations in `src/`, plugins in `plugins/`. Include with the repository root: `#include "Core/interface/Event.h"`.
3. A module reads its parameters in the constructor, with explicit defaults, and declares what it consumes and produces there. Nothing is looked up for the first time inside the event loop.
4. Only `Nano*Reader` modules touch NanoAOD branch names. Everything else works on the HELL data model.
5. Products are moved into the event (`ev.put(label(), std::move(x))`), never copied.
6. Prefer `const&`, `std::vector`, `std::unique_ptr`. Raw pointers only for ROOT objects owned by ROOT (histograms in a directory).
7. The header comment of a module says what it does, what it produces and shows the python line that configures it.
8. Register a module at the end of its file with one line of documentation: `HELL_REGISTER_MODULE(hell::X, "what it does")`.
9. Log through `HELL_INFO(label())`, `HELL_WARN`, `HELL_ERROR`. Every line starts with `[HELL]` so logs from batch jobs can be grepped.
10. Formatting: 2 spaces, 140 columns, braces on the same line, `camelCase` for functions and variables, `PascalCase` for types, trailing underscore for members.

## Python
1. Python 3.9+, standard library plus numpy, yaml, ROOT, torch (only in `hell.ml`), matplotlib (plots only).
2. Heavy imports (ROOT, torch) happen inside functions so that `hell --help` is instant.
3. Configs are plain python files defining `process`. Shared definitions live in `*_cff.py` next to them.
4. User-facing errors raise `HellError` with a sentence that says what to do; the CLI prints it without a traceback.
5. Functions that write files log the path they wrote.

## Configuration
1. A config must run as is: `hell run cfg.py sample=X era=Y`. Options have defaults and a one-line doc.
2. Object definitions (what is a jet, a lepton, a b tag) live in one place per analysis (`objects_cff.py`) and are used by the skim, the analysis tree and every study.
3. Skims apply no corrections. Corrections and scale factors belong to the analysis step, where they can be varied.
4. Histogram and fit configurations are YAML, versioned with the code.

## Generated files
1. Every file written by a tool (configuration JSON, datacard, condor or CRAB script, sample list, report, exported
   model) starts with the line `Automatically generated using: <the exact command>` (as a comment, or a
   `generatedBy` key in JSON), so that anyone finding the file knows how to make it again. `hell.generated_by()`
   gives the line.

## Data and bookkeeping
1. Nothing is deleted from a file by the framework; files are only created.
2. Every output file is mergeable with `hadd`: counts add, trees concatenate, provenance concatenates.
3. The normalisation uses the sum of generator weights of all generated events (`Runs.genEventSumw`), never the events after a selection.
4. Sample lists are generated (`hell samples files`), not typed. Cross sections live in `samples/xsec.yaml` with their source.

## Documentation and progress
1. A new tool or module comes with its entry in `docs/tools/` and its example in the header comment.
2. A study, a problem or a decision gets an issue folder: `hell issue new "title"`. It has the numbers, the plots and the exact commands to reproduce them. This is the raw material of the analysis note.
3. `docs/` describes how things are; `docs/issues/` records how they got there and what is still open. Anything uncertain, unverified or decided on a guess is written in an issue (a checklist with a status), not only mentioned in a doc, so that nothing "to verify" is forgotten. Every session that changes the analysis leaves an issue entry behind.
4. Physics effects and methods go to `docs/methodology/` and `docs/physics/`, written so that they can be pasted into the note.
5. Commit messages say what changed and why. Re-run `cmake` after committing so that the git hash in the provenance is right.

## Versions, branches and releases
1. `main` holds the latest released code. Development of a version happens on its branch `HELL_v<maj>.<min>`
   (the CMSSW `CMSSW_X_Y_X` cycle branch); it is merged into `main` when released.
2. Exact versions are annotated tags: `HELL_v<maj>.<min>_pre<N>` for pre-releases, `HELL_v<maj>.<min>` for the
   release, `HELL_v<maj>.<min>.<patch>` for fixes on a released version.
3. MAJOR changes when configs or analysis code break (a product, column or parameter renamed or removed);
   MINOR adds features; PATCH fixes only.
4. Every release has a note `docs/releases/notes/<tag>.md` from the template (new, changed, fixed, validation,
   known limitations) and updates the tables in `docs/releases/README.md`, committed with the release.
5. A release is tagged only from a clean tree that builds warning-free and passes `hell pray`. Never produce
   with a `-dirty` version; the version in every output file (`hell navigate`) is the proof.
6. Fixes needed by a released version are cherry-picked (one topic per commit), never merged across branches.
   Details and commands: `docs/releases/branching.md`.

## Fun
1. Commands and messages may be in the spirit of the name: `hell run`, `hell unleash-systematics`, `hell pray`, `[HELL] Fit failed to converge.`, `[HELL] Welcome to HELL.` Keep them short and never ambiguous about what happened.
2. Breaking the build is the only real sin.
