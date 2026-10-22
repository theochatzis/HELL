// Provenance: what produced this file, with what, from what.
// Stored as a TTree "hell/provenance" with a single string branch "json",
// one entry per job. hadd concatenates entries, so a merged file keeps the
// provenance of every job that contributed. Each record also embeds the
// provenance records found in its input files ("parents"), so the full
// chain nano -> skim -> analysis tree -> histograms can be reconstructed.
#pragma once
#include <TDirectory.h>
#include <TFile.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace hell {

class Provenance {
 public:
  using json = nlohmann::json;
  Provenance();
  // Software and environment: version, git hash, ROOT, host, user, time.
  static json software();
  void setConfig(const json& config) { record_["config"] = config; }
  void setCommandLine(int argc, char** argv);
  void setInputFiles(const std::vector<std::string>& files) { record_["inputs"]["files"] = files; }
  void setBadInputFiles(const std::vector<std::string>& files) { record_["inputs"]["skipped"] = files; }
  void setProducts(const json& products) { record_["products"] = products; }
  void set(const std::string& key, const json& v) { record_[key] = v; }
  // Harvest the provenance records of an input file (if it is a HELL file).
  void addParent(TFile* file, const std::string& path);
  static std::vector<json> read(TFile* file);
  const json& record() const { return record_; }
  json& record() { return record_; }
  void write(TDirectory* hellDir) const;

 private:
  json record_;
  std::vector<json> parents_;
};

}  // namespace hell
