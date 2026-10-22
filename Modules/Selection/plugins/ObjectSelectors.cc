// Object selectors: take a collection, give back the subset that passes.
//
// python:
//   process.goodMuons     = Module("MuonSelector", src="muons", minPt=26, id="tight", maxRelIso=0.15)
//   process.goodElectrons = Module("ElectronSelector", src="electrons", minPt=30, id="tight", maxRelIso=0.10)
//   process.goodJets  = Module("JetSelector", src="jets", minPt=30, maxAbsEta=4.7, jetIdBit=2,
//                              cleanAgainst=["goodMuons","goodElectrons"], minDeltaR=0.4)
//   process.bTags     = Module("BTagger", src="goodJets", loose=0.0499, medium=0.2605, tight=0.6915)
//   process.leptons   = Module("LeptonMerger", sources=["goodMuons","goodElectrons"])
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/DataFormats.h"
#include <algorithm>

namespace hell {

class JetSelector : public Producer {
 public:
  JetSelector(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx) {
    src_ = ctx.consumesProduct(ps.get<std::string>("src"));
    minPt_ = ps.get<double>("minPt", 30.0);
    maxAbsEta_ = ps.get<double>("maxAbsEta", 4.7);
    jetIdBit_ = ps.get<int>("jetIdBit", 2);  // 2 = tight, 6 = tight + lepton veto
    minDeltaR_ = ps.get<double>("minDeltaR", 0.4);
    for (const auto& l : ps.get<std::vector<std::string>>("cleanAgainst", {})) clean_.push_back(ctx.consumesProduct(l));
    sortByPt_ = ps.get<bool>("sortByPt", true);
    ctx.produces<JetCollection>();
  }
  void produce(Event& ev) override {
    const auto& in = ev.get<JetCollection>(src_);
    std::vector<const Candidate*> leptons;
    for (const auto& l : clean_) {
      if (const auto* c = ev.getIfExists<LeptonCollection>(l)) for (const auto& x : *c) leptons.push_back(&x);
      else if (const auto* c = ev.getIfExists<MuonCollection>(l)) for (const auto& x : *c) leptons.push_back(&x);
      else if (const auto* c = ev.getIfExists<ElectronCollection>(l)) for (const auto& x : *c) leptons.push_back(&x);
      else throw std::runtime_error("JetSelector: cleanAgainst product '" + l + "' is not a lepton collection");
    }
    JetCollection out;
    for (const auto& j : in) {
      if (j.pt < minPt_ || j.absEta() > maxAbsEta_) continue;
      if (jetIdBit_ > 0 && !j.passJetId(jetIdBit_)) continue;
      Jet jj = j;
      for (const auto* lep : leptons) jj.minDeltaRLepton = std::min<float>(jj.minDeltaRLepton, static_cast<float>(jj.deltaR(*lep)));
      if (!leptons.empty() && jj.minDeltaRLepton < minDeltaR_) continue;
      out.push_back(jj);
    }
    if (sortByPt_) std::stable_sort(out.begin(), out.end(), [](const Jet& a, const Jet& b) { return a.pt > b.pt; });
    ev.put(label(), std::move(out));
  }
 private:
  std::string src_;
  std::vector<std::string> clean_;
  double minPt_, maxAbsEta_, minDeltaR_;
  int jetIdBit_;
  bool sortByPt_;
};

// Common lepton category logic: isolated / non-isolated (QCD control) / veto.
template <class L>
struct LeptonCategories {
  double minPt, maxAbsEta, maxRelIso, nonIsoMaxRelIso, vetoMinPt, vetoMaxRelIso;
  template <class IdFn, class VetoIdFn>
  void apply(const std::vector<L>& in, std::vector<L>& sel, std::vector<L>& veto, IdFn passId, VetoIdFn passVetoId) const {
    for (const auto& l : in) {
      L ll = l;
      const bool kin = l.pt > minPt && l.absEta() < maxAbsEta && passId(l);
      if (kin && l.relIso < maxRelIso) { ll.isIsolated = true; sel.push_back(ll); continue; }
      if (kin && nonIsoMaxRelIso > 0 && l.relIso < nonIsoMaxRelIso) { ll.isIsolated = false; sel.push_back(ll); continue; }
      if (l.pt > vetoMinPt && l.absEta() < maxAbsEta && passVetoId(l) && l.relIso < vetoMaxRelIso) { ll.isVeto = true; veto.push_back(ll); }
    }
  }
};

// python: process.goodMuons = Module("MuonSelector", src="muons", minPt=26, maxAbsEta=2.4, id="tight",
//                                    maxRelIso=0.15, nonIsoMaxRelIso=0.6, vetoMinPt=10, vetoId="loose", vetoMaxRelIso=0.25)
// id: loose | medium | mediumPrompt | tight | highPt | none. Isolation by relIso cut, or set pfIsoId (1..6) to use the POG id level.
class MuonSelector : public Producer {
 public:
  MuonSelector(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx) {
    src_ = ctx.consumesProduct(ps.get<std::string>("src"));
    cat_ = {ps.get<double>("minPt", 26.0), ps.get<double>("maxAbsEta", 2.4), ps.get<double>("maxRelIso", 0.15), ps.get<double>("nonIsoMaxRelIso", -1.0),
            ps.get<double>("vetoMinPt", 10.0), ps.get<double>("vetoMaxRelIso", 0.25)};
    id_ = ps.get("id", "tight"); vetoId_ = ps.get("vetoId", "loose");
    pfIsoId_ = ps.get<int>("pfIsoId", 0);
    checkId(id_); checkId(vetoId_);
    ctx.produces<MuonCollection>();
    vetoLabel_ = ctx.produces<MuonCollection>("veto");
  }
  static void checkId(const std::string& id) {
    for (const char* k : {"loose", "medium", "mediumPrompt", "tight", "highPt", "none"}) if (id == k) return;
    throw std::runtime_error("MuonSelector: unknown id '" + id + "' (loose, medium, mediumPrompt, tight, highPt, none)");
  }
  static bool passId(const Muon& m, const std::string& id) {
    if (id == "tight") return m.tightId;
    if (id == "medium") return m.mediumId;
    if (id == "mediumPrompt") return m.mediumPromptId;
    if (id == "loose") return m.looseId;
    if (id == "highPt") return m.highPtId > 0;
    return true;
  }
  void produce(Event& ev) override {
    MuonCollection sel, veto;
    const int pfIso = pfIsoId_;
    cat_.apply(ev.get<MuonCollection>(src_), sel, veto,
               [&](const Muon& m) { return passId(m, id_) && (pfIso <= 0 || m.pfIsoId >= pfIso); },
               [&](const Muon& m) { return passId(m, vetoId_); });
    ev.put(label(), std::move(sel));
    ev.put(vetoLabel_, std::move(veto));
  }
 private:
  std::string src_, id_, vetoId_, vetoLabel_;
  LeptonCategories<Muon> cat_;
  int pfIsoId_;
};

// python: process.goodElectrons = Module("ElectronSelector", src="electrons", minPt=30, maxAbsEta=2.5, id="tight",
//                                        maxRelIso=0.10, nonIsoMaxRelIso=0.6, vetoMinPt=15, vetoId="loose", excludeGap=True)
// id: veto | loose | medium | tight (cut-based) | mvaIsoWP80 | mvaIsoWP90 | mvaNoIsoWP80 | mvaNoIsoWP90 | none.
// |etaSC| is used for the acceptance and the barrel-endcap gap (1.4442-1.566) is excluded by default.
class ElectronSelector : public Producer {
 public:
  ElectronSelector(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx) {
    src_ = ctx.consumesProduct(ps.get<std::string>("src"));
    cat_ = {ps.get<double>("minPt", 30.0), ps.get<double>("maxAbsEta", 2.5), ps.get<double>("maxRelIso", 0.10), ps.get<double>("nonIsoMaxRelIso", -1.0),
            ps.get<double>("vetoMinPt", 15.0), ps.get<double>("vetoMaxRelIso", 0.25)};
    id_ = ps.get("id", "tight"); vetoId_ = ps.get("vetoId", "loose");
    excludeGap_ = ps.get<bool>("excludeGap", true);
    requireConvVeto_ = ps.get<bool>("requireConvVeto", false);
    checkId(id_); checkId(vetoId_);
    ctx.produces<ElectronCollection>();
    vetoLabel_ = ctx.produces<ElectronCollection>("veto");
  }
  static void checkId(const std::string& id) {
    for (const char* k : {"veto", "loose", "medium", "tight", "mvaIsoWP80", "mvaIsoWP90", "mvaNoIsoWP80", "mvaNoIsoWP90", "none"}) if (id == k) return;
    throw std::runtime_error("ElectronSelector: unknown id '" + id + "'");
  }
  static bool passId(const Electron& e, const std::string& id) {
    if (id == "tight") return e.cutBased >= 4;
    if (id == "medium") return e.cutBased >= 3;
    if (id == "loose") return e.cutBased >= 2;
    if (id == "veto") return e.cutBased >= 1;
    if (id == "mvaIsoWP80") return e.mvaIsoWP80;
    if (id == "mvaIsoWP90") return e.mvaIsoWP90;
    if (id == "mvaNoIsoWP80") return e.mvaNoIsoWP80;
    if (id == "mvaNoIsoWP90") return e.mvaNoIsoWP90;
    return true;
  }
  void produce(Event& ev) override {
    ElectronCollection sel, veto;
    auto acc = [&](const Electron& e) { return std::abs(e.etaSC) < cat_.maxAbsEta && !(excludeGap_ && e.inGap()) && (!requireConvVeto_ || e.convVeto); };
    cat_.apply(ev.get<ElectronCollection>(src_), sel, veto,
               [&](const Electron& e) { return acc(e) && passId(e, id_); },
               [&](const Electron& e) { return acc(e) && passId(e, vetoId_); });
    ev.put(label(), std::move(sel));
    ev.put(vetoLabel_, std::move(veto));
  }
 private:
  std::string src_, id_, vetoId_, vetoLabel_;
  LeptonCategories<Electron> cat_;
  bool excludeGap_, requireConvVeto_;
};

// Merges muon and electron collections into one LeptonCollection (the common base part), sorted by pt.
// python: process.leptons = Module("LeptonMerger", sources=["goodMuons", "goodElectrons"])
class LeptonMerger : public Producer {
 public:
  LeptonMerger(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx) {
    for (const auto& s : ps.get<std::vector<std::string>>("sources")) srcs_.push_back(ctx.consumesProduct(s));
    ctx.produces<LeptonCollection>();
  }
  void produce(Event& ev) override {
    LeptonCollection out;
    for (const auto& s : srcs_) {
      if (const auto* m = ev.getIfExists<MuonCollection>(s)) for (const auto& l : *m) out.push_back(static_cast<const Lepton&>(l));
      else if (const auto* e = ev.getIfExists<ElectronCollection>(s)) for (const auto& l : *e) out.push_back(static_cast<const Lepton&>(l));
      else if (const auto* l = ev.getIfExists<LeptonCollection>(s)) out.insert(out.end(), l->begin(), l->end());
      else throw std::runtime_error("LeptonMerger: product '" + s + "' is not a muon, electron or lepton collection");
    }
    std::stable_sort(out.begin(), out.end(), [](const Lepton& a, const Lepton& b) { return a.pt > b.pt; });
    ev.put(label(), std::move(out));
  }
 private:
  std::vector<std::string> srcs_;
};

class BTagger : public Producer {
 public:
  BTagger(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx) {
    src_ = ctx.consumesProduct(ps.get<std::string>("src"));
    loose_ = ps.get<double>("loose"); medium_ = ps.get<double>("medium"); tight_ = ps.get<double>("tight");
    maxAbsEta_ = ps.get<double>("maxAbsEta", 2.5);
    ctx.produces<JetCollection>();
  }
  void produce(Event& ev) override {
    JetCollection out = ev.get<JetCollection>(src_);
    for (auto& j : out) {
      j.btagWP = 0;
      if (j.absEta() > maxAbsEta_ || j.btagScore < 0) continue;
      if (j.btagScore > tight_) j.btagWP = 3; else if (j.btagScore > medium_) j.btagWP = 2; else if (j.btagScore > loose_) j.btagWP = 1;
    }
    ev.put(label(), std::move(out));
  }
 private:
  std::string src_;
  double loose_, medium_, tight_, maxAbsEta_;
};

}  // namespace hell

HELL_REGISTER_MODULE(hell::JetSelector, "Selects jets by pt, |eta|, jet id and cleaning against lepton collections");
HELL_REGISTER_MODULE(hell::MuonSelector, "Selects muons (id, isolated / non-isolated categories) and a veto collection");
HELL_REGISTER_MODULE(hell::ElectronSelector, "Selects electrons (cut-based or MVA id, gap veto, isolation categories) and a veto collection");
HELL_REGISTER_MODULE(hell::LeptonMerger, "Merges muon/electron collections into one LeptonCollection, sorted by pt");
HELL_REGISTER_MODULE(hell::BTagger, "Assigns b-tag working points (loose/medium/tight thresholds) to jets");
