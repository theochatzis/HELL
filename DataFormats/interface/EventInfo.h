#pragma once

namespace hell {

// Event-level quantities from NanoAOD that most modules need.
struct EventInfo {
  unsigned int run = 0;
  unsigned int lumi = 0;
  unsigned long long event = 0;
  bool isData = false;
  float genWeight = 1;
  float rho = 0;
  int npvs = 0;
  int npvsGood = 0;
  float nTrueInt = 0;
  float lheHT = 0;
  int lheNjets = 0;
};

}  // namespace hell
