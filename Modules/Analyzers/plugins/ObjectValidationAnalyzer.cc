// Object-level validation histograms for data/MC comparisons of the
// reconstruction and of the scale factors.
//
// For each configured collection it fills, per object and per event, the
// distributions that a POG recommendation touches (kinematics, isolation,
// impact parameters, id levels, multiplicity, b-tag scores, MET ...):
//   <label>/<collection>/<variable>        weighted with the event weight as it is at this point of the path
//   <label>/<collection>/<variable>_noSF   same, divided by the scale factors carried by the objects of
//                                          the collection (leptons: scaleFactor; jets: btagSF when set)
// so that one instance placed after the scale-factor producers gives the
// before/after comparison, and `hell validate objects` turns data and MC
// files into agreement metrics (see python/hell/validation.py). It also
// fills profiles of the scale factor versus pt and eta as a check of what
// was applied.
//
// python:
//   process.objVal = Module("ObjectValidationAnalyzer", muons="goodMuons", electrons="goodElectrons",
//                           jets="bTags", met="jetVar:met", eventInfo="eventInfo")
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/DataFormats.h"
#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <map>

namespace hell {

class ObjectValidationAnalyzer : public Analyzer {
 public:
  ObjectValidationAnalyzer(const ParameterSet& ps, ModuleContext& ctx) : Analyzer(ctx) {
    muons_ = ps.has("muons") ? ctx.consumesProduct(ps.get<std::string>("muons")) : "";
    electrons_ = ps.has("electrons") ? ctx.consumesProduct(ps.get<std::string>("electrons")) : "";
    jets_ = ps.has("jets") ? ctx.consumesProduct(ps.get<std::string>("jets")) : "";
    met_ = ps.has("met") ? ctx.consumesProduct(ps.get<std::string>("met")) : "";
    info_ = ps.has("eventInfo") ? ctx.consumesProduct(ps.get<std::string>("eventInfo")) : "";
    maxPt_ = ps.get<double>("maxPt", 300.0);
    HistDir d = ctx.hists();
    if (!muons_.empty()) bookLepton(d.subdir("muons"), "muon", 2.4);
    if (!electrons_.empty()) bookLepton(d.subdir("electrons"), "electron", 2.5);
    if (!jets_.empty()) bookJets(d.subdir("jets"));
    if (!met_.empty() || !info_.empty()) bookEvent(d.subdir("event"));
  }

  void analyze(const Event& ev) override {
    const double w = ev.weight;
    if (!muons_.empty()) fillLeptons(ev.get<MuonCollection>(muons_), "muons", w);
    if (!electrons_.empty()) fillLeptons(ev.get<ElectronCollection>(electrons_), "electrons", w);
    if (!jets_.empty()) fillJets(ev.get<JetCollection>(jets_), w);
    if (!met_.empty()) { const auto& m = ev.get<MET>(met_); fill("event", "metPt", m.pt, w); fill("event", "metPhi", m.phi, w); }
    if (!info_.empty()) { const auto& i = ev.get<EventInfo>(info_); fill("event", "npvsGood", i.npvsGood, w); fill("event", "rho", i.rho, w); }
  }

