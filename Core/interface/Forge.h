// The Forge is the HELL event loop: it reads the job configuration, builds
// the modules of the path, runs every event through them and writes the
// output (histograms, trees, bookkeeping, provenance).
//
// Configuration (JSON, normally generated from a python config):
// {
//   "process": "SKIM",
//   "input":  {"files": [...], "treeName": "Events", "maxEvents": -1, "skipEvents": 0},
//   "output": {"file": "out.root", "copyRuns": true, "copyLumis": true},
//   "job":    {"isData": false, "era": "2022", "sample": "...", "variation": "nominal", "crossSection": 145.0},
//   "options":{"logLevel": "INFO", "reportEvery": 10000, "pluginDirs": []},
//   "modules":{"label": {"@type": "ModuleType", ...parameters}},
//   "path":   ["label1", "label2", ...]
// }
#pragma once
#include "Core/interface/Bookkeeper.h"
#include "Core/interface/BranchRegistry.h"
#include "Core/interface/Event.h"
#include "Core/interface/HistService.h"
#include "Core/interface/InputSource.h"
#include "Core/interface/JobContext.h"
#include "Core/interface/Module.h"
#include "Core/interface/ParameterSet.h"
#include "Core/interface/ProductRegistry.h"
#include "Core/interface/Provenance.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace hell {

class Forge {
 public:
  explicit Forge(const ParameterSet& config);
  ~Forge();

  // Overrides from the command line (applied before initialize()).
  void setMaxEvents(long long n) { maxEvents_ = n; }
  void setSkipEvents(long long n) { skipEvents_ = n; }
  void setInputFiles(const std::vector<std::string>& files) { inputOverride_ = files; }
  void setOutputFile(const std::string& f) { outputOverride_ = f; }
  void setVariation(const std::string& v) { variationOverride_ = v; }
  void setCommandLine(int argc, char** argv) { provenance_.setCommandLine(argc, argv); }

  void initialize();
  int run();
  std::string summary() const;
  nlohmann::json describe() const;  // modules, parameters, products (for --dump / navigator)

  const JobContext& job() const { return job_; }
  const ParameterSet& config() const { return config_; }

 private:
  struct PathEntry {
    std::string label;
    std::unique_ptr<ModuleBase> module;
    std::unique_ptr<ModuleContext> context;
    ParameterSet params;
    long long passed = 0;
  };
  void buildModules();
  void validateDependencies() const;

  ParameterSet config_;
  JobContext job_;
  long long maxEvents_ = -1, skipEvents_ = 0;
  std::vector<std::string> inputOverride_;
  std::string outputOverride_, variationOverride_;
  std::string outputFile_;
  long long reportEvery_ = 10000;

  std::unique_ptr<InputSource> input_;
  std::unique_ptr<HistService> hists_;
  std::unique_ptr<Bookkeeper> bookkeeper_;
  BranchRegistry branches_;
  ProductRegistry products_;
  Provenance provenance_;
  std::vector<PathEntry> path_;
  std::vector<std::function<void(TTree*)>> treeListeners_;
  ValueHandle<unsigned int> runH_, lumiH_;
  ValueHandle<unsigned long long> eventH_;
  ValueHandle<float> genWeightH_;
  bool initialized_ = false;
};

}  // namespace hell
