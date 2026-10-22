// Event weight producers. They fill a Weights product and update the
// nominal event weight used by the cutflow and histograms.
//
// Weight-type systematics (pileup, scale factors, scale/PDF) are stored as
// up/down variations in the Weights product and end up as branches
// <weights>_<name>Up/Down in the output tree, ready for the histogrammer.
//
// python:
//   process.genWeight = Module("GenWeightProducer")             # genWeight (sign only option)
//   process.puWeight  = Module("HistogramWeightProducer", name="pu", file="pu.root",
//                              hist="ratio", histUp="ratioUp", histDown="ratioDown",
//                              variable="nTrueInt", eventInfo="eventInfo")
//   process.weights   = Module("WeightCollector", sources=["genWeight","puWeight"])
#include "Core/interface/Logger.h"
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/DataFormats.h"
#include <TFile.h>
#include <TH1.h>
#include <memory>

namespace hell {

class GenWeightProducer : public Producer {
 public:
  GenWeightProducer(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx), isData_(ctx.job().isData) {
    genWeight_ = ctx.consumesValue<float>(ps.get("branch", "genWeight"), true);
    signOnly_ = ps.get<bool>("signOnly", false);
    // Optional: LHE scale and PS weights as variations (indices follow the NanoAOD convention).
    lheScale_ = ctx.consumesArray<float>("LHEScaleWeight", true);
    psWeight_ = ctx.consumesArray<float>("PSWeight", true);
    storeScale_ = ps.get<bool>("storeScaleVariations", true);
    ctx.produces<Weights>();
  }
  void produce(Event& ev) override {
    Weights w;
    double g = 1.0;
    if (!isData_ && genWeight_.valid()) g = *genWeight_;
    if (signOnly_) g = (g < 0) ? -1.0 : 1.0;
    w.set("gen", g);
    if (!isData_ && storeScale_ && lheScale_.valid() && lheScale_.size() >= 9) {
      // muR/muF: index 1 = (0.5,1)... NanoAOD: [0]=(0.5,0.5) [1]=(0.5,1) [3]=(1,0.5) [4]=(1,1) [5]=(1,2) [7]=(2,1) [8]=(2,2)
      w.set("muR", 1.0, lheScale_[7], lheScale_[1]);
      w.set("muF", 1.0, lheScale_[5], lheScale_[3]);
    }
    if (!isData_ && storeScale_ && psWeight_.valid() && psWeight_.size() >= 4) {
      w.set("isr", 1.0, psWeight_[0], psWeight_[2]);
      w.set("fsr", 1.0, psWeight_[1], psWeight_[3]);
    }
    ev.weight *= g;
    ev.put(label(), std::move(w));
  }
 private:
  bool isData_, signOnly_, storeScale_;
  ValueHandle<float> genWeight_;
  ArrayHandle<float> lheScale_, psWeight_;
};

// Weight from a histogram lookup (e.g. pileup reweighting ratio data/MC).
class HistogramWeightProducer : public Producer {
 public:
  HistogramWeightProducer(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx), isData_(ctx.job().isData) {
    name_ = ps.get<std::string>("name");
    variable_ = ps.get("variable", "nTrueInt");
    eventInfo_ = ctx.consumesProduct(ps.get("eventInfo", "eventInfo"));
    ctx.produces<Weights>();
    if (isData_) return;
    const std::string file = ps.get<std::string>("file");
    file_.reset(TFile::Open(file.c_str()));
    if (!file_ || file_->IsZombie()) throw std::runtime_error("HistogramWeightProducer: cannot open " + file);
    auto getH = [&](const std::string& n) -> TH1* {
      if (n.empty()) return nullptr;
      TH1* h = dynamic_cast<TH1*>(file_->Get(n.c_str()));
      if (!h) throw std::runtime_error("HistogramWeightProducer: no histogram " + n + " in " + file);
      h->SetDirectory(nullptr);
      return h;
    };
    h_ = getH(ps.get<std::string>("hist"));
    hUp_ = getH(ps.get("histUp", "")); hDown_ = getH(ps.get("histDown", ""));
  }
  void produce(Event& ev) override {
    Weights w;
    if (!isData_) {
      const auto& info = ev.get<EventInfo>(eventInfo_);
      double x = 0;
      if (variable_ == "nTrueInt") x = info.nTrueInt; else if (variable_ == "npvsGood") x = info.npvsGood; else if (variable_ == "rho") x = info.rho;
      else throw std::runtime_error("HistogramWeightProducer: unknown variable " + variable_);
      auto lookup = [&](TH1* h) { return h ? h->GetBinContent(std::min(std::max(h->FindFixBin(x), 1), h->GetNbinsX())) : 1.0; };
      const double nom = lookup(h_);
      if (hUp_ || hDown_) w.set(name_, nom, lookup(hUp_ ? hUp_ : h_), lookup(hDown_ ? hDown_ : h_)); else w.set(name_, nom);
      ev.weight *= nom;
    } else {
      w.set(name_, 1.0);
    }
    ev.put(label(), std::move(w));
  }
 private:
  bool isData_;
  std::string name_, variable_, eventInfo_;
  std::unique_ptr<TFile> file_;
  TH1 *h_ = nullptr, *hUp_ = nullptr, *hDown_ = nullptr;
};

// Collects the Weights products of several producers into a single product
// (written as one table "weights" with all factors and their variations).
class WeightCollector : public Producer {
 public:
  WeightCollector(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx) {
    for (const auto& s : ps.get<std::vector<std::string>>("sources")) srcs_.push_back(ctx.consumesProduct(s));
    ctx.produces<Weights>();
  }
  void produce(Event& ev) override {
    Weights all;
    for (const auto& s : srcs_) for (const auto& e : ev.get<Weights>(s).entries()) {
      if (e.hasVariation) all.set(e.name, e.nominal, e.up, e.down); else all.set(e.name, e.nominal);
    }
    ev.put(label(), std::move(all));
  }
 private:
  std::vector<std::string> srcs_;
};

}  // namespace hell

HELL_REGISTER_MODULE(hell::GenWeightProducer, "Generator weight (and muR/muF, ISR/FSR variations when present); multiplies the event weight");
HELL_REGISTER_MODULE(hell::HistogramWeightProducer, "Weight from a histogram lookup (pileup ratio etc.) with optional up/down histograms");
HELL_REGISTER_MODULE(hell::WeightCollector, "Merges several Weights products into one");
