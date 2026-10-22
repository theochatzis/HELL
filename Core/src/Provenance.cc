#include "Core/interface/Provenance.h"
#include "Core/interface/Logger.h"
#include "Core/interface/Utils.h"
#include "Core/interface/Version.h"
#include <TROOT.h>
#include <TTree.h>
#include <TObjString.h>

namespace hell {

Provenance::Provenance() {
  record_ = json::object();
  record_["software"] = software();
  record_["time"] = nowISO8601();
  record_["host"] = hostName();
  record_["user"] = userName();
  record_["cwd"] = currentDirectory();
}

Provenance::json Provenance::software() {
  json s;
  s["framework"] = "HELL";
  s["version"] = HELL_VERSION;
  s["git"] = HELL_GIT_HASH;
  s["branch"] = HELL_GIT_BRANCH;
  s["buildTime"] = HELL_BUILD_TIME;
  s["root"] = gROOT->GetVersion();
  s["cxxStandard"] = HELL_CXX_STANDARD;
  return s;
}

void Provenance::setCommandLine(int argc, char** argv) {
  std::vector<std::string> args(argv, argv + argc);
  record_["command"] = args;
}

std::vector<Provenance::json> Provenance::read(TFile* file) {
  std::vector<json> out;
  if (!file) return out;
  TTree* t = dynamic_cast<TTree*>(file->Get("hell/provenance"));
  if (!t) return out;
  std::string* s = nullptr;
  t->SetBranchAddress("json", &s);
  for (long long i = 0; i < t->GetEntries(); ++i) {
    t->GetEntry(i);
    try { out.push_back(json::parse(*s)); } catch (...) { HELL_WARN("provenance") << "unreadable provenance record in " << file->GetName(); }
  }
  t->ResetBranchAddresses();
  return out;
}

void Provenance::addParent(TFile* file, const std::string& path) {
  for (auto& p : read(file)) {
    p["_file"] = path;
    parents_.push_back(p);
  }
}

void Provenance::write(TDirectory* hellDir) const {
  TDirectory* old = gDirectory;
  hellDir->cd();
  json rec = record_;
  rec["parents"] = parents_;
  auto* t = new TTree("provenance", "HELL provenance: one JSON record per job (hadd concatenates)");
  t->SetDirectory(hellDir);
  std::string s = rec.dump();
  t->Branch("json", &s);
  t->Fill();
  t->Write("", TObject::kOverwrite);
  // Also a human-readable summary string, handy for `hell navigate`.
  TObjString tag(("HELL " + std::string(HELL_VERSION) + " " + std::string(HELL_GIT_HASH)).c_str());
  tag.Write("hell_tag", TObject::kOverwrite);
  if (old) old->cd();
}

}  // namespace hell
