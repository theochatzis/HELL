// Generator-level distributions BEFORE any selection, for every event of
// the sample. Each variable is histogrammed with the signed generator
// weight and with its absolute value. The ratio sum(w)/sum(|w|) per bin
// is the "positive reweighting" map that replaces negative weights by a
// smooth positive weight that reproduces the same distributions
// (see docs/methodology/positive_reweighting.md). Place it first in the
// skim path so that no selection has been applied.
//
// python:
//   process.genDists = Module("GenDistributionAnalyzer", genParticles="genParts", eventInfo="eventInfo")
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/DataFormats.h"
#include <TH1D.h>
#include <TH2D.h>

namespace hell {

class GenDistributionAnalyzer : public Analyzer {
 public:
  GenDistributionAnalyzer(const ParameterSet& ps, ModuleContext& ctx) : Analyzer(ctx), isData_(ctx.job().isData) {
    gen_ = ctx.consumesProduct(ps.get<std::string>("genParticles"));
    info_ = ctx.consumesProduct(ps.get<std::string>("eventInfo"));
    if (isData_) return;
    HistDir d = ctx.hists();
    hW_ = d.book1D("genWeight", "generator weight;genWeight;events", 200, -2, 2);
    hWsign_ = d.book1D("genWeightSign", "sign of the generator weight;sign;events", 3, -1.5, 1.5);
    book("topPt", "top quark pt [GeV]", 50, 0, 500);
    book("topEta", "top quark eta", 50, -5, 5);
    book("topMass", "top quark mass [GeV]", 100, 150, 200);
    book("leptonPt", "lepton pt [GeV]", 50, 0, 500);
    book("leptonEta", "lepton eta", 50, -5, 5);
    book("wPt", "W boson pt [GeV]", 50, 0, 500);
    book("lheHT", "LHE HT [GeV]", 60, 0, 1200);
    book("nTrueInt", "true pileup interactions", 100, 0, 100);
    h2W_ = d.book2D("topPt_leptonPt", "signed weight;top pt [GeV];lepton pt [GeV]", 50, 0, 1000, 50, 0, 500);
    h2A_ = d.book2D("topPt_leptonPt_abs", "|weight|;top pt [GeV];lepton pt [GeV]", 50, 0, 1000, 50, 0, 500);
  }
  void analyze(const Event& ev) override {
    if (isData_) return;
    const auto& info = ev.get<EventInfo>(info_);
    const double w = info.genWeight;
    hW_->Fill(w); hWsign_->Fill(w < 0 ? -1 : 1);
    const auto& gen = ev.get<GenParticleCollection>(gen_);
    const GenParticle *top = nullptr, *lep = nullptr, *wb = nullptr;
    for (const auto& g : gen) {
      const int id = std::abs(g.pdgId);
      if (id == 6 && g.isLastCopy() && !top) top = &g;
      if (id == 24 && g.isLastCopy() && !wb) wb = &g;
      if ((id == 11 || id == 13) && g.fromHardProcess() && g.isFirstCopy() && !lep) lep = &g;
    }
    if (top) { fill("topPt", top->pt, w); fill("topEta", top->eta, w); fill("topMass", top->mass, w); }
    if (lep) { fill("leptonPt", lep->pt, w); fill("leptonEta", lep->eta, w); }
    if (wb) fill("wPt", wb->pt, w);
    fill("lheHT", info.lheHT, w); fill("nTrueInt", info.nTrueInt, w);
    if (top && lep) { h2W_->Fill(top->pt, lep->pt, w); h2A_->Fill(top->pt, lep->pt, std::fabs(w)); }
  }

 private:
  void book(const std::string& n, const std::string& t, int nb, double lo, double hi) {
    HistDir d = HistDir(hW_->GetDirectory());
    hs_[n] = d.book1D(n, (t + ";" + t + ";weighted events").c_str(), nb, lo, hi);
    hsAbs_[n] = d.book1D(n + "_abs", (t + " (|w|);" + t + ";|w| weighted events").c_str(), nb, lo, hi);
  }
  void fill(const std::string& n, double x, double w) { hs_[n]->Fill(x, w); hsAbs_[n]->Fill(x, std::fabs(w)); }
  bool isData_;
  std::string gen_, info_;
  TH1D *hW_ = nullptr, *hWsign_ = nullptr;
  TH2D *h2W_ = nullptr, *h2A_ = nullptr;
  std::map<std::string, TH1D*> hs_, hsAbs_;
};

}  // namespace hell

HELL_REGISTER_MODULE(hell::GenDistributionAnalyzer, "Gen-level distributions of all events (signed and |w|) for positive reweighting and normalisation");
