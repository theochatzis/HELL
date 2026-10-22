// Reconstructed semileptonic top quark candidate: lepton + neutrino + b jet.
#pragma once
#include "DataFormats/interface/Candidate.h"

namespace hell {

struct TopCandidate {
  Candidate top;
  Candidate w;
  Candidate neutrino;
  int leptonIndex = -1;    // index in the lepton collection used
  int bJetIndex = -1;      // index in the jet collection used
  int lightJetIndex = -1;  // spectator (forward) jet, if any
  float mtw = 0;           // transverse W mass from lepton + MET
  float mlb = 0;           // invariant mass lepton + b jet
  float mlnub = 0;         // top candidate mass
  float nuPzAlternative = 0;  // the other solution of the quadratic
  float discriminant = 0;  // of the pz quadratic (negative -> complex, real part used)
  bool complexSolution = false;
  float cosThetaStar = -2;  // angle between lepton and light jet in the top rest frame
  bool valid() const { return leptonIndex >= 0 && bJetIndex >= 0; }
};

}  // namespace hell
