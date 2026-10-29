"""Object definitions shared by the SingleTopMass configurations.

Everything that defines what a "jet", a "lepton" or a "b tag" is lives here,
so that the skim, the analysis tree and any study use the same definitions.
Working points (Run 3, 2022, ParticleNet) from the BTV recommendations:
  https://btv-wiki.docs.cern.ch/ScaleFactors/Run3Summer22/
"""
from hell import HellError
from hell.config import Module

# ParticleNet b-tagging working points, Run3Summer22 (verify against the BTV wiki)
BTAG_WP = {"2022": {"loose": 0.047, "medium": 0.245, "tight": 0.6734},
           "2022EE": {"loose": 0.0499, "medium": 0.2605, "tight": 0.6915}}
SUPPORTED_ERAS = sorted(BTAG_WP)


def check_era(era):
    """The era every definition below is keyed on. Fails loudly instead of silently using a default."""
    if era not in SUPPORTED_ERAS:
        raise HellError(f"era '{era or '(none)'}' is not supported by configs/SingleTopMass/objects_cff.py; "
                        f"give era=... or sample=... with one of: {', '.join(SUPPORTED_ERAS)}")
    return era

TRIGGERS = {"mu": ["HLT_IsoMu24"], "el": ["HLT_Ele30_WPTight_Gsf"]}
MET_FILTERS = ["Flag_goodVertices", "Flag_globalSuperTightHalo2016Filter", "Flag_EcalDeadCellTriggerPrimitiveFilter",
               "Flag_BadPFMuonFilter", "Flag_BadPFMuonDzFilter", "Flag_hfNoisyHitsFilter", "Flag_eeBadScFilter"]


def readers(process):
    process.eventInfo = Module("NanoEventInfoReader")
    process.genParts = Module("NanoGenParticleReader")
    process.muons = Module("NanoMuonReader")
    process.electrons = Module("NanoElectronReader")
    process.jets = Module("NanoJetReader", btagger="btagPNetB")
    process.met = Module("NanoMETReader", prefix="PuppiMET")
    return ["eventInfo", "genParts", "muons", "electrons", "jets", "met"]


def leptons(process, loose=False):
    """Isolated and non-isolated (QCD control) lepton categories plus veto leptons."""
    f = 0.5 if loose else 1.0
    process.goodMuons = Module("MuonSelector", src="muons", minPt=26 * f, maxAbsEta=2.4, id="tight", maxRelIso=0.15,
                               nonIsoMaxRelIso=0.6, vetoMinPt=10, vetoId="loose", vetoMaxRelIso=0.25)
    process.goodElectrons = Module("ElectronSelector", src="electrons", minPt=30 * f, maxAbsEta=2.1, id="tight", maxRelIso=0.10,
                                   nonIsoMaxRelIso=0.6, vetoMinPt=15, vetoId="loose", vetoMaxRelIso=0.25, excludeGap=True)
    process.leptons = Module("LeptonMerger", sources=["goodMuons", "goodElectrons"])
    return ["goodMuons", "goodElectrons", "leptons"]


def jets(process, era, min_pt=30.0, variations=None):
    """Jets cleaned against the selected leptons, b-tagging working points, optional variation producer.
    The jet id is recomputed from the PF fractions (tight + lepton veto): the Run 3 NanoAODv12 Jet_jetId flag
    is unreliable (see Modules/Selection/interface/JetId.h)."""
    check_era(era)
    process.jetVar = Module("JetVariation", src="jets", met="met",
                            variations=variations or {"jesUp": {"scale": 1.03}, "jesDown": {"scale": 0.97},
                                                      "jerUp": {"scale": 1.01}, "jerDown": {"scale": 0.99}})
    process.goodJets = Module("JetSelector", src="jetVar", minPt=min_pt, maxAbsEta=4.7, jetId="tightLepVeto", jetIdSource="manual",
                              cleanAgainst=["leptons"], minDeltaR=0.4)
    wp = BTAG_WP[era]
    process.bTags = Module("BTagger", src="goodJets", loose=wp["loose"], medium=wp["medium"], tight=wp["tight"], maxAbsEta=2.5)
    return ["jetVar", "goodJets", "bTags"]
