# Object and sample recommendations to verify

**Id**: HELL-0003  **Area**: objects  **Opened**: 2026-10-30
**Status**: open
**Impact**: every item below is in the configs today with a value or a choice I am not certain is the current
recommendation. Until checked, results are "work in progress". Tick each item with the source and the date.

The checks need the TOP PAG and POG pages (PdmVRun3Analysis, TOP Run 3 recommendations, BTV, JME, EGM, MUO,
LUM twikis) that need a CERN login I cannot pass from this session, and the XSDB.

## Samples and campaigns
- [ ] **NanoAOD campaign.** We use `Run3Summer22NanoAODv12-130X_mcRun3_2022_realistic_v5` (pre-EE) and
  `Run3Summer22EENanoAODv12-...postEE_v6`. Check whether PdmV recommends v12 or a newer production (v13/v15:
  different jet id and PNet branches) for 2022, and the matching data re-reco (`22Sep2023` was used).
- [ ] **Datasets.** Signal `TBbarQtoLNu-t-channel-4FS` and `TbarBQtoLNu-t-channel-4FS` (powheg + madspin); check that
  these are the TOP recommended t-channel samples (and not `TQbartoLNu` variants), and which mass points and
  modelling variations (hdamp, tune, CR) exist for 2022. Backgrounds: `TTtoLNu2Q`, `TTto2L2Nu`, `TWminustoLNu2Q`,
  `TbarWplustoLNu2Q`, `WtoLNu-2Jets` (aMC@NLO FxFx), `DYto2L-2Jets_MLL-50`; QCD and diboson missing.
- [ ] **Cross sections** (`samples/xsec.yaml`, all marked "verify"): t-channel 145.0 / 87.2 pb (top / antitop, times
  BR), ttbar 923.6 pb split by BR, tW 87.9 pb, W+2jets 64481.6 pb, DY M>50 2 jets 6688.0 pb. Source each from
  XSDB or the TOP twiki, with the precision, and fix the BR factors.
- [ ] **Luminosity.** 2022 pre-EE from brilcalc with `normtag_PHYSICS.json`: 8086.1 pb^-1 for runs 355100 to 359021.
  Check the normtag and the official 2022 number (the Lumi POG quotes about 7.98 fb^-1 for C+D with the
  final calibration; the difference to our value needs an explanation: run range or normtag).
- [ ] **Golden JSON.** `Cert_Collisions2022_355100_362760_Golden.json` for both 2022 and 2022EE, and the run split
  at 359022 (first run of 2022E). Confirm the split and whether a newer certification file exists.

## Jets
- [ ] **Jet id.** The manual recipe (`Modules/Selection/interface/JetId.h`) follows TopNanoAnalysis for NanoAODv12.
  Two points I am not sure of: the lepton-veto cuts (`muEF < 0.8`, `chEmEF < 0.8`) are applied only for
  |eta| <= 2.6 and not in 2.6 to 2.7, and the forward region uses `nConstituents >= 2` where the JME recipe asks
  for `neMultiplicity >= 2` (branch missing in v12). Check against the JME 13.6 TeV jet id page; with v13+
  switch to the central JSON (`jetIdSource="correctionlib"`).
- [ ] **JEC.** NanoAODv12 jets carry the campaign JEC; we do not re-apply them (`reapplyJEC=False`). Confirm this is
  acceptable or whether `Summer22_22Sep2023_V2` (MC) and `RunCD_V2` (data) should be re-applied from the raw pt.
- [ ] **JES uncertainty sources.** We vary `Regrouped_Total` only. The TOP recommendation is the reduced set of
  regrouped sources (Absolute, Absolute_2022, BBEC1, BBEC1_2022, EC2, EC2_2022, FlavorQCD, HF, HF_2022,
  RelativeBal, RelativeSample_2022) treated as separate nuisances: to be added to `jesSources` and the
  histogram configuration.
- [ ] **JER.** Hybrid method with matching dR < 0.2 (R/2) and |pt - ptgen| < 3 sigma, `Summer22_22Sep2023_JRV1`.
  Check the JER version for 2022 and the matching parameters.
- [ ] **Jet veto map.** Applied to all jets above 15 GeV passing the selection with type `jetvetomap`; check the
  recommended jet set (loose id, pt > 15, EM fraction < 0.9, away from muons) and whether `jetvetomap_eep`
  is also needed for 2022EE.
