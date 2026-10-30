// b-tagging with the fixed working point method, from the central BTV JSON.
//  * BTagEfficiencyAnalyzer: MC tagging efficiencies per flavour in (pt, |eta|): histograms
//    <label>/<flavour>_all and <flavour>_pass (flavour b, c, light), to be merged over the sample and given
//    to the producer, which divides them (the efficiencies depend on the analysis selection, so they are made
//    with the analysis jets: run the analysis config with btagEfficiencyMaps=True on the MC samples and
//    `hell merge` the outputs into one file).
//  * BTagScaleFactorProducer: event weight  prod_tagged SF x prod_untagged (1 - SF eps) / (1 - eps),
//    with the heavy (b, c) and light scale factors varied separately: Weights "btagHeavy", "btagLight".
//    Without an efficiency file the weight is 1 and a warning is printed once, so the chain runs before the
//    efficiencies exist.
//
// python:
//   process.btagEff = Module("BTagEfficiencyAnalyzer", src="bTags", workingPoint="M")
//   process.btagSF  = Module("BTagScaleFactorProducer", src="bTags", workingPoint="M",
//                            efficiencies="btag_efficiencies_2022.root", efficiencyDir="btagEff")
#include "Core/interface/Logger.h"
#include "Core/interface/ModuleFactory.h"
#include "Corrections/interface/CorrectionService.h"
#include "Corrections/interface/EraCorrections.h"
#include "DataFormats/interface/DataFormats.h"
#include <TFile.h>
#include <TH2D.h>
#include <cmath>
#include <memory>

