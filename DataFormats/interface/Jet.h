#pragma once
#include "DataFormats/interface/Candidate.h"
#include <vector>

namespace hell {

// b-tagging working point reached by a jet (none < loose < medium < tight).
enum class BTagWP : int { None = 0, Loose = 1, Medium = 2, Tight = 3 };

struct Jet : public Candidate {
  float rawFactor = 0;      // 1 - rawPt/pt (NanoAOD definition)
  float area = 0;
  int jetId = 0;            // NanoAOD jetId bits
  int hadronFlavour = 0;    // 5, 4, 0 (MC only)
  int partonFlavour = 0;
  int genJetIdx = -1;
  float btagDeepFlavB = -1;
  float btagPNetB = -1;
  float btagScore = -1;     // the tagger chosen by configuration
  int btagWP = 0;           // see BTagWP
  float ptNominal = 0;      // pt before any systematic variation
  float scale = 1;          // multiplicative factor applied by variations (JES/JER)
  float minDeltaRLepton = 99;

  bool isBTagged(BTagWP wp = BTagWP::Medium) const { return btagWP >= static_cast<int>(wp); }
  bool passJetId(int bit) const { return (jetId & bit) == bit; }
};

using JetCollection = std::vector<Jet>;

}  // namespace hell
