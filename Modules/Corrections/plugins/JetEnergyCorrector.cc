// Jet energy scale and resolution from the central JME JSON (jet_jerc.json.gz):
//  * JER smearing of MC (hybrid method: scaling with the matched GenJet when it exists and is close,
//    stochastic smearing otherwise; deterministic random numbers from the event id),
//  * JES uncertainty sources as variations: the job variation "jes<Source>Up/Down" (e.g. jesRegrouped_TotalUp)
//    shifts the jets by the uncertainty of that source; "jerUp/jerDown" uses the up/down resolution scale factor,
//  * optional re-application of the JEC from the raw pt (L1L2L3[Res] compound), off by default since
//    NanoAOD already carries the corrections of its campaign,
//  * propagation of every change to the MET (type-1 style: sum of the jet momentum changes).
// Everything (file, tags, algorithm, sources) comes from samples/corrections.yaml through the job.
//
// python:
//   process.jetVar = Module("JetEnergyCorrector", src="goodJets", met="met", eventInfo="eventInfo",
//                           smear=True, genJetPrefix="GenJet", minPtMET=15)
//   hell run analysis_cfg.py ... variation=jesRegrouped_TotalUp     (or jerDown)
#include "Core/interface/Logger.h"
#include "Core/interface/ModuleFactory.h"
#include "Corrections/interface/CorrectionService.h"
#include "Corrections/interface/EraCorrections.h"
#include "DataFormats/interface/DataFormats.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace hell {

class JetEnergyCorrector : public Producer {
 public:
  JetEnergyCorrector(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx), isData_(ctx.job().isData), variation_(ctx.job().variation) {
    src_ = ctx.consumesProduct(ps.get<std::string>("src"));
    met_ = ps.has("met") ? ctx.consumesProduct(ps.get<std::string>("met")) : "";
    info_ = ctx.consumesProduct(ps.get("eventInfo", "eventInfo"));
    smear_ = ps.get<bool>("smear", true) && !isData_;
    reapplyJEC_ = ps.get<bool>("reapplyJEC", false);
    minPtMET_ = ps.get<double>("minPtMET", 15.0);
    const std::string genPrefix = ps.get("genJetPrefix", "GenJet");
    if (smear_) {
      genPt_ = ctx.consumesArray<float>(genPrefix + "_pt", true);
      genEta_ = ctx.consumesArray<float>(genPrefix + "_eta", true);
      genPhi_ = ctx.consumesArray<float>(genPrefix + "_phi", true);
    }
    const ParameterSet corr = EraCorrections::fromJob(ctx.job());
    const std::string file = EraCorrections::file(corr, "jerc");
    const std::string algo = EraCorrections::name(corr, "jetAlgo");
    const std::string jecMC = EraCorrections::name(corr, "jecMC");
    const std::string jer = EraCorrections::name(corr, "jer");
    auto& cs = CorrectionService::instance();
    if (smear_) {
      resolution_ = cs.get(file, jer + "_PtResolution_" + algo);
      resolutionSF_ = cs.get(file, jer + "_ScaleFactor_" + algo);
    }
    // Which variation is mine? jes<Source>Up/Down or jerUp/jerDown.
    jerSystematic_ = "nom";
    if (variation_ == "jerUp" || variation_ == "jerDown") {
      if (!smear_) throw std::runtime_error("JetEnergyCorrector: variation " + variation_ + " needs smear=True on MC");
      jerSystematic_ = variation_ == "jerUp" ? "up" : "down";
    } else if (variation_.rfind("jes", 0) == 0 && variation_.size() > 5) {
      const bool up = variation_.compare(variation_.size() - 2, 2, "Up") == 0;
      const bool down = variation_.compare(variation_.size() - 4, 4, "Down") == 0;
      if (up || down) {
        const std::string source = variation_.substr(3, variation_.size() - 3 - (up ? 2 : 4));
        jesUncertainty_ = cs.get(file, jecMC + "_" + source + "_" + algo);
        jesSign_ = up ? 1.0 : -1.0;
        if (isData_) { jesUncertainty_ = nullptr; HELL_INFO(label()) << "data: JES variation " << variation_ << " not applied"; }
      }
    }
    if (reapplyJEC_) {
      std::string tag = jecMC;
      if (isData_) {
        const ParameterSet data = corr.getPSet("jecData");
        const std::string runEra = ps.get<std::string>("dataRunEra", "");
        if (runEra.empty() || !data.has(runEra)) throw std::runtime_error("JetEnergyCorrector: reapplyJEC on data needs dataRunEra=<letter> present in jecData of samples/corrections.yaml");
        tag = data.get<std::string>(runEra);
      }
      jec_ = cs.getCompound(file, tag + "_L1L2L3Res_" + algo);
    }
    ctx.produces<JetCollection>();
    if (!met_.empty()) metLabel_ = ctx.produces<MET>("met");
    HELL_INFO(label()) << "JES/JER from " << file << " (" << jecMC << ", " << jer << ", " << algo << "); smear=" << smear_
                       << " reapplyJEC=" << reapplyJEC_ << " variation=" << variation_
                       << (jesUncertainty_ ? " [JES source applied]" : "") << (jerSystematic_ != "nom" ? " [JER " + jerSystematic_ + "]" : "");
  }

