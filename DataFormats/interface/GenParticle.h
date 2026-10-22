#pragma once
#include "DataFormats/interface/Candidate.h"
#include <vector>

namespace hell {

struct GenParticle : public Candidate {
  int status = 0;
  int statusFlags = 0;
  int motherIndex = -1;  // index into the GenParticle collection (NanoAOD convention)

  // NanoAOD statusFlags bits
  bool isPrompt() const { return statusFlags & (1 << 0); }
  bool isHardProcess() const { return statusFlags & (1 << 7); }
  bool fromHardProcess() const { return statusFlags & (1 << 8); }
  bool isFirstCopy() const { return statusFlags & (1 << 12); }
  bool isLastCopy() const { return statusFlags & (1 << 13); }
};

using GenParticleCollection = std::vector<GenParticle>;

}  // namespace hell
