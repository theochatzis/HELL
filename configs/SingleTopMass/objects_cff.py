"""Object definitions shared by the SingleTopMass configurations.

Everything that defines what a "jet", a "lepton" or a "b tag" is lives here, so that the skim, the analysis
tree and any study use the same definitions. Working points, scale factors, JES/JER and pileup weights come
from the central POG correctionlib JSONs listed per era in samples/corrections.yaml (`hell corrections --era 2022`);
nothing is typed here.
"""
from hell import HellError
from hell.config import Module
from hell.corrections import eras as correction_eras

SUPPORTED_ERAS = sorted(correction_eras())


def check_era(era):
    """The era every definition below is keyed on. Fails loudly instead of silently using a default."""
    if era not in SUPPORTED_ERAS:
        raise HellError(f"era '{era or '(none)'}' is not supported by configs/SingleTopMass/objects_cff.py; "
                        f"give era=... or sample=... with one of: {', '.join(SUPPORTED_ERAS)} (samples/corrections.yaml)")
    return era


TRIGGERS = {"mu": ["HLT_IsoMu24"], "el": ["HLT_Ele30_WPTight_Gsf"]}
MET_FILTERS = ["Flag_goodVertices", "Flag_globalSuperTightHalo2016Filter", "Flag_EcalDeadCellTriggerPrimitiveFilter",
               "Flag_BadPFMuonFilter", "Flag_BadPFMuonDzFilter", "Flag_hfNoisyHitsFilter", "Flag_eeBadScFilter"]


def nano(process):
    """Settings of the NanoAOD version declared for the job (samples/nanoaod.yaml); fails if none is known."""
    settings = process.job.get("nano")
    if not settings:
        raise HellError("no NanoAOD version for this job: give nanoVersion=<N> or a sample with nanoVersion in samples/<era>.yaml "
                        f"(known versions: see samples/nanoaod.yaml)")
    return settings


def readers(process):
    """NanoAOD readers, with the branch choices of the declared NanoAOD version."""
    n = nano(process)
    process.eventInfo = Module("NanoEventInfoReader")
    process.genParts = Module("NanoGenParticleReader")
    process.muons = Module("NanoMuonReader")
    process.electrons = Module("NanoElectronReader", etaSC=n["electronEtaSC"])
    process.jets = Module("NanoJetReader", btagger=n["btagger"])
    process.met = Module("NanoMETReader", prefix=n["metForAnalysis"])
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


def jets(process, era, min_pt=30.0, apply_veto_map=True):
    """Jets cleaned against the selected leptons, JES/JER from the central JME JSON (JER smearing of MC;
    the job variation jes<Source>Up/Down or jerUp/Down selects the shift), b-tag working points from the
    central BTV JSON, optional jet veto map. The jet id source follows the NanoAOD version: recomputed from
    the PF fractions for v12 (the flag is unreliable, see Modules/Selection/interface/JetId.h), the central
    JME JSON from v13 on."""
    check_era(era)
    jet_id_source = nano(process)["jetIdSource"]   # manual for NanoAODv12, the central JSON when the multiplicities exist
    process.goodJets = Module("JetSelector", src="jets", minPt=min_pt * 0.8, maxAbsEta=4.7, jetId="tightLepVeto", jetIdSource=jet_id_source,
                              cleanAgainst=["leptons"], minDeltaR=0.4)
    process.jetVar = Module("JetEnergyCorrector", src="goodJets", met="met", eventInfo="eventInfo", smear=True, minPtMET=15)
    process.corrJets = Module("JetSelector", src="jetVar", minPt=min_pt, maxAbsEta=4.7, jetId="none", cleanAgainst=[])
    process.bTags = Module("BTagger", src="corrJets", maxAbsEta=2.5)
    path = ["goodJets", "jetVar", "corrJets", "bTags"]
    if apply_veto_map:
        process.jetVeto = Module("JetVetoMapFilter", src="corrJets", minPt=15)
        path.append("jetVeto")
    return path


def weights(process, pileup=True, lepton_sf=True, btag_sf=True, btag_efficiencies=""):
    """Event weights with their variations: generator, pileup, lepton and b-tag scale factors, all from the
    central JSONs. Returns the list of Weights products to collect."""
    process.genWeight = Module("GenWeightProducer")
    sources = ["genWeight"]
    if pileup:
        process.puWeight = Module("PileupWeightProducer", eventInfo="eventInfo")
        sources.append("puWeight")
    if lepton_sf:
        process.muonSF = Module("MuonScaleFactorProducer", src="goodMuons", applyTrigger=True)
        process.electronSF = Module("ElectronScaleFactorProducer", src="goodElectrons", applyTrigger=True)
        sources += ["muonSF:weights", "electronSF:weights"]
    if btag_sf:
        process.btagSF = Module("BTagScaleFactorProducer", src="bTags", workingPoint="M", efficiencies=btag_efficiencies)
        sources.append("btagSF")
    process.weights = Module("WeightCollector", sources=sources)
    return [s.split(":")[0] for s in sources] + ["weights"]