namespace hell {
namespace {
int wpLevel(const std::string& wp) { return wp == "L" ? 1 : wp == "M" ? 2 : wp == "T" ? 3 : 0; }
const char* flavourName(int hadronFlavour) { return hadronFlavour == 5 ? "b" : hadronFlavour == 4 ? "c" : "light"; }
const std::vector<double> kPtEdges = {20, 30, 40, 50, 60, 80, 100, 140, 200, 300, 500, 1000};
const std::vector<double> kEtaEdges = {0.0, 0.5, 1.0, 1.5, 2.0, 2.5};
}  // namespace

class BTagEfficiencyAnalyzer : public Analyzer {
 public:
  BTagEfficiencyAnalyzer(const ParameterSet& ps, ModuleContext& ctx) : Analyzer(ctx), isData_(ctx.job().isData) {
    src_ = ctx.consumesProduct(ps.get<std::string>("src"));
    level_ = wpLevel(ps.get("workingPoint", "M"));
    if (level_ == 0) throw std::runtime_error("BTagEfficiencyAnalyzer: workingPoint must be L, M or T");
    HistDir d = ctx.hists();
    for (const char* f : {"b", "c", "light"}) {
      all_[f] = d.book<TH2D>(std::string(f) + "_all", (std::string(f) + " jets;p_{T} [GeV];|#eta|").c_str(), static_cast<int>(kPtEdges.size()) - 1, kPtEdges.data(), static_cast<int>(kEtaEdges.size()) - 1, kEtaEdges.data());
      pass_[f] = d.book<TH2D>(std::string(f) + "_pass", (std::string(f) + " jets tagged;p_{T} [GeV];|#eta|").c_str(), static_cast<int>(kPtEdges.size()) - 1, kPtEdges.data(), static_cast<int>(kEtaEdges.size()) - 1, kEtaEdges.data());
    }
  }
  void analyze(const Event& ev) override {
    if (isData_) return;
    for (const auto& j : ev.get<JetCollection>(src_)) {
      if (j.absEta() > 2.5) continue;
      const char* f = flavourName(j.hadronFlavour);
      all_[f]->Fill(j.pt, j.absEta(), ev.weight);
      if (j.btagWP >= level_) pass_[f]->Fill(j.pt, j.absEta(), ev.weight);
    }
  }
 private:
  bool isData_;
  std::string src_;
  int level_;
  std::map<std::string, TH2D*> all_, pass_;
};

class BTagScaleFactorProducer : public Producer {
 public:
  BTagScaleFactorProducer(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx), isData_(ctx.job().isData) {
    src_ = ctx.consumesProduct(ps.get<std::string>("src"));
    wp_ = ps.get("workingPoint", "M");
    level_ = wpLevel(wp_);
    if (level_ == 0) throw std::runtime_error("BTagScaleFactorProducer: workingPoint must be L, M or T");
    ctx.produces<Weights>();
    const std::string effFile = ps.get("efficiencies", "");
    const std::string effDir = ps.get("efficiencyDir", "btagEff");
    if (isData_) return;
    const ParameterSet corr = EraCorrections::fromJob(ctx.job());
    const std::string file = EraCorrections::file(corr, "btag");
    auto& cs = CorrectionService::instance();
    heavy_ = cs.get(file, EraCorrections::name(corr, "btagSFHeavy"));
    light_ = cs.get(file, EraCorrections::name(corr, "btagSFLight"));
    if (!effFile.empty()) {
      file_.reset(TFile::Open(effFile.c_str()));
      if (!file_ || file_->IsZombie()) throw std::runtime_error("BTagScaleFactorProducer: cannot open " + effFile);
      for (const char* f : {"b", "c", "light"}) {
        auto* all = dynamic_cast<TH2D*>(file_->Get((effDir + "/" + f + "_all").c_str()));
        auto* pass = dynamic_cast<TH2D*>(file_->Get((effDir + "/" + f + "_pass").c_str()));
        if (!all || !pass) throw std::runtime_error("BTagScaleFactorProducer: histograms " + effDir + "/" + f + "_all|_pass missing in " + effFile);
        auto* eff = static_cast<TH2D*>(pass->Clone((std::string(f) + "_eff").c_str()));
        eff->SetDirectory(nullptr);
        eff->Divide(all);
        eff_[f] = eff;
      }
      HELL_INFO(label()) << "efficiencies from " << effFile << " (" << effDir << "), working point " << wp_;
    } else {
      HELL_WARN(label()) << "no efficiency file given: b-tag weight is 1 (run the analysis config with btagEfficiencyMaps=True on the MC samples and hell merge the outputs)";
    }
  }
  double efficiency(const Jet& j) const {
    const TH2D* h = eff_.at(flavourName(j.hadronFlavour));
    const int bx = std::min(std::max(h->GetXaxis()->FindFixBin(j.pt), 1), h->GetNbinsX());
    const int by = std::min(std::max(h->GetYaxis()->FindFixBin(j.absEta()), 1), h->GetNbinsY());
    return std::min(std::max(h->GetBinContent(bx, by), 1e-3), 1.0 - 1e-3);
  }
  double sf(const Jet& j, const char* syst) const {
    const CorrectionService::Ref& c = (j.hadronFlavour == 5 || j.hadronFlavour == 4) ? heavy_ : light_;
    const double pt = std::min(std::max(static_cast<double>(j.pt), 20.0), 999.0);
    return c->evaluate({std::string(syst), wp_, static_cast<int>(j.hadronFlavour), std::min(j.absEta(), 2.49), pt});
  }
  void produce(Event& ev) override {
    Weights w;
    if (isData_ || eff_.empty()) { w.set("btagHeavy", 1.0); w.set("btagLight", 1.0); ev.put(label(), std::move(w)); return; }
    // weight = prod over jets of (tagged ? SF : (1 - SF eps)/(1 - eps)); heavy and light varied separately
    double nom = 1, heavyUp = 1, heavyDn = 1, lightUp = 1, lightDn = 1;
    for (const auto& j : ev.get<JetCollection>(src_)) {
      if (j.absEta() > 2.5 || j.pt < 20) continue;
      const bool heavy = j.hadronFlavour == 5 || j.hadronFlavour == 4;
      const double eps = efficiency(j);
      auto term = [&](double s) { return j.btagWP >= level_ ? s : (1.0 - s * eps) / (1.0 - eps); };
      const double c = sf(j, "central"), u = sf(j, "up"), d = sf(j, "down");
      nom *= term(c);
      heavyUp *= term(heavy ? u : c); heavyDn *= term(heavy ? d : c);
      lightUp *= term(heavy ? c : u); lightDn *= term(heavy ? c : d);
    }
    w.set("btagHeavy", nom, heavyUp, heavyDn);
    w.set("btagLight", 1.0, lightUp / nom, lightDn / nom);   // relative factor so that the product is the nominal weight
    ev.weight *= nom;
    ev.put(label(), std::move(w));
  }
 private:
  bool isData_;
  std::string src_, wp_;
  int level_;
  CorrectionService::Ref heavy_, light_;
  std::unique_ptr<TFile> file_;
  std::map<std::string, TH2D*> eff_;
};

}  // namespace hell

HELL_REGISTER_MODULE(hell::BTagEfficiencyAnalyzer, "MC b-tagging efficiency maps per flavour in (pt, |eta|) for the fixed-WP scale factor method");
HELL_REGISTER_MODULE(hell::BTagScaleFactorProducer, "b-tag event weight (fixed working point) from the central BTV JSON and MC efficiency maps, heavy and light varied separately");
