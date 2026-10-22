hell_add_library(DataFormats
  SOURCES src/Persistence.cc
  DEPENDS HellCore)
hell_add_dictionary(DataFormats
  HEADERS
    DataFormats/interface/Candidate.h DataFormats/interface/Jet.h DataFormats/interface/Lepton.h
    DataFormats/interface/MET.h DataFormats/interface/GenParticle.h DataFormats/interface/EventInfo.h
    DataFormats/interface/Weights.h DataFormats/interface/TopCandidate.h
  LINKDEF DataFormats/interface/LinkDef.h)
