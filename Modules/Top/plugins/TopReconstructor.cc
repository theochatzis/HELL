// Reconstructs a semileptonic top candidate: lepton + neutrino + b jet.
// The neutrino pz comes from the W-mass constraint (quadratic equation). If
// the discriminant is negative the real part is used (the standard choice;
// alternatives are configurable later). The b jet is the highest-pt jet
// passing the configured working point; the light jet the highest-|eta|
// untagged jet.
//
// python:
//   process.top = Module("TopReconstructor", leptons="leptons", jets="bTags", met="met",
//                        bTagWP="medium", wMass=80.379)
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/DataFormats.h"
#include <Math/Boost.h>
#include <cmath>

namespace hell {

class TopReconstructor : public Producer {
 public:
  TopReconstructor(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx) {
    leptons_ = ctx.consumesProduct(ps.get<std::string>("leptons"));
    jets_ = ctx.consumesProduct(ps.get<std::string>("jets"));
    met_ = ctx.consumesProduct(ps.get<std::string>("met"));
    const std::string wp = ps.get("bTagWP", "medium");
    wp_ = wp == "tight" ? 3 : wp == "medium" ? 2 : wp == "loose" ? 1 : 0;
    wMass_ = ps.get<double>("wMass", 80.379);
    pzChoice_ = ps.get("pzChoice", "smallest");  // smallest | largest
    ctx.produces<TopCandidate>();
  }
  void produce(Event& ev) override {
    TopCandidate c;
    const auto& leptons = ev.get<LeptonCollection>(leptons_);
    const auto& jets = ev.get<JetCollection>(jets_);
    const auto& met = ev.get<MET>(met_);
    if (!leptons.empty()) {
      c.leptonIndex = 0;
      for (size_t i = 0; i < jets.size(); ++i) if (jets[i].btagWP >= wp_ && c.bJetIndex < 0) c.bJetIndex = static_cast<int>(i);
      for (size_t i = 0; i < jets.size(); ++i) {
        if (static_cast<int>(i) == c.bJetIndex || jets[i].btagWP >= 1) continue;
        if (c.lightJetIndex < 0 || jets[i].absEta() > jets[c.lightJetIndex].absEta()) c.lightJetIndex = static_cast<int>(i);
      }
      const Lepton& l = leptons[0];
      c.mtw = static_cast<float>(transverseMass(l.pt, l.phi, met.pt, met.phi));
      // Neutrino pz from m(l nu) = mW.
      const double mu = wMass_ * wMass_ / 2.0 + l.px() * met.px() + l.py() * met.py();
      const double a = mu * l.pz() / (l.pt * l.pt);
      const double disc = a * a - (l.energy() * l.energy() * met.pt * met.pt - mu * mu) / (l.pt * l.pt);
      c.discriminant = static_cast<float>(disc);
      double pz1 = a, pz2 = a;
      if (disc >= 0) { pz1 = a + std::sqrt(disc); pz2 = a - std::sqrt(disc); } else { c.complexSolution = true; }
      double pz = (std::fabs(pz1) < std::fabs(pz2)) ? pz1 : pz2;
      if (pzChoice_ == "largest") pz = (std::fabs(pz1) > std::fabs(pz2)) ? pz1 : pz2;
      c.nuPzAlternative = static_cast<float>(pz == pz1 ? pz2 : pz1);
      XYZTVector nu(met.px(), met.py(), pz, std::sqrt(met.pt * met.pt + pz * pz));
      c.neutrino.setP4(LorentzVector(nu)); c.neutrino.pdgId = l.pdgId > 0 ? -(std::abs(l.pdgId) + 1) : std::abs(l.pdgId) + 1;
      const LorentzVector wv = l.p4() + c.neutrino.p4();
      c.w.setP4(wv);
      if (c.bJetIndex >= 0) {
        const Jet& b = jets[c.bJetIndex];
        c.mlb = static_cast<float>((l.p4() + b.p4()).M());
        const LorentzVector top = wv + b.p4();
        c.top.setP4(top); c.top.pdgId = l.pdgId > 0 ? -6 : 6; c.mlnub = static_cast<float>(top.M());
        if (c.lightJetIndex >= 0) {
          ROOT::Math::Boost boost(top.BoostToCM());
          const auto lRest = boost(XYZTVector(l.p4()));
          const auto qRest = boost(XYZTVector(jets[c.lightJetIndex].p4()));
          const double num = lRest.Vect().Dot(qRest.Vect());
          const double den = lRest.P() * qRest.P();
          c.cosThetaStar = den > 0 ? static_cast<float>(num / den) : -2.f;
        }
      }
    }
    ev.put(label(), std::move(c));
  }
 private:
  std::string leptons_, jets_, met_, pzChoice_;
  int wp_;
  double wMass_;
};

}  // namespace hell

HELL_REGISTER_MODULE(hell::TopReconstructor, "Semileptonic top candidate from lepton + MET (W-mass pz constraint) + b jet");
