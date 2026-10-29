// FlatTable converters for the HELL data model. Every column has a doc
// string: it ends up as the branch title and is shown by `hell navigate`.
#include "Core/interface/Persistence.h"
#include "DataFormats/interface/EventInfo.h"
#include "DataFormats/interface/GenParticle.h"
#include "DataFormats/interface/Jet.h"
#include "DataFormats/interface/Lepton.h"
#include "DataFormats/interface/MET.h"
#include "DataFormats/interface/TopCandidate.h"
#include "DataFormats/interface/Weights.h"

namespace hell {
namespace {

template <class C, class F> std::vector<float> colF(const C& c, F f) { std::vector<float> v; v.reserve(c.size()); for (const auto& x : c) v.push_back(static_cast<float>(f(x))); return v; }
template <class C, class F> std::vector<int> colI(const C& c, F f) { std::vector<int> v; v.reserve(c.size()); for (const auto& x : c) v.push_back(static_cast<int>(f(x))); return v; }
template <class C, class F> std::vector<bool> colB(const C& c, F f) { std::vector<bool> v; v.reserve(c.size()); for (const auto& x : c) v.push_back(static_cast<bool>(f(x))); return v; }

template <class T>
void candidateColumns(FlatTable& t, const std::vector<T>& c) {
  t.addColumn("pt", colF(c, [](const T& x) { return x.pt; }), "transverse momentum [GeV]");
  t.addColumn("eta", colF(c, [](const T& x) { return x.eta; }), "pseudorapidity");
  t.addColumn("phi", colF(c, [](const T& x) { return x.phi; }), "azimuth");
  t.addColumn("mass", colF(c, [](const T& x) { return x.mass; }), "mass [GeV]");
  t.addColumn("charge", colI(c, [](const T& x) { return x.charge; }), "charge");
  t.addColumn("pdgId", colI(c, [](const T& x) { return x.pdgId; }), "PDG id");
  t.addColumn("index", colI(c, [](const T& x) { return x.index; }), "index in the NanoAOD collection");
}

template <class T>
void leptonColumns(FlatTable& t, const std::vector<T>& c) {
  candidateColumns(t, c);
  t.addColumn("relIso", colF(c, [](const T& x) { return x.relIso; }), "relative PF isolation");
  t.addColumn("miniIso", colF(c, [](const T& x) { return x.miniIso; }), "mini PF isolation");
  t.addColumn("dxy", colF(c, [](const T& x) { return x.dxy; }), "dxy [cm]");
  t.addColumn("dz", colF(c, [](const T& x) { return x.dz; }), "dz [cm]");
  t.addColumn("sip3d", colF(c, [](const T& x) { return x.sip3d; }), "3D impact parameter significance");
  t.addColumn("looseId", colB(c, [](const T& x) { return x.looseId; }), "loose id");
  t.addColumn("mediumId", colB(c, [](const T& x) { return x.mediumId; }), "medium id");
  t.addColumn("tightId", colB(c, [](const T& x) { return x.tightId; }), "tight id");
  t.addColumn("isIsolated", colB(c, [](const T& x) { return x.isIsolated; }), "isolated category");
  t.addColumn("isVeto", colB(c, [](const T& x) { return x.isVeto; }), "veto category only");
  t.addColumn("genPartIdx", colI(c, [](const T& x) { return x.genPartIdx; }), "matched GenPart index");
  t.addColumn("scaleFactor", colF(c, [](const T& x) { return x.scaleFactor; }), "product of lepton scale factors");
}

struct Registrar {
  Registrar() {
    auto& r = PersistenceRegistry::instance();
    r.add<JetCollection>([](const JetCollection& c, const std::string& n) {
      FlatTable t(n, c.size());
      candidateColumns(t, c);
      t.addColumn("jetId", colI(c, [](const Jet& x) { return x.jetId; }), "NanoAOD jet id bits");
      t.addColumn("hadronFlavour", colI(c, [](const Jet& x) { return x.hadronFlavour; }), "hadron flavour (MC)");
      t.addColumn("partonFlavour", colI(c, [](const Jet& x) { return x.partonFlavour; }), "parton flavour (MC)");
      t.addColumn("genJetIdx", colI(c, [](const Jet& x) { return x.genJetIdx; }), "matched GenJet index");
      t.addColumn("btagDeepFlavB", colF(c, [](const Jet& x) { return x.btagDeepFlavB; }), "DeepJet b score");
      t.addColumn("btagPNetB", colF(c, [](const Jet& x) { return x.btagPNetB; }), "ParticleNet b score");
      t.addColumn("btagScore", colF(c, [](const Jet& x) { return x.btagScore; }), "b score of the configured tagger");
      t.addColumn("btagWP", colI(c, [](const Jet& x) { return x.btagWP; }), "0 none, 1 loose, 2 medium, 3 tight");
      t.addColumn("ptNominal", colF(c, [](const Jet& x) { return x.ptNominal; }), "pt before systematic variations [GeV]");
      t.addColumn("scale", colF(c, [](const Jet& x) { return x.scale; }), "variation factor applied to the four-vector");
      t.addColumn("rawFactor", colF(c, [](const Jet& x) { return x.rawFactor; }), "1 - rawPt/pt");
      t.addColumn("area", colF(c, [](const Jet& x) { return x.area; }), "jet area");
      t.addColumn("minDeltaRLepton", colF(c, [](const Jet& x) { return x.minDeltaRLepton; }), "min dR to a selected lepton");
      t.addColumn("neHEF", colF(c, [](const Jet& x) { return x.neHEF; }), "neutral hadron energy fraction");
      t.addColumn("neEmEF", colF(c, [](const Jet& x) { return x.neEmEF; }), "neutral EM energy fraction");
      t.addColumn("chHEF", colF(c, [](const Jet& x) { return x.chHEF; }), "charged hadron energy fraction");
      t.addColumn("chEmEF", colF(c, [](const Jet& x) { return x.chEmEF; }), "charged EM energy fraction");
      t.addColumn("muEF", colF(c, [](const Jet& x) { return x.muEF; }), "muon energy fraction");
      t.addColumn("nConstituents", colI(c, [](const Jet& x) { return x.nConstituents; }), "number of constituents");
      t.addColumn("passJetIdManual", colB(c, [](const Jet& x) { return x.passJetIdManual; }), "passed the manual jet id applied by the selector");
      return t;
    });
    r.add<LeptonCollection>([](const LeptonCollection& c, const std::string& n) { FlatTable t(n, c.size()); leptonColumns(t, c); return t; });
    r.add<MuonCollection>([](const MuonCollection& c, const std::string& n) {
      FlatTable t(n, c.size());
      leptonColumns(t, c);
      t.addColumn("pfIsoId", colI(c, [](const Muon& x) { return x.pfIsoId; }), "PF isolation id level (1..6)");
      t.addColumn("highPtId", colI(c, [](const Muon& x) { return x.highPtId; }), "high-pt id: 0 none, 1 tracker, 2 global");
      t.addColumn("mediumPromptId", colB(c, [](const Muon& x) { return x.mediumPromptId; }), "medium prompt id");
      t.addColumn("isGlobal", colB(c, [](const Muon& x) { return x.isGlobal; }), "global muon");
      t.addColumn("isTracker", colB(c, [](const Muon& x) { return x.isTracker; }), "tracker muon");
      t.addColumn("isPFcand", colB(c, [](const Muon& x) { return x.isPFcand; }), "PF candidate");
      t.addColumn("nStations", colI(c, [](const Muon& x) { return x.nStations; }), "matched stations");
      t.addColumn("tunepRelPt", colF(c, [](const Muon& x) { return x.tunepRelPt; }), "TuneP pt / pt");
      t.addColumn("ptErr", colF(c, [](const Muon& x) { return x.ptErr; }), "pt uncertainty [GeV]");
      return t;
    });
    r.add<ElectronCollection>([](const ElectronCollection& c, const std::string& n) {
      FlatTable t(n, c.size());
      leptonColumns(t, c);
      t.addColumn("cutBased", colI(c, [](const Electron& x) { return x.cutBased; }), "cut-based id level (0 fail .. 4 tight)");
      t.addColumn("mvaIsoWP80", colB(c, [](const Electron& x) { return x.mvaIsoWP80; }), "MVA iso WP80");
      t.addColumn("mvaIsoWP90", colB(c, [](const Electron& x) { return x.mvaIsoWP90; }), "MVA iso WP90");
      t.addColumn("mvaNoIsoWP80", colB(c, [](const Electron& x) { return x.mvaNoIsoWP80; }), "MVA no-iso WP80");
      t.addColumn("mvaNoIsoWP90", colB(c, [](const Electron& x) { return x.mvaNoIsoWP90; }), "MVA no-iso WP90");
      t.addColumn("etaSC", colF(c, [](const Electron& x) { return x.etaSC; }), "supercluster eta");
      t.addColumn("convVeto", colB(c, [](const Electron& x) { return x.convVeto; }), "conversion veto");
      t.addColumn("lostHits", colI(c, [](const Electron& x) { return x.lostHits; }), "missing inner hits");
      t.addColumn("r9", colF(c, [](const Electron& x) { return x.r9; }), "R9");
      t.addColumn("sieie", colF(c, [](const Electron& x) { return x.sieie; }), "sigma ieta ieta");
      t.addColumn("hoe", colF(c, [](const Electron& x) { return x.hoe; }), "H/E");
      t.addColumn("eInvMinusPInv", colF(c, [](const Electron& x) { return x.eInvMinusPInv; }), "1/E - 1/p");
      t.addColumn("scEtOverPt", colF(c, [](const Electron& x) { return x.scEtOverPt; }), "SC Et / pt - 1");
      return t;
    });
    r.add<GenParticleCollection>([](const GenParticleCollection& c, const std::string& n) {
      FlatTable t(n, c.size());
      candidateColumns(t, c);
      t.addColumn("status", colI(c, [](const GenParticle& x) { return x.status; }), "generator status");
      t.addColumn("statusFlags", colI(c, [](const GenParticle& x) { return x.statusFlags; }), "NanoAOD status flags");
      t.addColumn("motherIndex", colI(c, [](const GenParticle& x) { return x.motherIndex; }), "mother index in this collection");
      return t;
    });
    r.add<MET>([](const MET& m, const std::string& n) {
      FlatTable t(n, 1, true);
      t.addColumn("pt", std::vector<float>{m.pt}, "missing transverse momentum [GeV]");
      t.addColumn("phi", std::vector<float>{m.phi}, "MET azimuth");
      t.addColumn("sumEt", std::vector<float>{m.sumEt}, "scalar sum Et");
      t.addColumn("ptNominal", std::vector<float>{m.ptNominal}, "MET before variations");
      return t;
    });
    r.add<EventInfo>([](const EventInfo& e, const std::string& n) {
      FlatTable t(n, 1, true);
      t.addColumn("genWeight", std::vector<float>{e.genWeight}, "generator weight");
      t.addColumn("rho", std::vector<float>{e.rho}, "rho");
      t.addColumn("npvs", std::vector<int>{e.npvs}, "number of primary vertices");
      t.addColumn("npvsGood", std::vector<int>{e.npvsGood}, "number of good primary vertices");
      t.addColumn("nTrueInt", std::vector<float>{e.nTrueInt}, "true pileup interactions");
      t.addColumn("lheHT", std::vector<float>{e.lheHT}, "LHE HT");
      t.addColumn("lheNjets", std::vector<int>{e.lheNjets}, "LHE number of jets");
      return t;
    });
    r.add<Weights>([](const Weights& w, const std::string& n) {
      FlatTable t(n, 1, true);
      t.addColumn("nominal", std::vector<double>{w.nominal()}, "product of all nominal weights");
      for (const auto& e : w.entries()) {
        t.addColumn(e.name, std::vector<double>{e.nominal}, "weight " + e.name);
        if (e.hasVariation) {
          t.addColumn(e.name + "Up", std::vector<double>{w.varied(e.name, true)}, "total weight with " + e.name + " up");
          t.addColumn(e.name + "Down", std::vector<double>{w.varied(e.name, false)}, "total weight with " + e.name + " down");
        }
      }
      return t;
    });
    r.add<TopCandidate>([](const TopCandidate& c, const std::string& n) {
      FlatTable t(n, 1, true);
      t.addColumn("valid", std::vector<bool>{c.valid()}, "candidate was built");
      t.addColumn("mass", std::vector<float>{c.mlnub}, "m(l nu b) [GeV]");
      t.addColumn("pt", std::vector<float>{c.top.pt}, "top pt [GeV]");
      t.addColumn("eta", std::vector<float>{c.top.eta}, "top eta");
      t.addColumn("phi", std::vector<float>{c.top.phi}, "top phi");
      t.addColumn("mtw", std::vector<float>{c.mtw}, "transverse W mass [GeV]");
      t.addColumn("mlb", std::vector<float>{c.mlb}, "m(l b) [GeV]");
      t.addColumn("wMass", std::vector<float>{c.w.mass}, "reconstructed W mass [GeV]");
      t.addColumn("nuPt", std::vector<float>{c.neutrino.pt}, "neutrino pt [GeV]");
      t.addColumn("nuEta", std::vector<float>{c.neutrino.eta}, "neutrino eta");
      t.addColumn("nuPz", std::vector<float>{static_cast<float>(c.neutrino.pz())}, "neutrino pz [GeV]");
      t.addColumn("nuPzAlternative", std::vector<float>{c.nuPzAlternative}, "other pz solution [GeV]");
      t.addColumn("discriminant", std::vector<float>{c.discriminant}, "discriminant of the pz quadratic");
      t.addColumn("complexSolution", std::vector<bool>{c.complexSolution}, "quadratic had no real solution");
      t.addColumn("cosThetaStar", std::vector<float>{c.cosThetaStar}, "cos of lepton-light jet angle in the top rest frame");
      t.addColumn("leptonIndex", std::vector<int>{c.leptonIndex}, "lepton used");
      t.addColumn("bJetIndex", std::vector<int>{c.bJetIndex}, "b jet used");
      t.addColumn("lightJetIndex", std::vector<int>{c.lightJetIndex}, "light jet used");
      return t;
    });
  }
};
const Registrar registrar_;

}  // namespace
// Called by executables to make sure this library (and its converters) is linked in.
void loadDataFormats() { (void)registrar_; }
}  // namespace hell