 private:
  struct Pair { TH1D* h = nullptr; TH1D* noSF = nullptr; };
  void book(HistDir d, const std::string& coll, const std::string& var, const std::string& title, int nb, double lo, double hi, bool withNoSF = true) {
    Pair p;
    p.h = d.book1D(var, (";" + title + ";events").c_str(), nb, lo, hi);
    if (withNoSF) p.noSF = d.book1D(var + "_noSF", (";" + title + " (no scale factors);events").c_str(), nb, lo, hi);
    hists_[coll + "/" + var] = p;
  }
  void fill(const std::string& coll, const std::string& var, double x, double w, double wNoSF = -1) {
    auto it = hists_.find(coll + "/" + var);
    if (it == hists_.end()) return;
    it->second.h->Fill(x, w);
    if (it->second.noSF) it->second.noSF->Fill(x, wNoSF < 0 ? w : wNoSF);
  }
  void bookLepton(HistDir d, const std::string& coll, double etaMax) {
    const std::string c = coll + "s";
    book(d, c, "n", "number of " + c, 5, -0.5, 4.5);
    book(d, c, "pt", coll + " p_{T} [GeV]", 60, 0, maxPt_);
    book(d, c, "eta", coll + " #eta", 50, -etaMax, etaMax);
    book(d, c, "phi", coll + " #phi", 32, -3.2, 3.2);
    book(d, c, "relIso", coll + " relative isolation", 50, 0, 0.6);
    book(d, c, "dxy", coll + " d_{xy} [cm]", 50, -0.1, 0.1);
    book(d, c, "dz", coll + " d_{z} [cm]", 50, -0.2, 0.2);
    book(d, c, "sip3d", coll + " SIP_{3D}", 50, 0, 10);
    book(d, c, "charge", coll + " charge", 3, -1.5, 1.5);
    book(d, c, "idLevel", coll + " id level (0 none, 1 loose, 2 medium, 3 tight)", 4, -0.5, 3.5);
    book(d, c, "isIsolated", coll + " isolated category", 2, -0.5, 1.5);
    book(d, c, "scaleFactor", coll + " scale factor", 60, 0.7, 1.3, false);
    sfVsPt_[c] = d.bookProfile("scaleFactor_vs_pt", (";" + coll + " p_{T} [GeV];mean scale factor").c_str(), 30, 0, maxPt_);
    sfVsEta_[c] = d.bookProfile("scaleFactor_vs_eta", (";" + coll + " #eta;mean scale factor").c_str(), 25, -etaMax, etaMax);
    ptEta_[c] = d.book2D("pt_eta", (";" + coll + " p_{T} [GeV];" + coll + " #eta").c_str(), 30, 0, maxPt_, 25, -etaMax, etaMax);
  }
  template <class L>
  void fillLeptons(const std::vector<L>& leps, const std::string& c, double w) {
    double sfProduct = 1;
    for (const auto& l : leps) sfProduct *= l.scaleFactor;
    const double wNoSF = sfProduct != 0 ? w / sfProduct : w;
    fill(c, "n", leps.size(), w, wNoSF);
    for (const auto& l : leps) {
      fill(c, "pt", l.pt, w, wNoSF); fill(c, "eta", l.eta, w, wNoSF); fill(c, "phi", l.phi, w, wNoSF);
      fill(c, "relIso", l.relIso, w, wNoSF); fill(c, "dxy", l.dxy, w, wNoSF); fill(c, "dz", l.dz, w, wNoSF); fill(c, "sip3d", l.sip3d, w, wNoSF);
      fill(c, "charge", l.charge, w, wNoSF); fill(c, "idLevel", l.tightId ? 3 : l.mediumId ? 2 : l.looseId ? 1 : 0, w, wNoSF);
      fill(c, "isIsolated", l.isIsolated ? 1 : 0, w, wNoSF); fill(c, "scaleFactor", l.scaleFactor, 1.0);
      sfVsPt_[c]->Fill(l.pt, l.scaleFactor); sfVsEta_[c]->Fill(l.eta, l.scaleFactor); ptEta_[c]->Fill(l.pt, l.eta, w);
    }
  }
  void bookJets(HistDir d) {
    const std::string c = "jets";
    book(d, c, "n", "number of jets", 10, -0.5, 9.5);
    book(d, c, "nBJet", "number of medium b jets", 5, -0.5, 4.5);
    book(d, c, "pt", "jet p_{T} [GeV]", 60, 0, maxPt_);
    book(d, c, "eta", "jet #eta", 50, -5, 5);
    book(d, c, "phi", "jet #phi", 32, -3.2, 3.2);
    book(d, c, "mass", "jet mass [GeV]", 40, 0, 80);
    book(d, c, "btagScore", "b tag score", 50, 0, 1);
    book(d, c, "btagWP", "b tag working point (0 none, 1 L, 2 M, 3 T)", 4, -0.5, 3.5);
    book(d, c, "jetId", "jet id bits", 8, -0.5, 7.5);
    book(d, c, "rawFactor", "1 - raw p_{T} / p_{T}", 50, -0.5, 0.5);
    book(d, c, "minDeltaRLepton", "min #DeltaR(jet, lepton)", 50, 0, 5);
    book(d, c, "leadingPt", "leading jet p_{T} [GeV]", 60, 0, maxPt_);
    book(d, c, "ht", "H_{T} [GeV]", 60, 0, 3 * maxPt_);
    ptEta_[c] = d.book2D("pt_eta", ";jet p_{T} [GeV];jet #eta", 30, 0, maxPt_, 25, -5, 5);
  }
  void fillJets(const JetCollection& jets, double w) {
    const std::string c = "jets";
    int nb = 0; double ht = 0;
    for (const auto& j : jets) { if (j.btagWP >= 2) ++nb; ht += j.pt; }
    fill(c, "n", jets.size(), w); fill(c, "nBJet", nb, w); fill(c, "ht", ht, w);
    if (!jets.empty()) fill(c, "leadingPt", jets[0].pt, w);
    for (const auto& j : jets) {
      fill(c, "pt", j.pt, w); fill(c, "eta", j.eta, w); fill(c, "phi", j.phi, w); fill(c, "mass", j.mass, w);
      fill(c, "btagScore", j.btagScore, w); fill(c, "btagWP", j.btagWP, w); fill(c, "jetId", j.jetId, w);
      fill(c, "rawFactor", j.rawFactor, w); fill(c, "minDeltaRLepton", std::min<float>(j.minDeltaRLepton, 4.99f), w);
      ptEta_[c]->Fill(j.pt, j.eta, w);
    }
  }
  void bookEvent(HistDir d) {
    book(d, "event", "metPt", "p_{T}^{miss} [GeV]", 50, 0, maxPt_, false);
    book(d, "event", "metPhi", "#phi^{miss}", 32, -3.2, 3.2, false);
    book(d, "event", "npvsGood", "number of good vertices", 60, -0.5, 59.5, false);
    book(d, "event", "rho", "#rho", 50, 0, 50, false);
  }

  std::string muons_, electrons_, jets_, met_, info_;
  double maxPt_;
  std::map<std::string, Pair> hists_;
  std::map<std::string, TProfile*> sfVsPt_, sfVsEta_;
  std::map<std::string, TH2D*> ptEta_;
};

}  // namespace hell

HELL_REGISTER_MODULE(hell::ObjectValidationAnalyzer, "Object-level distributions (with and without scale factors) for data/MC validation");
