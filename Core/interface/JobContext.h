// Job-wide information available to all modules: data/MC, era, the active
// systematic variation, the output file and the job options.
#pragma once
#include "Core/interface/ParameterSet.h"
#include <string>

namespace hell {

struct JobContext {
  std::string processName = "HELL";
  bool isData = false;
  std::string era;            // e.g. "2022", "2022EE", "2018"
  std::string sample;         // sample name from samples/*.yaml, may be empty
  std::string variation = "nominal";  // active object-level variation, e.g. "jesUp"
  double crossSection = -1;   // pb, if known
  ParameterSet options;       // free-form job options
  bool isMC() const { return !isData; }
  // True if the active variation is one of the given names.
  bool variationIs(const std::string& name) const { return variation == name; }
};

}  // namespace hell
