// Quick-look histograms of a jet or lepton collection and of the MET.
// Useful to check a selection step without writing a tree.
//
// python:
//   process.jetPlots = Module("KinematicsAnalyzer", jets="bTags", leptons="leptons", met="met")
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/DataFormats.h"
#include <TH1D.h>

namespace hell {

class KinematicsAnalyzer : public Analyzer {
 public:
  KinematicsAnalyzer(const ParameterSet& ps, ModuleContext& ctx) : Analyzer(ctx) {
    jets_ = ps.has("jets") ? ctx.consumesProduct(ps.get<std::string>("jets")) : "";
    leptons_ = ps.has("leptons") ? ctx.consumesProduct(ps.get<std::string>("leptons")) : "";
    met_ = ps.has("met") ? ctx.consumesProduct(ps.get<std::string>("met")) : "";
    HistDir d = ctx.hists();
    if (!jets_.empty()) {
      nJet_ = d.book1D("nJet", ";number of jets;events", 12, -0.5, 11.5);
      jetPt_ = d.book1D("jetPt", ";jet p_{T} [GeV];jets", 60, 0, 300);
      jetEta_ = d.book1D("jetEta", ";jet #eta;jets", 50, -5, 5);
      nBJet_ = d.book1D("nBJet", ";number of medium b jets;events", 6, -0.5, 5.5);
      btag_ = d.book1D("btagScore", ";b tag score;jets", 50, 0, 1);
    }
    if (!leptons_.empty()) {
      nLep_ = d.book1D("nLepton", ";number of leptons;events", 5, -0.5, 4.5);
      lepPt_ = d.book1D("leptonPt", ";lepton p_{T} [GeV];leptons", 60, 0, 300);
      lepEta_ = d.book1D("leptonEta", ";lepton #eta;leptons", 50, -2.5, 2.5);
      lepIso_ = d.book1D("leptonRelIso", ";lepton relative isolation;leptons", 50, 0, 0.5);
    }
    if (!met_.empty()) metPt_ = d.book1D("metPt", ";p_{T}^{miss} [GeV];events", 60, 0, 300);
  }
  void analyze(const Event& ev) override {
    const double w = ev.weight;
    if (!jets_.empty()) {
      const auto& jets = ev.get<JetCollection>(jets_);
      nJet_->Fill(jets.size(), w);
      int nb = 0;
      for (const auto& j : jets) { jetPt_->Fill(j.pt, w); jetEta_->Fill(j.eta, w); btag_->Fill(j.btagScore, w); if (j.btagWP >= 2) ++nb; }
      nBJet_->Fill(nb, w);
    }
    if (!leptons_.empty()) {
      const auto& leps = ev.get<LeptonCollection>(leptons_);
      nLep_->Fill(leps.size(), w);
      for (const auto& l : leps) { lepPt_->Fill(l.pt, w); lepEta_->Fill(l.eta, w); lepIso_->Fill(l.relIso, w); }
    }
    if (!met_.empty()) metPt_->Fill(ev.get<MET>(met_).pt, w);
  }
 private:
  std::string jets_, leptons_, met_;
  TH1D *nJet_ = nullptr, *jetPt_ = nullptr, *jetEta_ = nullptr, *nBJet_ = nullptr, *btag_ = nullptr;
  TH1D *nLep_ = nullptr, *lepPt_ = nullptr, *lepEta_ = nullptr, *lepIso_ = nullptr, *metPt_ = nullptr;
};

}  // namespace hell

HELL_REGISTER_MODULE(hell::KinematicsAnalyzer, "Quick-look histograms of jets, leptons and MET (weighted with the event weight)");
