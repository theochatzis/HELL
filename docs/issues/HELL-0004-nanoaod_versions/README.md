# NanoAOD versions: alignment of the selections to the input

**Id**: HELL-0004  **Area**: samples  **Opened**: 2026-10-30
**Status**: done (mechanism, v12 and v15 exercised); open: v13, a v15 production
**Impact**: a job knows which NanoAOD version it reads and uses the branch names and recipes of that version

## Context
The 2022 samples exist in NanoAODv12, v13 and v15 (and Run 2 in v9). Branch names and content differ, and the
files carry no version (`tag = untagged`). Without a mechanism, a change of input version silently changes
what a selection does (or crashes on a missing branch).

## What was done
- `samples/nanoaod.yaml`: fingerprints and settings per version (9, 12, 13, 15), old entries never removed.
- Sample registry: `nanoVersion` per sample (from the dataset name), shown by `hell samples list`.
- Job: version from the option, the sample or the file; settings in `job.nano`; `objects_cff.nano(process)`
  drives the readers and the jet id source; `hell-run` checks the fingerprint and refuses a mismatch.
- `NanoElectronReader` `etaSC` option (`deltaEtaSC` / `superclusterEta`).

## Findings / numbers
Branch comparison, Run3Summer22NanoAODv12 (TTtoLNu2Q) vs Run3Summer22EENanoAODv15 (TTLL): 1840 vs 1888
branches. Of the ones HELL uses: `Jet_jetId` only in v12; `Jet_chMultiplicity`, `Jet_neMultiplicity`,
`Jet_btagUParTAK4B`, `Electron_superclusterEta`, `Electron_pfRelIso04_all` only in v15; `MET_*` (v12)
became `PFMET_*` (v15); the `PuppiMET_*JES/JER*` pre-computed variations of v12 are gone in v15. Everything
else we read (PuppiMET, muon and electron ids and isolations, pileup, rho, LHE/PS weights, GenJet, HLT paths,
MET flags) is unchanged.

Exercised: skim and analysis on a Run3Summer22EENanoAODv15 file (TTLL, 20000 events, era 2022EE): the
fingerprint check passes, the central jet id (`AK4PUPPI_TightLeptonVeto` from `2022_Summer22EE/jetid.json.gz`)
is used, PF MET is read from `PFMET_*`, and all 2022EE corrections load and evaluate (JER `Summer22EE_22Sep2023_JRV1`,
PNet working points L 0.0499 M 0.2605 T 0.6915, pileup `eraEFG`, muon and electron JSONs with year
`2022Re-recoE+PromptFG`). A v12 file declared as v15 is refused with the list of missing and unexpected branches.

## Conclusion / decision
Selections follow `samples/nanoaod.yaml`. Open: a v13 file; decide whether to move the tagger to UParT for
v15 (the BTV working points and scale factors exist for `robustParticleTransformer` in `2022_Summer22`, UParT
in newer JSONs); the `PuppiMET_*JES/JER*` pre-computed variations of v12 are not used (our own propagation).
