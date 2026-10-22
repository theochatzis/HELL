// Event filters. A filter returning false stops the path for that event;
// the cutflow records how many events passed each one.
//
// python:
//   process.trigger   = Module("TriggerFilter", paths=["HLT_IsoMu24", "HLT_Ele30_WPTight_Gsf"])
//   process.metFilters= Module("FlagFilter", flags=["Flag_goodVertices", "Flag_globalSuperTightHalo2016Filter"])
//   process.oneLepton = Module("CollectionSizeFilter", src="leptons", min=1, max=1)
//   process.noVeto    = Module("CollectionSizeFilter", src="goodMuons:veto", max=0)
//   process.golden    = Module("GoldenJSONFilter", json="/eos/user/c/cmsdqm/www/CAF/certification/Collisions22/Cert_Collisions2022_355100_362760_Golden.json")
#include "Core/interface/Logger.h"
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/DataFormats.h"
#include <fstream>
#include <map>

namespace hell {

class TriggerFilter : public Filter {
 public:
  TriggerFilter(const ParameterSet& ps, ModuleContext& ctx) : Filter(ctx) {
    for (const auto& p : ps.get<std::vector<std::string>>("paths")) bits_.push_back(ctx.consumesValue<bool>(p, ps.get<bool>("optional", true)));
    if (bits_.empty()) throw std::runtime_error("TriggerFilter needs at least one path");
    requireAll_ = ps.get<bool>("requireAll", false);
  }
  bool filter(Event&) override {
    bool any = false, all = true;
    for (const auto& b : bits_) { const bool v = b.valid() && *b; any |= v; all &= v; }
    return requireAll_ ? all : any;
  }
 private:
  std::vector<ValueHandle<bool>> bits_;
  bool requireAll_;
};

class FlagFilter : public Filter {
 public:
  FlagFilter(const ParameterSet& ps, ModuleContext& ctx) : Filter(ctx) {
    for (const auto& f : ps.get<std::vector<std::string>>("flags")) bits_.push_back(ctx.consumesValue<bool>(f, ps.get<bool>("optional", true)));
  }
  bool filter(Event&) override {
    for (const auto& b : bits_) if (b.valid() && !*b) return false;
    return true;
  }
 private:
  std::vector<ValueHandle<bool>> bits_;
};

class CollectionSizeFilter : public Filter {
 public:
  CollectionSizeFilter(const ParameterSet& ps, ModuleContext& ctx) : Filter(ctx) {
    src_ = ctx.consumesProduct(ps.get<std::string>("src"));
    min_ = ps.get<int>("min", 0); max_ = ps.get<int>("max", -1);
    exact_ = ps.get<int>("exact", -1);
  }
  bool filter(Event& ev) override {
    size_t n = 0;
    if (const auto* j = ev.getIfExists<JetCollection>(src_)) n = j->size();
    else if (const auto* l = ev.getIfExists<LeptonCollection>(src_)) n = l->size();
    else if (const auto* m = ev.getIfExists<MuonCollection>(src_)) n = m->size();
    else if (const auto* e = ev.getIfExists<ElectronCollection>(src_)) n = e->size();
    else if (const auto* g = ev.getIfExists<GenParticleCollection>(src_)) n = g->size();
    else throw std::runtime_error("CollectionSizeFilter: product '" + src_ + "' is not a known collection type");
    if (exact_ >= 0) return static_cast<int>(n) == exact_;
    if (static_cast<int>(n) < min_) return false;
    if (max_ >= 0 && static_cast<int>(n) > max_) return false;
    return true;
  }
 private:
  std::string src_;
  int min_, max_, exact_;
};

// Certified luminosity sections (Golden JSON). Transparent for MC.
class GoldenJSONFilter : public Filter {
 public:
  GoldenJSONFilter(const ParameterSet& ps, ModuleContext& ctx) : Filter(ctx), isData_(ctx.job().isData) {
    const std::string path = ps.get<std::string>("json");
    applyToMC_ = ps.get<bool>("applyToMC", false);
    if (!isData_ && !applyToMC_) return;
    std::ifstream in(path);
    if (!in) throw std::runtime_error("GoldenJSONFilter: cannot open " + path);
    nlohmann::json j; in >> j;
    for (auto it = j.begin(); it != j.end(); ++it) {
      const unsigned run = std::stoul(it.key());
      for (const auto& r : it.value()) ranges_[run].emplace_back(r.at(0).get<unsigned>(), r.at(1).get<unsigned>());
    }
    HELL_INFO(label()) << "loaded " << ranges_.size() << " certified runs from " << path;
  }
  bool filter(Event& ev) override {
    if (!isData_ && !applyToMC_) return true;
    auto it = ranges_.find(ev.id.run);
    if (it == ranges_.end()) return false;
    for (const auto& r : it->second) if (ev.id.lumi >= r.first && ev.id.lumi <= r.second) return true;
    return false;
  }
 private:
  bool isData_, applyToMC_ = false;
  std::map<unsigned, std::vector<std::pair<unsigned, unsigned>>> ranges_;
};

// Keep every N-th event (e.g. to store an unbiased sample of gen-level info).
class PrescaleFilter : public Filter {
 public:
  PrescaleFilter(const ParameterSet& ps, ModuleContext& ctx) : Filter(ctx), prescale_(ps.get<int>("prescale", 1)) {}
  bool filter(Event&) override { return prescale_ <= 1 || (counter_++ % prescale_) == 0; }
 private:
  int prescale_; long long counter_ = 0;
};

}  // namespace hell

HELL_REGISTER_MODULE(hell::TriggerFilter, "Passes events firing any (or all) of the given HLT paths");
HELL_REGISTER_MODULE(hell::FlagFilter, "Requires all given event flags (MET filters) to be true");
HELL_REGISTER_MODULE(hell::CollectionSizeFilter, "Requires min <= size(collection) <= max (or exact)");
HELL_REGISTER_MODULE(hell::GoldenJSONFilter, "Keeps only certified run/lumi sections (data only unless applyToMC)");
HELL_REGISTER_MODULE(hell::PrescaleFilter, "Keeps one event every N");
