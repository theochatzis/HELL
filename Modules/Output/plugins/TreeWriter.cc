// Writes the output event tree. Put it last in the path: only events that
// survive all filters reach it. Two sources of content:
//  * keep/drop: glob patterns on input branch names, copied unchanged
//    (a NanoAOD skim). Counter branches of kept arrays follow automatically.
//  * products: {"productLabel": "TableName"} written as flat tables
//    (nTableName, TableName_column). Any product with a persistence
//    converter works: collections, EventInfo, Weights, TopCandidate, FlatTable.
// The Runs and LuminosityBlocks trees of the inputs are copied by the
// framework (see Bookkeeper), so the output is self-contained.
//
// python (skim):
//   process.skim = Module("TreeWriter", treeName="Events",
//                         keep=["run","luminosityBlock","event","genWeight","HLT_IsoMu24","Jet_*","Muon_*","Electron_*","PuppiMET_*","GenPart_*","Pileup_nTrueInt","PV_npvsGood","LHE*"],
//                         drop=["Jet_*Calo*"])
// python (analysis tree):
//   process.tree = Module("TreeWriter", products={"bTags":"Jet","leptons":"Lepton","met":"MET","top":"Top","weights":"Weight","stVars":"st"})
#include "Core/interface/Logger.h"
#include "Core/interface/ModuleFactory.h"
#include "Core/interface/OutputTree.h"
#include "Core/interface/Persistence.h"
#include "DataFormats/interface/DataFormats.h"
#include <map>

namespace hell {

class TreeWriter : public Analyzer {
 public:
  TreeWriter(const ParameterSet& ps, ModuleContext& ctx) : Analyzer(ctx) {
    treeName_ = ps.get("treeName", "Events");
    keep_ = ps.get<std::vector<std::string>>("keep", {});
    drop_ = ps.get<std::vector<std::string>>("drop", {});
    const ParameterSet prods = ps.getPSet("products", ParameterSet());
    for (const auto& k : prods.keys()) products_[ctx.consumesProduct(k)] = prods.get<std::string>(k);
    if (keep_.empty() && products_.empty()) throw std::runtime_error("TreeWriter: nothing to write (give keep patterns and/or products)");
    TDirectory* dir = ps.get("directory", "").empty() ? ctx.histService().file() : ctx.histService().dir(ps.get<std::string>("directory")).dir();
    tree_ = std::make_unique<OutputTree>(dir, treeName_, *ctx.inputChain(), ctx.branches(), keep_, drop_);
    ctx.onTreeChange([this](TTree* t) { tree_->onTreeChange(t); });
  }
  void analyze(const Event& ev) override {
    for (const auto& kv : products_) {
      const std::string& label = kv.first;
      // Products are type-erased; find the holder through the known types via getIfExists chain.
      FlatTable table;
      if (auto* p = ev.getIfExists<JetCollection>(label)) table = PersistenceRegistry::instance().convert(ProductHolder<JetCollection>(*p), kv.second);
      else if (auto* p = ev.getIfExists<LeptonCollection>(label)) table = PersistenceRegistry::instance().convert(ProductHolder<LeptonCollection>(*p), kv.second);
      else if (auto* p = ev.getIfExists<MuonCollection>(label)) table = PersistenceRegistry::instance().convert(ProductHolder<MuonCollection>(*p), kv.second);
      else if (auto* p = ev.getIfExists<ElectronCollection>(label)) table = PersistenceRegistry::instance().convert(ProductHolder<ElectronCollection>(*p), kv.second);
      else if (auto* p = ev.getIfExists<GenParticleCollection>(label)) table = PersistenceRegistry::instance().convert(ProductHolder<GenParticleCollection>(*p), kv.second);
      else if (auto* p = ev.getIfExists<MET>(label)) table = PersistenceRegistry::instance().convert(ProductHolder<MET>(*p), kv.second);
      else if (auto* p = ev.getIfExists<EventInfo>(label)) table = PersistenceRegistry::instance().convert(ProductHolder<EventInfo>(*p), kv.second);
      else if (auto* p = ev.getIfExists<Weights>(label)) table = PersistenceRegistry::instance().convert(ProductHolder<Weights>(*p), kv.second);
      else if (auto* p = ev.getIfExists<TopCandidate>(label)) table = PersistenceRegistry::instance().convert(ProductHolder<TopCandidate>(*p), kv.second);
      else if (auto* p = ev.getIfExists<FlatTable>(label)) table = PersistenceRegistry::instance().convert(ProductHolder<FlatTable>(*p), kv.second);
      else if (auto* p = ev.getIfExists<double>(label)) table = PersistenceRegistry::instance().convert(ProductHolder<double>(*p), kv.second);
      else if (auto* p = ev.getIfExists<float>(label)) table = PersistenceRegistry::instance().convert(ProductHolder<float>(*p), kv.second);
      else if (auto* p = ev.getIfExists<int>(label)) table = PersistenceRegistry::instance().convert(ProductHolder<int>(*p), kv.second);
      else if (auto* p = ev.getIfExists<std::vector<float>>(label)) table = PersistenceRegistry::instance().convert(ProductHolder<std::vector<float>>(*p), kv.second);
      else throw std::runtime_error("TreeWriter: product '" + label + "' is missing or has no persistence converter");
      tree_->setTable(table);
    }
    tree_->fill(ev.entry);
  }
  void endJob() override { HELL_INFO(label()) << "wrote " << tree_->entries() << " events to tree " << treeName_; }

 private:
  std::string treeName_;
  std::vector<std::string> keep_, drop_;
  std::map<std::string, std::string> products_;
  std::unique_ptr<OutputTree> tree_;
};

}  // namespace hell

HELL_REGISTER_MODULE(hell::TreeWriter, "Writes the output tree: kept input branches (skim) and/or products as flat tables");
