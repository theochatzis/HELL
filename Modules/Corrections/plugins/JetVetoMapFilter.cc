// Rejects events with a jet in a detector region of the JME jet veto map (Run 3 recommendation: apply to
// data and MC, on jets with pt > 15 GeV passing the jet id, away from leptons).
// python: process.jetVeto = Module("JetVetoMapFilter", src="goodJets", minPt=15)
#include "Core/interface/ModuleFactory.h"
#include "Corrections/interface/CorrectionService.h"
#include "Corrections/interface/EraCorrections.h"
#include "DataFormats/interface/DataFormats.h"

namespace hell {

class JetVetoMapFilter : public Filter {
 public:
  JetVetoMapFilter(const ParameterSet& ps, ModuleContext& ctx) : Filter(ctx) {
    src_ = ctx.consumesProduct(ps.get<std::string>("src"));
    minPt_ = ps.get<double>("minPt", 15.0);
    type_ = ps.get("type", "jetvetomap");
    const ParameterSet corr = EraCorrections::fromJob(ctx.job());
    map_ = CorrectionService::instance().get(EraCorrections::file(corr, "jetVetoMap"), EraCorrections::name(corr, "jetVetoMap"));
  }
  bool filter(Event& ev) override {
    for (const auto& j : ev.get<JetCollection>(src_)) {
      if (j.pt < minPt_ || j.absEta() > 5.19) continue;
      const double phi = std::max(std::min(static_cast<double>(j.phi), 3.14159), -3.14159);
      if (map_->evaluate({type_, static_cast<double>(j.eta), phi}) > 0) return false;
    }
    return true;
  }
 private:
  std::string src_, type_;
  double minPt_;
  CorrectionService::Ref map_;
};

}  // namespace hell

HELL_REGISTER_MODULE(hell::JetVetoMapFilter, "Rejects events with a jet inside the JME jet veto map regions");
