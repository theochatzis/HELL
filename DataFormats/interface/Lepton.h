// Leptons. Muons and electrons are different physics objects with different
// identification, isolation and corrections, so they have their own types.
// What they share (kinematics, charge, impact parameters, isolation, the
// analysis categories) is in the base `Lepton`. The merged collection used
// by the analysis modules (one selected lepton per event) is a
// std::vector<Lepton> holding the base part; `index` points back to the
// position in the Muon or Electron collection, pdgId tells the flavour.
#pragma once
#include "DataFormats/interface/Candidate.h"
#include <cstdlib>
#include <vector>

namespace hell {

struct Lepton : public Candidate {
  float relIso = 0;         // pfRelIso04_all (mu) / pfRelIso03_all (e)
  float miniIso = 0;        // miniPFRelIso_all
  float dxy = 0, dz = 0, sip3d = 0;
  bool looseId = false, mediumId = false, tightId = false;  // the working points of the flavour's standard id
  bool isIsolated = false;  // analysis isolation category (set by the selector)
  bool isVeto = false;      // passes only the veto selection
  int genPartIdx = -1;      // matched GenPart (MC)
  float scaleFactor = 1;    // product of the lepton scale factors applied

  bool isMuon() const { return std::abs(pdgId) == 13; }
  bool isElectron() const { return std::abs(pdgId) == 11; }
  int flavour() const { return std::abs(pdgId); }
};

struct Muon : public Lepton {
  int pfIsoId = 0;          // PF isolation id level (1 very loose ... 6 very very tight)
  int highPtId = 0;         // 0 none, 1 tracker, 2 global
  bool mediumPromptId = false;
  bool isGlobal = false, isTracker = false, isPFcand = false;
  int nStations = 0;
  float tunepRelPt = 1;     // TuneP pt / pt, high-pt muons
  float ptErr = 0;
};

struct Electron : public Lepton {
  int cutBased = 0;         // cut-based id level: 0 fail, 1 veto, 2 loose, 3 medium, 4 tight
  bool mvaIsoWP80 = false, mvaIsoWP90 = false, mvaNoIsoWP80 = false, mvaNoIsoWP90 = false;
  float etaSC = 0;          // supercluster eta (eta + deltaEtaSC)
  bool convVeto = false;
  int lostHits = 0;
  float r9 = 0, sieie = 0, hoe = 0, eInvMinusPInv = 0;
  float scEtOverPt = 0;
  bool inGap() const { const float a = std::abs(etaSC); return a > 1.4442f && a < 1.566f; }
};

using LeptonCollection = std::vector<Lepton>;
using MuonCollection = std::vector<Muon>;
using ElectronCollection = std::vector<Electron>;

}  // namespace hell
