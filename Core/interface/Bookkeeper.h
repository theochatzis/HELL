// Bookkeeping of what went into a job and what came out. Everything written
// is mergeable with hadd so that split jobs can be merged without loss:
//  * hell/counts     TH1D with labelled bins (events in/out, genEventCount,
//                    genEventSumw, genEventSumw2, sum of positive/negative
//                    weights, files).
//  * hell/cutflow    TH1D, unweighted pass counts per module of the path.
//  * hell/cutflow_w  TH1D, weighted with the nominal event weight.
//  * Runs            copy of the NanoAOD Runs tree of every input file
//                    (per-run genEventSumw, LHE/PS sums) - entries concatenate.
//  * LuminosityBlocks copy of the input lumi tree (run, luminosityBlock) so
//                    that certified lumi and the Golden JSON can be applied
//                    and the luminosity recomputed later.
// Normalisation of a process later uses sum(genEventSumw) over all files.
#pragma once
#include <TDirectory.h>
#include <TFile.h>
#include <TH1D.h>
#include <TTree.h>
#include <map>
#include <string>
#include <vector>

namespace hell {

class Bookkeeper {
 public:
  Bookkeeper(TDirectory* hellDir, TDirectory* treeDir, bool copyRuns, bool copyLumis);

  void bookCutflow(const std::vector<std::string>& moduleLabels);
  void countEvent(double weight) { eventsIn_++; sumwIn_ += weight; fillCount("events_in", 1.0); }
  void countPassed(size_t moduleIndex, double weight);
  void countOutput(double weight) { eventsOut_++; fillCount("events_out", 1.0); fillCount("sumw_out", weight); }
  void countGenWeight(double w) {
    fillCount("sumw_in", w);
    if (w >= 0) fillCount("sumw_pos", w); else fillCount("sumw_neg", w);
    fillCount(w >= 0 ? "n_pos" : "n_neg", 1.0);
  }

  // Called for every input file that is opened: harvests Runs/LuminosityBlocks.
  void newInputFile(TFile* file, const std::string& path);

  long long eventsIn() const { return eventsIn_; }
  long long eventsOut() const { return eventsOut_; }
  double genEventSumw() const { return genEventSumw_; }
  long long genEventCount() const { return genEventCount_; }
  std::vector<long long> passed() const { return passed_; }
  const std::vector<std::string>& labels() const { return labels_; }
  std::string summaryTable() const;
  void write();

 private:
  void fillCount(const std::string& label, double value);
  TDirectory* hellDir_;
  TDirectory* treeDir_;
  bool copyRuns_, copyLumis_;
  TH1D* counts_ = nullptr;
  TH1D* cutflow_ = nullptr;
  TH1D* cutflowW_ = nullptr;
  TTree* runsOut_ = nullptr;
  TTree* lumisOut_ = nullptr;
  std::vector<std::string> labels_;
  std::vector<long long> passed_;
  long long eventsIn_ = 0, eventsOut_ = 0;
  double sumwIn_ = 0;
  double genEventSumw_ = 0;
  long long genEventCount_ = 0;
  int filesSeen_ = 0;
  std::map<std::string, int> countBins_;
};

}  // namespace hell
