// Flat table of the single-top t-channel analysis variables: the region
// counters (nJet, nBJet) and the discriminating observables, written with
// one row per event so that the histogrammer and the ML tools can use them
// with plain TTree expressions.
//
// python:
//   process.stVars = Module("SingleTopVariables", leptons="leptons", jets="bTags", met="met", top="top")
#include "Core/interface/FlatTable.h"
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/DataFormats.h"

namespace hell {

class SingleTopVariables : public Producer {
 public:
  SingleTopVariables(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx) {
    leptons_ = ctx.consumesProduct(ps.get<std::string>("leptons"));
    jets_ = ctx.consumesProduct(ps.get<std::string>("jets"));
    met_ = ctx.consumesProduct(ps.get<std::string>("met"));
    top_ = ctx.consumesProduct(ps.get<std::string>("top"));
    ctx.produces<FlatTable>();
  }
  void produce(Event& ev) override {
    const auto& leptons = ev.get<LeptonCollection>(leptons_);
    const auto& jets = ev.get<JetCollection>(jets_);
    const auto& met = ev.get<MET>(met_);
    const auto& top = ev.get<TopCandidate>(top_);
    FlatTable t("st", 1, true);
    int nb = 0, nbL = 0, nbT = 0;
    for (const auto& j : jets) { if (j.btagWP >= 2) ++nb; if (j.btagWP >= 1) ++nbL; if (j.btagWP >= 3) ++nbT; }
    auto f = [&](const std::string& n, double v, const std::string& doc) { t.addColumn(n, std::vector<float>{static_cast<float>(v)}, doc); };
    auto i = [&](const std::string& n, int v, const std::string& doc) { t.addColumn(n, std::vector<int>{v}, doc); };
    i("nJet", static_cast<int>(jets.size()), "selected jets");
    i("nBJet", nb, "medium b-tagged jets"); i("nBJetLoose", nbL, "loose b-tagged jets"); i("nBJetTight", nbT, "tight b-tagged jets");
    i("nLepton", static_cast<int>(leptons.size()), "selected leptons");
    const bool hasL = !leptons.empty();
    i("leptonPdgId", hasL ? leptons[0].pdgId : 0, "pdgId of the leading lepton");
    i("isMuon", hasL && leptons[0].isMuon() ? 1 : 0, "leading lepton is a muon");
    i("isIsolated", hasL && leptons[0].isIsolated ? 1 : 0, "leading lepton is isolated");
    f("leptonPt", hasL ? leptons[0].pt : 0, "leading lepton pt"); f("leptonEta", hasL ? leptons[0].eta : 0, "leading lepton eta");
    f("leptonRelIso", hasL ? leptons[0].relIso : 0, "leading lepton relative isolation");
    f("metPt", met.pt, "MET"); f("metPhi", met.phi, "MET phi");
    f("mtw", top.mtw, "transverse W mass");
    const Jet* b = top.bJetIndex >= 0 ? &jets[top.bJetIndex] : nullptr;
    const Jet* q = top.lightJetIndex >= 0 ? &jets[top.lightJetIndex] : nullptr;
    f("bJetPt", b ? b->pt : 0, "b jet pt"); f("bJetEta", b ? b->eta : 0, "b jet eta"); f("bJetScore", b ? b->btagScore : -1, "b jet tag score");
    f("lightJetPt", q ? q->pt : 0, "light (spectator) jet pt"); f("lightJetEta", q ? q->eta : 0, "light jet eta");
    f("lightJetAbsEta", q ? q->absEta() : 0, "|eta| of the light jet"); f("lightJetScore", q ? q->btagScore : -1, "light jet tag score");
    f("topMass", top.mlnub, "m(l nu b)"); f("topPt", top.top.pt, "top candidate pt"); f("topEta", top.top.eta, "top candidate eta");
    f("mlb", top.mlb, "m(l b)"); f("wMass", top.w.mass, "m(l nu)");
    f("nuPz", top.neutrino.pz(), "neutrino pz"); f("cosThetaStar", top.cosThetaStar, "cos theta* (l, q) in top frame");
    f("dRlb", (hasL && b) ? leptons[0].deltaR(*b) : 0, "dR(lepton, b jet)");
    f("dEtaBQ", (b && q) ? std::fabs(b->eta - q->eta) : 0, "|eta(b) - eta(q)|");
    f("mjj", (b && q) ? (b->p4() + q->p4()).M() : 0, "m(b, q)");
    f("ht", [&] { double h = 0; for (const auto& j : jets) h += j.pt; return h; }(), "scalar sum of jet pt");
    i("topValid", top.valid() ? 1 : 0, "top candidate exists");
    i("topComplex", top.complexSolution ? 1 : 0, "neutrino pz had complex solutions");
    ev.put(label(), std::move(t));
  }
 private:
  std::string leptons_, jets_, met_, top_;
};

}  // namespace hell

HELL_REGISTER_MODULE(hell::SingleTopVariables, "One-row table of single-top t-channel analysis variables (regions and observables)");
