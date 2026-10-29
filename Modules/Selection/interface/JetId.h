// Jet identification computed from the PF energy fractions ("manual" jet id).
//
// The NanoAOD `Jet_jetId` flag of Run 3 NanoAODv12 is known to be unreliable for PUPPI jets, so the
// identification is recomputed from the stored fractions, following what TopNanoAnalysis does
// (NanoToPico/test/common_utils.py, applyJetID) and the JME recommendations:
//   https://twiki.cern.ch/twiki/bin/view/CMS/JetID13p6TeV
//   https://twiki.cern.ch/twiki/bin/view/CMS/JetID13TeVUL#nanoAOD_Flags
// Criteria (AK4 PUPPI, 13.6 TeV):
//   |eta| <= 2.6        : neHEF < 0.99, neEmEF < 0.90, nConstituents > 1, chHEF > 0.01, [lepton veto: muEF < 0.8, chEmEF < 0.8]
//   2.6 < |eta| <= 2.7  : neHEF < 0.90, neEmEF < 0.99
//   2.7 < |eta| <= 3.0  : neHEF < 0.99
//   |eta| > 3.0         : neEmEF < 0.4, nConstituents >= 2  (neMultiplicity >= 2 when the branch exists)
#pragma once
#include "DataFormats/interface/Jet.h"
#include <cmath>

namespace hell {
namespace jetid {

enum class WorkingPoint { None, Tight, TightLepVeto };

inline bool passManual(const Jet& j, WorkingPoint wp) {
  if (wp == WorkingPoint::None) return true;
  const double aeta = std::fabs(j.eta);
  const bool lepVeto = wp == WorkingPoint::TightLepVeto;
  if (aeta <= 2.6) {
    if (!(j.neHEF < 0.99 && j.neEmEF < 0.90 && j.nConstituents > 1 && j.chHEF > 0.01)) return false;
    if (lepVeto && !(j.muEF < 0.8 && j.chEmEF < 0.8)) return false;
    return true;
  }
  if (aeta <= 2.7) return j.neHEF < 0.90 && j.neEmEF < 0.99;
  if (aeta <= 3.0) return j.neHEF < 0.99;
  const int neutrals = j.neMultiplicity >= 0 ? j.neMultiplicity : j.nConstituents;
  return j.neEmEF < 0.4 && neutrals >= 2;
}

// The NanoAOD flag: bit 1 loose (obsolete), bit 2 tight, bit 4 tight lepton veto.
inline bool passNano(const Jet& j, WorkingPoint wp) {
  if (wp == WorkingPoint::None) return true;
  if (wp == WorkingPoint::Tight) return (j.jetId & 2) == 2;
  return (j.jetId & 6) == 6;
}

inline WorkingPoint workingPoint(const std::string& name) {
  if (name == "none") return WorkingPoint::None;
  if (name == "tight") return WorkingPoint::Tight;
  if (name == "tightLepVeto") return WorkingPoint::TightLepVeto;
  throw std::runtime_error("unknown jet id '" + name + "' (none, tight, tightLepVeto)");
}

}  // namespace jetid
}  // namespace hell
