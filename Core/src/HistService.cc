#include "Core/interface/HistService.h"
#include "Core/interface/Logger.h"
#include "Core/interface/Utils.h"
#include <stdexcept>

namespace hell {

HistDir HistDir::subdir(const std::string& name) const {
  if (!dir_) throw std::runtime_error("HistDir::subdir on an invalid directory");
  TDirectory* d = dir_->GetDirectory(name.c_str());
  if (!d) d = dir_->mkdir(name.c_str(), "", true);
  return HistDir(d);
}
TH1D* HistDir::book1D(const std::string& n, const std::string& t, int nb, double lo, double hi) const { return book<TH1D>(n, t, nb, lo, hi); }
TH1D* HistDir::book1D(const std::string& n, const std::string& t, const std::vector<double>& e) const { return book<TH1D>(n, t, static_cast<int>(e.size()) - 1, e.data()); }
TH2D* HistDir::book2D(const std::string& n, const std::string& t, int nx, double xlo, double xhi, int ny, double ylo, double yhi) const { return book<TH2D>(n, t, nx, xlo, xhi, ny, ylo, yhi); }
TProfile* HistDir::bookProfile(const std::string& n, const std::string& t, int nb, double lo, double hi) const {
  TDirectory* old = gDirectory; dir_->cd();
  auto* p = new TProfile(n.c_str(), t.c_str(), nb, lo, hi);
  p->SetDirectory(dir_);
  if (old) old->cd();
  return p;
}
void HistDir::adopt(TObject* obj) const {
  if (auto* t = dynamic_cast<TTree*>(obj)) t->SetDirectory(dir_);
  else if (auto* h = dynamic_cast<TH1*>(obj)) h->SetDirectory(dir_);
  else dir_->Append(obj);
}

HistService::HistService(const std::string& fileName) : fileName_(fileName) {
  file_.reset(TFile::Open(fileName.c_str(), "RECREATE"));
  if (!file_ || file_->IsZombie()) throw std::runtime_error("cannot create output file " + fileName);
}
HistService::~HistService() { close(); }
HistDir HistService::moduleDir(const std::string& label) { return dir(label); }
HistDir HistService::dir(const std::string& path) {
  TDirectory* d = file_.get();
  for (const auto& part : splitString(path, '/')) {
    TDirectory* next = d->GetDirectory(part.c_str());
    if (!next) next = d->mkdir(part.c_str(), "", true);
    d = next;
  }
  return HistDir(d);
}
void HistService::write() { if (file_) { file_->Write("", TObject::kOverwrite); } }
void HistService::close() {
  if (file_) { file_->Write("", TObject::kOverwrite); file_->Close(); file_.reset(); }
}

}  // namespace hell
