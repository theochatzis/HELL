# Object definitions (SingleTopMass, Run 3)

Definitions live in `configs/SingleTopMass/objects_cff.py` (`MuonSelector`, `ElectronSelector`, `JetSelector`,
`BTagger`); this page records the reasons and the references.

- Muons: tight ID, pt > 26 GeV (HLT_IsoMu24 plateau), |eta| < 2.4, PF relative isolation (dR 0.4) < 0.15.
  Non-isolated category 0.15 < iso < 0.6 for the QCD estimation. Veto: loose ID, pt > 10, iso < 0.25.
- Electrons: tight cut-based ID (Fall17V2 / Run 3 equivalent in NanoAOD `cutBased == 4`), pt > 30
  (HLT_Ele30_WPTight_Gsf), |eta_SC| < 2.5 (transition region to be excluded), iso < 0.10 (part of the ID in
  Run 3; checked separately). Veto: loose ID, pt > 15.
- Jets: AK4 PUPPI, tight + lepton-veto id recomputed from the PF energy fractions (the `Jet_jetId` flag of
  Run 3 NanoAODv12 is unreliable; criteria and references in `Modules/Selection/interface/JetId.h`, as in
  TopNanoAnalysis), pt > 30, |eta| < 4.7, cleaned from selected leptons (dR > 0.4). Pileup ID not needed with
  PUPPI. JEC applied in NanoAOD; variations in the analysis step. `jetIdSource="nano"` switches back to the flag.
- b tagging: ParticleNet (`btagPNetB`), working points from BTV for Run3Summer22 (values in `objects_cff.py`,
  verify), |eta| < 2.5. The light (spectator) jet is the most forward untagged jet (not even loose).
- MET: PUPPI MET (`PuppiMET_pt/phi`); propagation of jet variations done by `JetVariation`.
- Triggers: `HLT_IsoMu24`, `HLT_Ele30_WPTight_Gsf`. MET filters: the Run 3 recommended list.
References: see the README links (POG twikis, BTV wiki, JERC, EGamma, Muon).
