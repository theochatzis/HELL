"""SingleTopMass skim: loose single-lepton + jet preselection of NanoAOD.

Keeps the NanoAOD branches needed later (no corrections applied here: they
belong to the analysis step), the Runs/LuminosityBlocks trees for the
normalisation and the Golden JSON, and gen-level distributions of ALL events
(before any cut) for the positive reweighting and cross checks.

    hell run configs/SingleTopMass/skim_cfg.py sample=TbarBQtoLNu_tch era=2022 maxEvents=5000
    hell submit run configs/SingleTopMass/skim_cfg.py --sample TbarBQtoLNu_tch --era 2022 --files-per-job 10 --output-dir ... --tag skim_v1
"""
from hell.config import Process, Module, VarParsing
from hell.runner import apply_options
from objects_cff import readers, leptons, jets, TRIGGERS, MET_FILTERS

options = VarParsing()
options.register("minJets", 1, int, "minimum number of selected jets")
options.register("genPrescale", 1, int, "fill gen distributions for one event every N (1: all)")
options.parse()

process = Process("SKIM", output="skim.root", reportEvery=20000)
apply_options(process, options)
era = process.job["era"] or "2022"

# Gen-level information of every event, before anything else.
path = readers(process)
process.genDists = Module("GenDistributionAnalyzer", genParticles="genParts", eventInfo="eventInfo")
path += ["genDists"]
# Trigger and MET filters
process.trigger = Module("TriggerFilter", paths=TRIGGERS["mu"] + TRIGGERS["el"])
process.metFilters = Module("FlagFilter", flags=MET_FILTERS)
path += ["trigger", "metFilters"]
# Certified lumi sections for data (transparent for MC); can also be applied later in the analysis.
if process.job["isData"] and process.job.get("goldenJSON"):
    process.golden = Module("GoldenJSONFilter", json=process.job["goldenJSON"])
    path += ["golden"]
# Loose objects (half the analysis thresholds), one lepton, >= minJets jets
path += leptons(process, loose=True)
process.oneLepton = Module("CollectionSizeFilter", src="leptons", min=1)
process.looseJets = Module("JetSelector", src="jets", minPt=20, maxAbsEta=4.7, jetIdBit=2, cleanAgainst=["leptons"])
process.minJets = Module("CollectionSizeFilter", src="looseJets", min=options.minJets)
path += ["oneLepton", "looseJets", "minJets"]
# What is kept
process.skim = Module("TreeWriter", treeName="Events",
                      keep=["run", "luminosityBlock", "event", "genWeight", "HLT_IsoMu24", "HLT_IsoMu27", "HLT_Ele30_WPTight_Gsf", "HLT_Ele32_WPTight_Gsf",
                            "Flag_*", "Jet_*", "nJet", "Muon_*", "nMuon", "Electron_*", "nElectron", "PuppiMET_*", "MET_*", "RawPuppiMET_*",
                            "GenPart_*", "nGenPart", "GenJet_*", "nGenJet", "Pileup_nTrueInt", "Pileup_nPU", "PV_npvs", "PV_npvsGood",
                            "Rho_fixedGridRhoFastjetAll", "LHE_*", "LHEScaleWeight", "nLHEScaleWeight", "LHEPdfWeight", "nLHEPdfWeight",
                            "PSWeight", "nPSWeight", "GenMET_*", "genTtbarId"],
                      drop=["Jet_*Calo*", "Jet_hfsigma*", "Jet_hfcentral*", "Muon_*mva*", "Muon_jetPtRel*", "Electron_*mva*", "Electron_jetPtRel*",
                            "Jet_btagRobust*", "Jet_btagDeepFlavCvB", "Jet_btagDeepFlavCvL", "Jet_btagDeepFlavQG", "Jet_PNetRegPtRaw*"])
path += ["skim"]
process.path = path
