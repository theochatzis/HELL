#include "Core/interface/Bookkeeper.h"
#include "Core/interface/Logger.h"
#include <TList.h>
#include <iomanip>
#include <sstream>

namespace hell {

Bookkeeper::Bookkeeper(TDirectory* hellDir, TDirectory* treeDir, bool copyRuns, bool copyLumis)
    : hellDir_(hellDir), treeDir_(treeDir), copyRuns_(copyRuns), copyLumis_(copyLumis) {
  TDirectory* old = gDirectory;
  hellDir_->cd();
  counts_ = new TH1D("counts", "HELL bookkeeping counts;;", 16, 0, 16);
  counts_->SetDirectory(hellDir_);
  counts_->SetCanExtend(TH1::kAllAxes);
  const char* labels[] = {"events_in", "events_out", "sumw_in", "sumw_out", "sumw_pos", "sumw_neg", "n_pos", "n_neg",
                          "genEventCount", "genEventSumw", "genEventSumw2", "files"};
  int bin = 1;
  for (const char* l : labels) { counts_->GetXaxis()->SetBinLabel(bin, l); countBins_[l] = bin; ++bin; }
  if (old) old->cd();
}

void Bookkeeper::fillCount(const std::string& label, double value) {
  auto it = countBins_.find(label);
  if (it == countBins_.end()) {
    const int bin = static_cast<int>(countBins_.size()) + 1;
    if (bin > counts_->GetNbinsX()) counts_->SetBins(bin + 8, 0, bin + 8);
    counts_->GetXaxis()->SetBinLabel(bin, label.c_str());
    it = countBins_.emplace(label, bin).first;
  }
  counts_->AddBinContent(it->second, value);
}

void Bookkeeper::bookCutflow(const std::vector<std::string>& labels) {
  labels_ = labels;
  passed_.assign(labels.size(), 0);
  TDirectory* old = gDirectory;
  hellDir_->cd();
  const int n = static_cast<int>(labels.size()) + 1;
  cutflow_ = new TH1D("cutflow", "Events passing each module of the path;;events", n, 0, n);
  cutflowW_ = new TH1D("cutflow_w", "Weighted events passing each module of the path;;weighted events", n, 0, n);
  cutflow_->SetDirectory(hellDir_); cutflowW_->SetDirectory(hellDir_);
  cutflow_->GetXaxis()->SetBinLabel(1, "all"); cutflowW_->GetXaxis()->SetBinLabel(1, "all");
  for (size_t i = 0; i < labels.size(); ++i) {
    cutflow_->GetXaxis()->SetBinLabel(static_cast<int>(i) + 2, labels[i].c_str());
    cutflowW_->GetXaxis()->SetBinLabel(static_cast<int>(i) + 2, labels[i].c_str());
  }
  if (old) old->cd();
}

void Bookkeeper::countPassed(size_t i, double w) {
  if (i < passed_.size()) passed_[i]++;
  if (cutflow_) { cutflow_->AddBinContent(static_cast<int>(i) + 2, 1.0); cutflowW_->AddBinContent(static_cast<int>(i) + 2, w); }
}

void Bookkeeper::newInputFile(TFile* file, const std::string& path) {
  if (!file) return;
  ++filesSeen_;
  fillCount("files", 1.0);
  if (cutflow_) { /* "all" bin is filled per event in countEvent via counts; keep cutflow all = events_in */ }
  auto copyTree = [&](const char* name, TTree*& out) {
    TTree* in = dynamic_cast<TTree*>(file->Get(name));
    if (!in) return;
    TDirectory* old = gDirectory;
    treeDir_->cd();
    if (!out) {
      out = in->CloneTree(0);
      out->SetDirectory(treeDir_);
      out->SetName(name);
    } else {
      in->CopyAddresses(out);
    }
    if (out) {
      const long long n = in->GetEntries();
      for (long long i = 0; i < n; ++i) { in->GetEntry(i); out->Fill(); }
      in->ResetBranchAddresses();
    }
    if (old) old->cd();
  };
  if (copyRuns_) copyTree("Runs", runsOut_);
  if (copyLumis_) copyTree("LuminosityBlocks", lumisOut_);
  // Sum the generator bookkeeping of this file.
  if (TTree* runs = dynamic_cast<TTree*>(file->Get("Runs"))) {
    if (runs->GetBranch("genEventSumw")) {
      double sumw = 0, sumw2 = 0; long long count = 0;
      runs->SetBranchAddress("genEventSumw", &sumw);
      if (runs->GetBranch("genEventSumw2")) runs->SetBranchAddress("genEventSumw2", &sumw2);
      if (runs->GetBranch("genEventCount")) runs->SetBranchAddress("genEventCount", &count);
      for (long long i = 0; i < runs->GetEntries(); ++i) {
        runs->GetEntry(i);
        genEventSumw_ += sumw; genEventCount_ += count;
        fillCount("genEventSumw", sumw); fillCount("genEventSumw2", sumw2); fillCount("genEventCount", static_cast<double>(count));
      }
      runs->ResetBranchAddresses();
    }
  }
}

std::string Bookkeeper::summaryTable() const {
  std::ostringstream os;
  os << std::left << std::setw(28) << "module" << std::right << std::setw(12) << "passed" << std::setw(10) << "eff" << "\n";
  os << std::left << std::setw(28) << "all" << std::right << std::setw(12) << eventsIn_ << std::setw(10) << "" << "\n";
  long long prev = eventsIn_;
  for (size_t i = 0; i < labels_.size(); ++i) {
    os << std::left << std::setw(28) << labels_[i] << std::right << std::setw(12) << passed_[i];
    if (prev > 0) os << std::setw(9) << std::fixed << std::setprecision(1) << 100.0 * passed_[i] / prev << "%";
    os << "\n";
    prev = passed_[i];
  }
  return os.str();
}

void Bookkeeper::write() {
  if (cutflow_) { cutflow_->SetBinContent(1, static_cast<double>(eventsIn_)); cutflowW_->SetBinContent(1, sumwIn_); }
}

}  // namespace hell
