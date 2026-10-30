// Pileup reweighting from the central LUM JSON (puWeights.json.gz) as a Weights product "pu" with up/down.
// python: process.puWeight = Module("PileupWeightProducer", eventInfo="eventInfo")
#include "Core/interface/ModuleFactory.h"
#include "Corrections/interface/CorrectionService.h"
#include "Corrections/interface/EraCorrections.h"
#include "DataFormats/interface/DataFormats.h"

namespace hell {

class PileupWeightProducer : public Producer {
 public:
  PileupWeightProducer(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx), isData_(ctx.job().isData) {
    info_ = ctx.consumesProduct(ps.get("eventInfo", "eventInfo"));
    name_ = ps.get("name", "pu");
    ctx.produces<Weights>();
    if (isData_) return;
    const ParameterSet corr = EraCorrections::fromJob(ctx.job());
    weights_ = CorrectionService::instance().get(EraCorrections::file(corr, "pileup"), EraCorrections::name(corr, "pileup"));
  }
  void produce(Event& ev) override {
    Weights w;
    if (isData_) { w.set(name_, 1.0); }
    else {
      const double n = ev.get<EventInfo>(info_).nTrueInt;
      const double nom = weights_->evaluate({n, std::string("nominal")});
      w.set(name_, nom, weights_->evaluate({n, std::string("up")}), weights_->evaluate({n, std::string("down")}));
      ev.weight *= nom;
    }
    ev.put(label(), std::move(w));
  }
 private:
  bool isData_;
  std::string info_, name_;
  CorrectionService::Ref weights_;
};

}  // namespace hell

HELL_REGISTER_MODULE(hell::PileupWeightProducer, "Pileup weight (nominal, up, down) from the central LUM JSON");
