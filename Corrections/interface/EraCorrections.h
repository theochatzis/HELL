// The corrections of the era as given to the job (process.job["corrections"], resolved by
// python/hell/corrections.py from samples/corrections.yaml): file paths per key and correction names.
#pragma once
#include "Core/interface/JobContext.h"
#include "Core/interface/ParameterSet.h"
#include <string>

namespace hell {

struct EraCorrections {
  static ParameterSet fromJob(const JobContext& job) {
    if (!job.options.has("corrections"))
      throw std::runtime_error("no corrections for era '" + job.era + "' in the job (samples/corrections.yaml has no entry, or era= is missing)");
    return job.options.getPSet("corrections");
  }
  static std::string file(const ParameterSet& corr, const std::string& key) {
    const ParameterSet files = corr.getPSet("files");
    if (!files.has(key)) throw std::runtime_error("corrections: no file '" + key + "' in samples/corrections.yaml");
    return files.get<std::string>(key);
  }
  static std::string name(const ParameterSet& corr, const std::string& key) {
    if (!corr.has(key)) throw std::runtime_error("corrections: no entry '" + key + "' for this era in samples/corrections.yaml");
    return corr.get<std::string>(key);
  }
};

}  // namespace hell
