"""SingleTopMass analysis tree: from the skim (or NanoAOD) to the flat tree
with the selected objects, the top candidate, the analysis variables, the
weights (with their variations) and optionally ML scores.

Object-level systematics: run once per variation
    hell run configs/SingleTopMass/analysis_cfg.py inputFiles=skim.root era=2022 variation=jesUp
    hell unleash-systematics configs/SingleTopMass/analysis_cfg.py --local inputFiles=skim.root era=2022
Weight systematics are branches of the Weight table (Weight_puUp ...).
"""
from hell.config import Process, Module, VarParsing
from hell.runner import apply_options
from objects_cff import readers, leptons, jets, TRIGGERS, MET_FILTERS

options = VarParsing()
options.register("model", "", str, "exported MLP (JSON) to evaluate on the analysis variables")
options.register("puWeights", "", str, "ROOT file with pileup ratio histograms (ratio, ratioUp, ratioDown)")
options.register("minJets", 2, int, "minimum number of selected jets")
options.register("maxJets", 3, int, "maximum number of selected jets (-1: none)")
options.register("quickHists", True, bool, "book quick-look histograms")
options.parse()

process = Process("ANALYSIS", output="analysis.root", reportEvery=50000)
apply_options(process, options)
era = process.job["era"] or "2022"

path = readers(process)
process.trigger = Module("TriggerFilter", paths=TRIGGERS["mu"] + TRIGGERS["el"])
process.metFilters = Module("FlagFilter", flags=MET_FILTERS)
path += ["trigger", "metFilters"]
path += leptons(process)
process.oneLepton = Module("CollectionSizeFilter", src="leptons", exact=1)
process.noVetoMuons = Module("CollectionSizeFilter", src="goodMuons:veto", max=0)
process.noVetoElectrons = Module("CollectionSizeFilter", src="goodElectrons:veto", max=0)
path += ["oneLepton", "noVetoMuons", "noVetoElectrons"]
path += jets(process, era)
process.jetCount = Module("CollectionSizeFilter", src="bTags", min=options.minJets, max=options.maxJets)
path += ["jetCount"]
# Weights
process.genWeight = Module("GenWeightProducer")
weight_sources = ["genWeight"]
if options.puWeights:
    process.puWeight = Module("HistogramWeightProducer", name="pu", file=options.puWeights, hist="ratio", histUp="ratioUp", histDown="ratioDown",
                              variable="nTrueInt", eventInfo="eventInfo")
    weight_sources.append("puWeight")
process.weights = Module("WeightCollector", sources=weight_sources)
path += weight_sources + ["weights"]
# Top reconstruction and analysis variables
process.top = Module("TopReconstructor", leptons="leptons", jets="bTags", met="jetVar:met", bTagWP="medium", wMass=80.379)
process.stVars = Module("SingleTopVariables", leptons="leptons", jets="bTags", met="jetVar:met", top="top")
path += ["top", "stVars"]
products = {"eventInfo": "Event", "bTags": "Jet", "leptons": "Lepton", "goodMuons": "Muon", "goodElectrons": "Electron",
            "jetVar:met": "MET", "top": "Top", "weights": "Weight", "stVars": "st"}
if options.model:
    process.mlScore = Module("MLPScoreProducer", model=options.model, table="stVars")
    path += ["mlScore"]
    products["mlScore"] = "ml"
if options.quickHists:
    process.quick = Module("KinematicsAnalyzer", jets="bTags", leptons="leptons", met="jetVar:met")
    path += ["quick"]
# Object validation (data/MC agreement of the reconstruction and of the scale factors), after the weights,
# per channel: the Muon primary dataset is compared with objValMu, EGamma with objValEl.
for ch in ("mu", "el"):
    setattr(process, f"objVal{ch.capitalize()}", Module("ObjectValidationAnalyzer", channel=ch, leptons="leptons", muons="goodMuons",
                                                        electrons="goodElectrons", jets="bTags", met="jetVar:met", eventInfo="eventInfo"))
path += ["objValMu", "objValEl"]
process.tree = Module("TreeWriter", treeName="Events", keep=["run", "luminosityBlock", "event", "HLT_IsoMu24", "HLT_Ele30_WPTight_Gsf"],
                      products=products)
path += ["tree"]
process.path = path