- [ ] **Jet pt threshold** 30 GeV and |eta| < 4.7; forward jet pt 50 in the old analysis. Check the TOP recommendation.
- [ ] **b tagging.** ParticleNet (`btagPNetB`) with the BTV working points of `2022_Summer22` (L 0.047, M 0.245,
  T 0.6734); scale factors `particleNet_comb` for b and c, `particleNet_light` for light, fixed working point
  method with efficiency maps. Check: tagger choice for the top mass (PNet vs robustParticleTransformer),
  `comb` vs `mujets`, the correlated/uncorrelated split across eras, and that the fixed-WP method is accepted.

## Leptons
- [ ] **Muons.** Tight id, `pfRelIso04_all < 0.15` (should it be the POG `pfIsoId >= 4` instead?), pt > 26, |eta| < 2.4,
  veto: loose id, pt > 10, iso < 0.25. Scale factors from `muon_Z.json.gz`: `NUM_TightID_DEN_TrackerMuons`,
  `NUM_TightPFIso_DEN_TightID`, `NUM_IsoMu24_DEN_CutBasedIdTight_and_PFIsoTight`; pt is clamped to 15 to 200 GeV
  and |eta| to 2.4. Check the recommended id/iso combination and whether `muon_HighPt` is needed above 200 GeV.
- [ ] **Electrons.** `cutBased == 4` (tight), pt > 30, |etaSC| < 2.5 with the gap 1.4442 to 1.566 excluded, an extra
  `pfRelIso03_all < 0.10` (the cut-based id already includes isolation: is the extra cut wanted?), veto:
  `cutBased >= 2`, pt > 15. Scale factors: `Electron-ID-SF` with year `2022Re-recoBCD`, working point `Tight`
  and the reco factors `RecoBelow20`, `Reco20to75`, `RecoAbove75` with the boundaries at 20 and 75 GeV;
  HLT `Electron-HLT-SF`, path `HLT_SF_Ele30_TightID`. The 2022EE year key `2022Re-recoE+PromptFG` and all 2022EE
  correction names load and evaluate (HELL-0004, 2026-10-30). Check the reco boundaries and whether the EGM
  scale and smearing corrections (`electronSS`) must be applied.
- [ ] **Triggers.** `HLT_IsoMu24` and `HLT_Ele30_WPTight_Gsf` for 2022 (and the same for 2022EE). Confirm the
  recommended single-lepton paths and whether the OR with `HLT_IsoMu27` / `HLT_Ele32_WPTight_Gsf` is used.
- [ ] **Lepton cleaning** of jets with dR > 0.4 against the selected (not the veto) leptons; check the TOP convention.

## MET and event
- [ ] **MET.** PUPPI MET (`PuppiMET_pt/phi`); our JES/JER propagation subtracts the jet changes above 15 GeV without
  the muon-fraction subtlety; the Type-1 recipe and the XY corrections (`met_xyCorrections_2022_2022.json.gz`)
  are not applied. Check the Run 3 MET recommendation (PUPPI vs PF, XY corrections, unclustered variation).
- [ ] **MET filters.** `Flag_goodVertices`, `Flag_globalSuperTightHalo2016Filter`,
  `Flag_EcalDeadCellTriggerPrimitiveFilter`, `Flag_BadPFMuonFilter`, `Flag_BadPFMuonDzFilter`,
  `Flag_hfNoisyHitsFilter`, `Flag_eeBadScFilter`. Check the Run 3 list (`Flag_ecalBadCalibFilter`?).
- [ ] **Pileup.** `Collisions2022_355100_357900_eraBCD_GoldenJson` for 2022 pre-EE (our run range is C+D, 355100 to
  359021): the correction is named for B, C, D; confirm it is the right one for C+D and that the up/down are the
  minimum-bias cross section variations.
- [ ] **Generator weights.** muR/muF from `LHEScaleWeight` indices 1, 3, 5, 7 (NanoAOD convention), ISR/FSR from
  `PSWeight` 0 to 3; no PDF variations yet; no top pt reweighting (not recommended for the mass).
- [ ] **Negative weights** of aMC@NLO samples: the positive reweighting tool is planned; until then the W+jets and
  DY templates can have negative bins.

## How to close an item
Write the source (page and date) next to the item, change the value in the config or `samples/*.yaml` if
needed, and record the change in the release note of the next tag. Items that turn into real changes get their
own issue if they need a study.
