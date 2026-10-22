// Kinematic helpers. Units: GeV, radians.
#pragma once
#include <Math/Vector4D.h>
#include <cmath>

namespace hell {
using LorentzVector = ROOT::Math::PtEtaPhiMVector;
using XYZTVector = ROOT::Math::PxPyPzEVector;

inline double deltaPhi(double a, double b) {
  double d = a - b;
  while (d > M_PI) d -= 2 * M_PI;
  while (d <= -M_PI) d += 2 * M_PI;
  return d;
}
inline double deltaR(double eta1, double phi1, double eta2, double phi2) {
  const double dphi = deltaPhi(phi1, phi2), deta = eta1 - eta2;
  return std::sqrt(deta * deta + dphi * dphi);
}
// Transverse mass of a lepton and the missing transverse momentum.
inline double transverseMass(double pt1, double phi1, double pt2, double phi2) {
  return std::sqrt(std::max(0.0, 2.0 * pt1 * pt2 * (1.0 - std::cos(deltaPhi(phi1, phi2)))));
}
}  // namespace hell
