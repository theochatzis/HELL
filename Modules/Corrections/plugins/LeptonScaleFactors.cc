// Lepton scale factors from the central MUO and EGM JSONs. Each producer takes a selected collection,
// writes a copy with `scaleFactor` set per lepton (id x iso [x trigger for the leading one]) and a Weights
// product with one entry per factor (up/down = the POG systematic). Transparent for data (weights 1).
//
// python:
//   process.muonSF     = Module("MuonScaleFactorProducer", src="goodMuons", applyTrigger=True)
//   process.electronSF = Module("ElectronScaleFactorProducer", src="goodElectrons", applyTrigger=True)
// Correction names (id, iso, trigger, electron year and working point) come from samples/corrections.yaml.
#include "Core/interface/Logger.h"
#include "Core/interface/ModuleFactory.h"
#include "Corrections/interface/CorrectionService.h"
#include "Corrections/interface/EraCorrections.h"
#include "DataFormats/interface/DataFormats.h"
#include <algorithm>
#include <cmath>

namespace hell {

class MuonScaleFactorProducer : public Producer {
 public:
  MuonScaleFactorProducer(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx), isData_(ctx.job().isData) {
    src_ = ctx.consumesProduct(ps.get<std::string>("src"));
    applyTrigger_ = ps.get<bool>("applyTrigger", true);
    minPt_ = ps.get<double>("minPt", 15.0); maxPt_ = ps.get<double>("maxPt", 199.9); maxAbsEta_ = ps.get<double>("maxAbsEta", 2.39);
    ctx.produces<MuonCollection>();
    weightsLabel_ = ctx.produces<Weights>("weights");
    if (isData_) return;
    const ParameterSet corr = EraCorrections::fromJob(ctx.job());
    const std::string file = EraCorrections::file(corr, "muon");
    auto& cs = CorrectionService::instance();
    id_ = cs.get(file, EraCorrections::name(corr, "muonId"));
    iso_ = cs.get(file, EraCorrections::name(corr, "muonIso"));
    if (applyTrigger_) trigger_ = cs.get(file, EraCorrections::name(corr, "muonTrigger"));
  }
  double eval(const CorrectionService::Ref& c, const Muon& m, const char* syst) const {
    const double eta = std::min(std::fabs(static_cast<double>(m.eta)), maxAbsEta_);
    const double pt = std::min(std::max(static_cast<double>(m.pt), minPt_), maxPt_);
    return c->evaluate({eta, pt, std::string(syst)});
  }
  void produce(Event& ev) override {
    MuonCollection muons = ev.get<MuonCollection>(src_);
    Weights w;
    if (isData_) { w.set("muId", 1.0); w.set("muIso", 1.0); if (applyTrigger_) w.set("muTrig", 1.0); }
    else {
      double id = 1, idUp = 1, idDn = 1, iso = 1, isoUp = 1, isoDn = 1, trig = 1, trigUp = 1, trigDn = 1;
      for (size_t i = 0; i < muons.size(); ++i) {
        Muon& m = muons[i];
        const double sId = eval(id_, m, "nominal"), sIso = eval(iso_, m, "nominal");
        id *= sId; idUp *= eval(id_, m, "systup"); idDn *= eval(id_, m, "systdown");
        iso *= sIso; isoUp *= eval(iso_, m, "systup"); isoDn *= eval(iso_, m, "systdown");
        m.scaleFactor = static_cast<float>(sId * sIso);
        if (applyTrigger_ && i == 0 && m.isIsolated) {  // the leading muon fires the single-muon trigger
          const double sTrig = eval(trigger_, m, "nominal");
          trig *= sTrig; trigUp *= eval(trigger_, m, "systup"); trigDn *= eval(trigger_, m, "systdown");
          m.scaleFactor *= static_cast<float>(sTrig);
        }
      }
      w.set("muId", id, idUp, idDn); w.set("muIso", iso, isoUp, isoDn);
      if (applyTrigger_) w.set("muTrig", trig, trigUp, trigDn);
      ev.weight *= w.nominal();
    }
    ev.put(label(), std::move(muons));
    ev.put(weightsLabel_, std::move(w));
  }
 private:
  bool isData_, applyTrigger_;
  std::string src_, weightsLabel_;
  double minPt_, maxPt_, maxAbsEta_;
  CorrectionService::Ref id_, iso_, trigger_;
};

class ElectronScaleFactorProducer : public Producer {
 public:
  ElectronScaleFactorProducer(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx), isData_(ctx.job().isData) {
    src_ = ctx.consumesProduct(ps.get<std::string>("src"));
    applyTrigger_ = ps.get<bool>("applyTrigger", true);
    applyReco_ = ps.get<bool>("applyReco", true);
    minPt_ = ps.get<double>("minPt", 10.01); maxPt_ = ps.get<double>("maxPt", 499.0);
    ctx.produces<ElectronCollection>();
    weightsLabel_ = ctx.produces<Weights>("weights");
    if (isData_) return;
    const ParameterSet corr = EraCorrections::fromJob(ctx.job());
    auto& cs = CorrectionService::instance();
    year_ = EraCorrections::name(corr, "electronYear");
    idWP_ = EraCorrections::name(corr, "electronId");
    idSF_ = cs.get(EraCorrections::file(corr, "electron"), "Electron-ID-SF");
    if (applyTrigger_) { hltSF_ = cs.get(EraCorrections::file(corr, "electronHlt"), "Electron-HLT-SF"); hltPath_ = EraCorrections::name(corr, "electronHltPath"); }
  }
  double evalId(const Electron& e, const std::string& wp, const char* val) const {
    const double pt = std::min(std::max(static_cast<double>(e.pt), minPt_), maxPt_);
    return idSF_->evaluate({year_, std::string(val), wp, static_cast<double>(e.etaSC), pt});
  }
  static std::string recoWP(double pt) { return pt < 20 ? "RecoBelow20" : pt < 75 ? "Reco20to75" : "RecoAbove75"; }
  void produce(Event& ev) override {
    ElectronCollection electrons = ev.get<ElectronCollection>(src_);
    Weights w;
    if (isData_) { w.set("elId", 1.0); if (applyReco_) w.set("elReco", 1.0); if (applyTrigger_) w.set("elTrig", 1.0); }
    else {
      double id = 1, idUp = 1, idDn = 1, reco = 1, recoUp = 1, recoDn = 1, trig = 1, trigUp = 1, trigDn = 1;
      for (size_t i = 0; i < electrons.size(); ++i) {
        Electron& e = electrons[i];
        const double sId = evalId(e, idWP_, "sf");
        id *= sId; idUp *= evalId(e, idWP_, "sfup"); idDn *= evalId(e, idWP_, "sfdown");
        e.scaleFactor = static_cast<float>(sId);
        if (applyReco_) {
          const std::string wp = recoWP(e.pt);
          const double sReco = evalId(e, wp, "sf");
          reco *= sReco; recoUp *= evalId(e, wp, "sfup"); recoDn *= evalId(e, wp, "sfdown");
          e.scaleFactor *= static_cast<float>(sReco);
        }
        if (applyTrigger_ && i == 0 && e.isIsolated) {
          const double pt = std::min(std::max(static_cast<double>(e.pt), minPt_), maxPt_);
          auto hlt = [&](const char* v) { return hltSF_->evaluate({year_, std::string(v), hltPath_, static_cast<double>(e.eta), pt}); };
          const double sTrig = hlt("sf");
          trig *= sTrig; trigUp *= hlt("sfup"); trigDn *= hlt("sfdown");
          e.scaleFactor *= static_cast<float>(sTrig);
        }
      }
      w.set("elId", id, idUp, idDn);
      if (applyReco_) w.set("elReco", reco, recoUp, recoDn);
      if (applyTrigger_) w.set("elTrig", trig, trigUp, trigDn);
      ev.weight *= w.nominal();
    }
    ev.put(label(), std::move(electrons));
    ev.put(weightsLabel_, std::move(w));
  }
 private:
  bool isData_, applyTrigger_, applyReco_;
  std::string src_, weightsLabel_, year_, idWP_, hltPath_;
  double minPt_, maxPt_;
  CorrectionService::Ref idSF_, hltSF_;
};

}  // namespace hell

HELL_REGISTER_MODULE(hell::MuonScaleFactorProducer, "Muon id, isolation and trigger scale factors from the central MUO JSON (per muon and as weights with variations)");
HELL_REGISTER_MODULE(hell::ElectronScaleFactorProducer, "Electron id, reco and trigger scale factors from the central EGM JSONs (per electron and as weights with variations)");
