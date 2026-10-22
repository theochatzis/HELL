// Input files and the chain of their event trees. Also gives access to the
// per-file auxiliary trees (Runs, LuminosityBlocks) used for bookkeeping.
#pragma once
#include <TChain.h>
#include <TFile.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace hell {

struct InputFileInfo {
  std::string path;
  long long entries = 0;
  bool good = true;
  std::string problem;
};

class InputSource {
 public:
  // Accepts plain paths, "@list.txt" (one path per line, '#' comments) and directories (all *.root).
  static std::vector<std::string> expandInputFiles(const std::vector<std::string>& inputs);

  InputSource(const std::vector<std::string>& files, const std::string& treeName, bool probe = true);
  ~InputSource();

  TChain& chain() { return *chain_; }
  const std::vector<InputFileInfo>& files() const { return files_; }
  std::vector<std::string> goodFiles() const;
  std::vector<std::string> badFiles() const;
  long long entries() const { return entries_; }
  const std::string& treeName() const { return treeName_; }

  // Load an entry; returns the local entry number (or < 0 on failure).
  // Calls the tree-change callback when the chain moves to a new file.
  long long load(long long entry);
  int currentTreeNumber() const { return currentTree_; }
  std::string currentFile() const;
  TFile* currentFileHandle() const { return chain_->GetCurrentFile(); }

  void onTreeChange(std::function<void(TTree*, int)> f) { treeChange_.push_back(std::move(f)); }

 private:
  void probeFiles();
  std::unique_ptr<TChain> chain_;
  std::string treeName_;
  std::vector<InputFileInfo> files_;
  long long entries_ = 0;
  int currentTree_ = -1;
  std::vector<std::function<void(TTree*, int)>> treeChange_;
};

}  // namespace hell
