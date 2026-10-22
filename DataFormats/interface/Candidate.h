// Base of every physics object in the HELL data model: a four-vector in the
// (pt, eta, phi, mass) representation plus charge and pdgId. `index` keeps
// the position in the NanoAOD collection it came from, so that any
// NanoAOD branch can be looked up later.
#pragma once
#include "DataFormats/interface/Math.h"

namespace hell {

struct Candidate {
  float pt = 0, eta = 0, phi = 0, mass = 0;
  int charge = 0;
  int pdgId = 0;
  int index = -1;

  Candidate() = default;
  Candidate(float pt_, float eta_, float phi_, float mass_, int charge_ = 0, int pdgId_ = 0, int index_ = -1)
      : pt(pt_), eta(eta_), phi(phi_), mass(mass_), charge(charge_), pdgId(pdgId_), index(index_) {}

  LorentzVector p4() const { return LorentzVector(pt, eta, phi, mass); }
  void setP4(const LorentzVector& v) { pt = v.Pt(); eta = v.Eta(); phi = v.Phi(); mass = v.M(); }
  double energy() const { return p4().E(); }
  double px() const { return pt * std::cos(phi); }
  double py() const { return pt * std::sin(phi); }
  double pz() const { return pt * std::sinh(eta); }
  double p() const { return pt * std::cosh(eta); }
  double absEta() const { return std::fabs(eta); }
  double deltaR(const Candidate& o) const { return hell::deltaR(eta, phi, o.eta, o.phi); }
  double deltaPhi(const Candidate& o) const { return hell::deltaPhi(phi, o.phi); }
};

}  // namespace hell
