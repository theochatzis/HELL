# NanoAOD versions

The selections are aligned to the NanoAOD version of the input, not to whatever branches happen to exist.

- `samples/nanoaod.yaml` describes each version: a branch **fingerprint** (branches that must be present and
  absent) and the **settings** that differ (jet id source, PF MET prefix, electron supercluster eta branch,
  b-tag score branch, availability of the jet multiplicities). A version inherits the defaults and lists only
  its differences; old versions keep their entries when a new one is added.
- Every sample has a `nanoVersion` (`hell samples create` reads it from the dataset name, `NanoAODv12`;
  `load_era` fills it from the DAS name for older entries). `hell samples list` shows it.
- A job gets the version from `nanoVersion=`, else from the sample, else by detecting it from the first
  input file (`hell navigate file.root` also prints the detected version). The settings go into
  `process.job["nano"]` and `objects_cff.nano(process)` reads them: readers (`etaSC`, `btagger`, MET prefix)
  and `JetSelector` (`jetIdSource`) follow the version.
- `hell-run` checks the declared version against the fingerprint of the input and refuses a mismatch
  (`allowNanoVersionMismatch=True` to override); the version is recorded in the provenance.

Known differences (measured on Run3Summer22 v12 and v15 files, see issue HELL-0004):

| | v12 | v13 | v15 |
|---|---|---|---|
| `Jet_jetId` | present, unreliable for PUPPI: manual recipe | present | removed: central JME jet id |
| `Jet_chMultiplicity`, `Jet_neMultiplicity` | no | yes | yes |
| PF MET | `MET_*` | `MET_*` | `PFMET_*` |
| electron supercluster eta | `eta + deltaEtaSC` | same | `Electron_superclusterEta` |
| taggers | DeepJet, PNet, RobustParT | same | DeepJet, PNet, UParT (`Jet_btagUParTAK4B`) |

Adding a version: a new entry in `samples/nanoaod.yaml` with its fingerprint and the settings that differ;
run `hell navigate` on a file of it to confirm the detection; record the differences in the issue.
