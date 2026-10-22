// Object-level systematic variations of jets (and MET propagation).
// The job runs with one active variation (job.variation, set with
// `hell run cfg.py --variation jesUp` or by `hell unleash-systematics`).
// This producer applies the factor that belongs to the active variation and
// is transparent for every other one, so the same config serves all
// variations and the output tree of each job is a complete analysis tree.
//
// Sources of the factor, per variation name:
//   "branch": a per-jet NanoAOD branch holding the factor (e.g. from a skim
//             that stored Jet_jesTotalUp, Jet_smearUp...), or
//   "scale":  a constant factor (useful for tests),
//   "formula": not yet; JEC/JER from correctionlib JSONs come next.
//
// python:
//   process.jetVar = Module("JetVariation", src="jets", met="met",
//       variations={"jesUp": {"scale": 1.03}, "jesDown": {"scale": 0.97},
//                   "jerUp": {"branch": "Jet_smearUp"}, "jerDown": {"branch": "Jet_smearDown"}})
#include "Core/interface/Logger.h"
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/DataFormats.h"

namespace hell {

class JetVariation : public Producer {
 public:
  JetVariation(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx), active_(ctx.job().variation) {
    src_ = ctx.consumesProduct(ps.get<std::string>("src"));
    met_ = ps.has("met") ? ctx.consumesProduct(ps.get<std::string>("met")) : "";
    const ParameterSet vars = ps.getPSet("variations", ParameterSet());
    for (const auto& name : vars.keys()) {
      const ParameterSet v = vars.getPSet(name);
      if (name == active_) {
        if (v.has("branch")) { factor_ = ctx.consumesArray<float>(v.get<std::string>("branch")); useBranch_ = true; }
        else scale_ = v.get<double>("scale", 1.0);
        enabled_ = true;
      } else { (void)v.get<std::string>("branch", ""); (void)v.get<double>("scale", 1.0); }
      known_.push_back(name);
    }
    ctx.produces<JetCollection>();
    if (!met_.empty()) metLabel_ = ctx.produces<MET>("met");
    if (enabled_) HELL_INFO(label()) << "applying variation '" << active_ << "'";
    else if (active_ != "nominal") HELL_INFO(label()) << "variation '" << active_ << "' is not one of mine; passing jets through";
  }
  void produce(Event& ev) override {
    JetCollection jets = ev.get<JetCollection>(src_);
    double dpx = 0, dpy = 0;
    if (enabled_) {
      for (auto& j : jets) {
        const double f = useBranch_ ? factor_[static_cast<size_t>(j.index)] : scale_;
        const double px0 = j.px(), py0 = j.py();
        j.setP4(j.p4() * f);
        j.scale *= static_cast<float>(f);
        dpx += j.px() - px0; dpy += j.py() - py0;
      }
    }
    ev.put(label(), std::move(jets));
    if (!met_.empty()) {
      MET m = ev.get<MET>(met_);
      if (enabled_) { const double px = m.px() - dpx, py = m.py() - dpy; m.pt = std::hypot(px, py); m.phi = std::atan2(py, px); }
      ev.put(metLabel_, std::move(m));
    }
  }
  const std::vector<std::string>& variations() const { return known_; }
 private:
  std::string active_, src_, met_, metLabel_;
  std::vector<std::string> known_;
  bool enabled_ = false, useBranch_ = false;
  double scale_ = 1.0;
  ArrayHandle<float> factor_;
};

}  // namespace hell

HELL_REGISTER_MODULE(hell::JetVariation, "Applies the active object-level variation (JES/JER factor) to jets and propagates it to MET");
