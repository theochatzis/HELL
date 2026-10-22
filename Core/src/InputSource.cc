#include "Core/interface/InputSource.h"
#include "Core/interface/Logger.h"
#include "Core/interface/Utils.h"
#include <TSystem.h>
#include <TSystemDirectory.h>
#include <TList.h>
#include <fstream>

namespace hell {

std::vector<std::string> InputSource::expandInputFiles(const std::vector<std::string>& inputs) {
  std::vector<std::string> out;
  for (const auto& in : inputs) {
    const std::string item = trim(in);
    if (item.empty()) continue;
    if (item[0] == '@') {
      std::ifstream f(item.substr(1));
      if (!f) throw std::runtime_error("cannot open file list " + item.substr(1));
      std::string line;
      while (std::getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        out.push_back(line);
      }
      continue;
    }
    if (!startsWith(item, "root://") && gSystem->AccessPathName(item.c_str()) == false) {
      FileStat_t st;
      gSystem->GetPathInfo(item.c_str(), st);
      if (R_ISDIR(st.fMode)) {
        TSystemDirectory dir(item.c_str(), item.c_str());
        std::vector<std::string> found;
        if (TList* files = dir.GetListOfFiles()) {
          for (TObject* o : *files) {
            const std::string n = o->GetName();
            if (endsWith(n, ".root")) found.push_back(item + "/" + n);
          }
          delete files;
        }
        std::sort(found.begin(), found.end());
        out.insert(out.end(), found.begin(), found.end());
        continue;
      }
    }
    out.push_back(item);
  }
  return out;
}

InputSource::InputSource(const std::vector<std::string>& files, const std::string& treeName, bool probe)
    : chain_(std::make_unique<TChain>(treeName.c_str())), treeName_(treeName) {
  for (const auto& f : files) files_.push_back(InputFileInfo{f, 0, true, ""});
  if (probe) probeFiles();
  for (auto& fi : files_) {
    if (!fi.good) continue;
    if (probe) chain_->Add(fi.path.c_str(), fi.entries);
    else chain_->Add(fi.path.c_str());
  }
  entries_ = chain_->GetEntries();
  if (files_.empty()) throw std::runtime_error("no input files given");
  if (goodFiles().empty()) throw std::runtime_error("none of the input files could be opened");
}

InputSource::~InputSource() = default;

void InputSource::probeFiles() {
  for (auto& fi : files_) {
    std::unique_ptr<TFile> f(TFile::Open(fi.path.c_str()));
    if (!f || f->IsZombie()) { fi.good = false; fi.problem = "cannot open"; }
    else if (f->TestBit(TFile::kRecovered)) { fi.good = false; fi.problem = "recovered (truncated) file"; }
    else {
      TTree* t = dynamic_cast<TTree*>(f->Get(treeName_.c_str()));
      if (!t) { fi.good = false; fi.problem = "no tree " + treeName_; }
      else fi.entries = t->GetEntries();
    }
    if (!fi.good) HELL_WARN("input") << "skipping " << fi.path << ": " << fi.problem;
  }
}

std::vector<std::string> InputSource::goodFiles() const {
  std::vector<std::string> v; for (const auto& f : files_) if (f.good) v.push_back(f.path); return v;
}
std::vector<std::string> InputSource::badFiles() const {
  std::vector<std::string> v; for (const auto& f : files_) if (!f.good) v.push_back(f.path); return v;
}

long long InputSource::load(long long entry) {
  const long long local = chain_->LoadTree(entry);
  if (local < 0) return local;
  if (chain_->GetTreeNumber() != currentTree_) {
    currentTree_ = chain_->GetTreeNumber();
    for (auto& f : treeChange_) f(chain_->GetTree(), currentTree_);
  }
  return local;
}

std::string InputSource::currentFile() const {
  TFile* f = chain_->GetCurrentFile();
  return f ? f->GetName() : "";
}

}  // namespace hell