  void produce(Event& ev) override {
    JetCollection jets = ev.get<JetCollection>(src_);
    const auto& info = ev.get<EventInfo>(info_);
    double dpx = 0, dpy = 0;
    std::mt19937 rng(static_cast<unsigned>(ev.id.event * 2654435761ULL + ev.id.lumi * 97 + ev.id.run));
    std::normal_distribution<double> gaus(0.0, 1.0);
    for (auto& j : jets) {
      const double px0 = j.px(), py0 = j.py();
      double factor = 1.0;
      if (jec_) {
        const double rawPt = j.pt * (1.0 - j.rawFactor);
        const double corrected = jec_->evaluate({static_cast<double>(j.area), static_cast<double>(j.eta), rawPt, static_cast<double>(info.rho)});
        factor *= corrected * rawPt / j.pt;
      }
      const double ptCorr = j.pt * factor;
      if (smear_) {
        const double sigma = resolution_->evaluate({static_cast<double>(j.eta), ptCorr, static_cast<double>(info.rho)});
        const double sf = resolutionSF_->evaluate({static_cast<double>(j.eta), ptCorr, jerSystematic_});
        double c = 1.0;
        bool matched = false;
        if (j.genJetIdx >= 0 && genPt_.valid() && static_cast<size_t>(j.genJetIdx) < genPt_.size()) {
          const double gpt = genPt_[j.genJetIdx];
          const double dr = deltaR(j.eta, j.phi, genEta_[j.genJetIdx], genPhi_[j.genJetIdx]);
          if (dr < 0.2 && std::fabs(ptCorr - gpt) < 3.0 * sigma * ptCorr) { c = 1.0 + (sf - 1.0) * (ptCorr - gpt) / ptCorr; matched = true; }
        }
        if (!matched) { const double g = gaus(rng); c = 1.0 + g * sigma * std::sqrt(std::max(sf * sf - 1.0, 0.0)); }
        factor *= std::max(c, 1e-2);
      }
      if (jesUncertainty_) {
        const double unc = jesUncertainty_->evaluate({static_cast<double>(j.eta), ptCorr});
        factor *= 1.0 + jesSign_ * unc;
      }
      if (factor != 1.0) { j.setP4(j.p4() * factor); j.scale *= static_cast<float>(factor); }
      if (j.pt > minPtMET_ || ptCorr > minPtMET_) { dpx += j.px() - px0; dpy += j.py() - py0; }
    }
    std::stable_sort(jets.begin(), jets.end(), [](const Jet& a, const Jet& b) { return a.pt > b.pt; });
    ev.put(label(), std::move(jets));
    if (!met_.empty()) {
      MET m = ev.get<MET>(met_);
      const double px = m.px() - dpx, py = m.py() - dpy;
      m.pt = static_cast<float>(std::hypot(px, py)); m.phi = static_cast<float>(std::atan2(py, px));
      ev.put(metLabel_, std::move(m));
    }
  }

 private:
  bool isData_, smear_ = false, reapplyJEC_ = false;
  std::string variation_, src_, met_, info_, metLabel_, jerSystematic_;
  double minPtMET_, jesSign_ = 0;
  ArrayHandle<float> genPt_, genEta_, genPhi_;
  CorrectionService::Ref resolution_, resolutionSF_, jesUncertainty_;
  CorrectionService::CompoundRef jec_;
};

}  // namespace hell

HELL_REGISTER_MODULE(hell::JetEnergyCorrector, "JER smearing, JES uncertainty variations and optional JEC re-application from the central JME JSON, propagated to MET");
