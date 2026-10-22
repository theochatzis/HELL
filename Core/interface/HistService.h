// TFileService-like histogram booking. Every module gets its own directory
// (named after its label) in the job output file; subdirectories are allowed.
// All histograms are created with Sumw2 and owned by the file.
#pragma once
#include <TDirectory.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>
#include <TTree.h>
#include <memory>
#include <string>
#include <vector>

namespace hell {

class HistDir {
 public:
  HistDir() = default;
  explicit HistDir(TDirectory* dir) : dir_(dir) {}
  bool valid() const { return dir_ != nullptr; }
  TDirectory* dir() const { return dir_; }
  HistDir subdir(const std::string& name) const;

  template <class H, class... Args>
  H* book(const std::string& name, const std::string& title, Args&&... args) const {
    TDirectory* old = gDirectory;
    dir_->cd();
    H* h = new H(name.c_str(), title.c_str(), std::forward<Args>(args)...);
    h->Sumw2();
    h->SetDirectory(dir_);
    if (old) old->cd();
    return h;
  }
  TH1D* book1D(const std::string& name, const std::string& title, int nbins, double lo, double hi) const;
  TH1D* book1D(const std::string& name, const std::string& title, const std::vector<double>& edges) const;
  TH2D* book2D(const std::string& name, const std::string& title, int nx, double xlo, double xhi, int ny, double ylo, double yhi) const;
  TProfile* bookProfile(const std::string& name, const std::string& title, int nbins, double lo, double hi) const;
  // Generic ROOT object registration (e.g. a TTree) into this directory.
  void adopt(TObject* obj) const;

 private:
  TDirectory* dir_ = nullptr;
};

class HistService {
 public:
  explicit HistService(const std::string& fileName);
  ~HistService();
  HistDir moduleDir(const std::string& label);
  HistDir dir(const std::string& path);  // "a/b/c" is created as needed
  TFile* file() { return file_.get(); }
  const std::string& fileName() const { return fileName_; }
  void write();
  void close();

 private:
  std::string fileName_;
  std::unique_ptr<TFile> file_;
};

}  // namespace hell
