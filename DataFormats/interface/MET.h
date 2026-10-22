#pragma once
#include <cmath>

namespace hell {

struct MET {
  float pt = 0, phi = 0, sumEt = 0;
  float ptNominal = 0, phiNominal = 0;
  double px() const { return pt * std::cos(phi); }
  double py() const { return pt * std::sin(phi); }
};

}  // namespace hell
