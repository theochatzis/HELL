// Producers that turn NanoAOD branches into HELL objects. They are the only
// modules that touch raw branch names: everything downstream works on the
// data model. Branch prefixes are configurable so that private NanoAOD-like
// ntuples (Pico) with different names can be read with the same modules.
//
// python:
//   process.eventInfo = Module("NanoEventInfoReader")
//   process.jets      = Module("NanoJetReader", prefix="Jet", btagger="btagPNetB")
//   process.muons     = Module("NanoMuonReader")
//   process.electrons = Module("NanoElectronReader")
//   process.met       = Module("NanoMETReader", prefix="PuppiMET")
//   process.genParts  = Module("NanoGenParticleReader")
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/DataFormats.h"

namespace hell {

class NanoEventInfoReader : public Producer {
 public:
  NanoEventInfoReader(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx), isData_(ctx.job().isData) {
    ctx.produces<EventInfo>();
    run_ = ctx.consumesValue<unsigned int>("run");
    lumi_ = ctx.consumesValue<unsigned int>("luminosityBlock");
    event_ = ctx.consumesValue<unsigned long long>("event");
    genWeight_ = ctx.consumesValue<float>(ps.get("genWeight", "genWeight"), true);
    rho_ = ctx.consumesValue<float>(ps.get("rho", "Rho_fixedGridRhoFastjetAll"), true);
    npvs_ = ctx.consumesValue<int>(ps.get("npvs", "PV_npvs"), true);
    npvsGood_ = ctx.consumesValue<int>(ps.get("npvsGood", "PV_npvsGood"), true);
    nTrueInt_ = ctx.consumesValue<float>(ps.get("nTrueInt", "Pileup_nTrueInt"), true);
    lheHT_ = ctx.consumesValue<float>(ps.get("lheHT", "LHE_HT"), true);
    lheNjets_ = ctx.consumesValue<int>(ps.get("lheNjets", "LHE_Njets"), true);
  }
  void produce(Event& ev) override {
    EventInfo e;
    e.run = *run_; e.lumi = *lumi_; e.event = *event_; e.isData = isData_;
    e.genWeight = isData_ ? 1.f : genWeight_.getOr(1.f);
    e.rho = rho_.getOr(0.f); e.npvs = npvs_.getOr(0); e.npvsGood = npvsGood_.getOr(0);
    e.nTrueInt = isData_ ? 0.f : nTrueInt_.getOr(0.f);
    e.lheHT = isData_ ? 0.f : lheHT_.getOr(0.f); e.lheNjets = isData_ ? 0 : lheNjets_.getOr(0);
    ev.put(label(), std::move(e));
  }
 private:
  bool isData_;
  ValueHandle<unsigned int> run_, lumi_;
  ValueHandle<unsigned long long> event_;
  ValueHandle<float> genWeight_, rho_, nTrueInt_, lheHT_;
  ValueHandle<int> npvs_, npvsGood_, lheNjets_;
};

class NanoJetReader : public Producer {
 public:
  NanoJetReader(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx) {
    const std::string p = ps.get("prefix", "Jet");
    btagger_ = ps.get("btagger", "btagPNetB");
    ctx.produces<JetCollection>();
    pt_ = ctx.consumesArray<float>(p + "_pt"); eta_ = ctx.consumesArray<float>(p + "_eta");
    phi_ = ctx.consumesArray<float>(p + "_phi"); mass_ = ctx.consumesArray<float>(p + "_mass");
    jetId_ = ctx.consumesArray<int>(p + "_jetId", true);
    rawFactor_ = ctx.consumesArray<float>(p + "_rawFactor", true);
    area_ = ctx.consumesArray<float>(p + "_area", true);
    hadFlav_ = ctx.consumesArray<int>(p + "_hadronFlavour", true);
    partFlav_ = ctx.consumesArray<int>(p + "_partonFlavour", true);
    genJetIdx_ = ctx.consumesArray<int>(p + "_genJetIdx", true);
    deepFlav_ = ctx.consumesArray<float>(p + "_btagDeepFlavB", true);
    pnet_ = ctx.consumesArray<float>(p + "_btagPNetB", true);
    score_ = ctx.consumesArray<float>(p + "_" + btagger_, true);
  }
  void produce(Event& ev) override {
    JetCollection jets;
    const size_t n = pt_.size();
    jets.reserve(n);
    for (size_t i = 0; i < n; ++i) {
      Jet j;
      j.pt = pt_[i]; j.eta = eta_[i]; j.phi = phi_[i]; j.mass = mass_[i]; j.index = static_cast<int>(i);
      j.ptNominal = j.pt;
      if (jetId_.valid()) j.jetId = jetId_[i];
      if (rawFactor_.valid()) j.rawFactor = rawFactor_[i];
      if (area_.valid()) j.area = area_[i];
      if (hadFlav_.valid()) j.hadronFlavour = hadFlav_[i];
      if (partFlav_.valid()) j.partonFlavour = partFlav_[i];
      if (genJetIdx_.valid()) j.genJetIdx = genJetIdx_[i];
      if (deepFlav_.valid()) j.btagDeepFlavB = deepFlav_[i];
      if (pnet_.valid()) j.btagPNetB = pnet_[i];
      j.btagScore = score_.valid() ? score_[i] : -1.f;
      jets.push_back(j);
    }
    ev.put(label(), std::move(jets));
  }
 private:
  std::string btagger_;
  ArrayHandle<float> pt_, eta_, phi_, mass_, rawFactor_, area_, deepFlav_, pnet_, score_;
  ArrayHandle<int> jetId_, hadFlav_, partFlav_, genJetIdx_;
};

class NanoMuonReader : public Producer {
 public:
  NanoMuonReader(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx) {
    const std::string p = ps.get("prefix", "Muon");
    ctx.produces<MuonCollection>();
    pt_ = ctx.consumesArray<float>(p + "_pt"); eta_ = ctx.consumesArray<float>(p + "_eta");
    phi_ = ctx.consumesArray<float>(p + "_phi"); mass_ = ctx.consumesArray<float>(p + "_mass");
    charge_ = ctx.consumesArray<int>(p + "_charge");
    iso_ = ctx.consumesArray<float>(p + "_" + ps.get("isolation", "pfRelIso04_all"), true);
    miniIso_ = ctx.consumesArray<float>(p + "_miniPFRelIso_all", true);
    dxy_ = ctx.consumesArray<float>(p + "_dxy", true); dz_ = ctx.consumesArray<float>(p + "_dz", true);
    sip3d_ = ctx.consumesArray<float>(p + "_sip3d", true);
    loose_ = ctx.consumesArray<bool>(p + "_looseId", true); medium_ = ctx.consumesArray<bool>(p + "_mediumId", true);
    tight_ = ctx.consumesArray<bool>(p + "_tightId", true); mediumPrompt_ = ctx.consumesArray<bool>(p + "_mediumPromptId", true);
    pfIsoId_ = ctx.consumesArray<int>(p + "_pfIsoId", true); highPtId_ = ctx.consumesArray<int>(p + "_highPtId", true);
    isGlobal_ = ctx.consumesArray<bool>(p + "_isGlobal", true); isTracker_ = ctx.consumesArray<bool>(p + "_isTracker", true);
    isPF_ = ctx.consumesArray<bool>(p + "_isPFcand", true); nStations_ = ctx.consumesArray<int>(p + "_nStations", true);
    tunepRelPt_ = ctx.consumesArray<float>(p + "_tunepRelPt", true); ptErr_ = ctx.consumesArray<float>(p + "_ptErr", true);
    genIdx_ = ctx.consumesArray<int>(p + "_genPartIdx", true);
  }
  void produce(Event& ev) override {
    MuonCollection out;
    const size_t n = pt_.size();
    out.reserve(n);
    for (size_t i = 0; i < n; ++i) {
      Muon m;
      m.pt = pt_[i]; m.eta = eta_[i]; m.phi = phi_[i]; m.mass = mass_[i]; m.charge = charge_[i];
      m.pdgId = -13 * m.charge; m.index = static_cast<int>(i);
      if (iso_.valid()) m.relIso = iso_[i];
      if (miniIso_.valid()) m.miniIso = miniIso_[i];
      if (dxy_.valid()) m.dxy = dxy_[i];
      if (dz_.valid()) m.dz = dz_[i];
      if (sip3d_.valid()) m.sip3d = sip3d_[i];
      if (loose_.valid()) m.looseId = loose_[i];
      if (medium_.valid()) m.mediumId = medium_[i];
      if (tight_.valid()) m.tightId = tight_[i];
      if (mediumPrompt_.valid()) m.mediumPromptId = mediumPrompt_[i];
      if (pfIsoId_.valid()) m.pfIsoId = pfIsoId_[i];
      if (highPtId_.valid()) m.highPtId = highPtId_[i];
      if (isGlobal_.valid()) m.isGlobal = isGlobal_[i];
      if (isTracker_.valid()) m.isTracker = isTracker_[i];
      if (isPF_.valid()) m.isPFcand = isPF_[i];
      if (nStations_.valid()) m.nStations = nStations_[i];
      if (tunepRelPt_.valid()) m.tunepRelPt = tunepRelPt_[i];
      if (ptErr_.valid()) m.ptErr = ptErr_[i];
      if (genIdx_.valid() && !ev.isData) m.genPartIdx = genIdx_[i];
      out.push_back(m);
    }
    ev.put(label(), std::move(out));
  }
 private:
  ArrayHandle<float> pt_, eta_, phi_, mass_, iso_, miniIso_, dxy_, dz_, sip3d_, tunepRelPt_, ptErr_;
  ArrayHandle<int> charge_, pfIsoId_, highPtId_, nStations_, genIdx_;
  ArrayHandle<bool> loose_, medium_, tight_, mediumPrompt_, isGlobal_, isTracker_, isPF_;
};

class NanoElectronReader : public Producer {
 public:
  NanoElectronReader(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx) {
    const std::string p = ps.get("prefix", "Electron");
    ctx.produces<ElectronCollection>();
    pt_ = ctx.consumesArray<float>(p + "_pt"); eta_ = ctx.consumesArray<float>(p + "_eta");
    phi_ = ctx.consumesArray<float>(p + "_phi"); mass_ = ctx.consumesArray<float>(p + "_mass");
    charge_ = ctx.consumesArray<int>(p + "_charge");
    iso_ = ctx.consumesArray<float>(p + "_" + ps.get("isolation", "pfRelIso03_all"), true);
    miniIso_ = ctx.consumesArray<float>(p + "_miniPFRelIso_all", true);
    dxy_ = ctx.consumesArray<float>(p + "_dxy", true); dz_ = ctx.consumesArray<float>(p + "_dz", true);
    sip3d_ = ctx.consumesArray<float>(p + "_sip3d", true);
    cutBased_ = ctx.consumesArray<int>(p + "_cutBased", true);
    deltaEtaSC_ = ctx.consumesArray<float>(p + "_deltaEtaSC", true);
    mvaIso90_ = ctx.consumesArray<bool>(p + "_mvaIso_WP90", true); mvaIso80_ = ctx.consumesArray<bool>(p + "_mvaIso_WP80", true);
    mvaNoIso90_ = ctx.consumesArray<bool>(p + "_mvaNoIso_WP90", true); mvaNoIso80_ = ctx.consumesArray<bool>(p + "_mvaNoIso_WP80", true);
    convVeto_ = ctx.consumesArray<bool>(p + "_convVeto", true); lostHits_ = ctx.consumesArray<int>(p + "_lostHits", true);
    r9_ = ctx.consumesArray<float>(p + "_r9", true); sieie_ = ctx.consumesArray<float>(p + "_sieie", true);
    hoe_ = ctx.consumesArray<float>(p + "_hoe", true); eInvMinusPInv_ = ctx.consumesArray<float>(p + "_eInvMinusPInv", true);
    scEtOverPt_ = ctx.consumesArray<float>(p + "_scEtOverPt", true);
    genIdx_ = ctx.consumesArray<int>(p + "_genPartIdx", true);
  }
  void produce(Event& ev) override {
    ElectronCollection out;
    const size_t n = pt_.size();
    out.reserve(n);
    for (size_t i = 0; i < n; ++i) {
      Electron e;
      e.pt = pt_[i]; e.eta = eta_[i]; e.phi = phi_[i]; e.mass = mass_[i]; e.charge = charge_[i];
      e.pdgId = -11 * e.charge; e.index = static_cast<int>(i);
      if (iso_.valid()) e.relIso = iso_[i];
      if (miniIso_.valid()) e.miniIso = miniIso_[i];
      if (dxy_.valid()) e.dxy = dxy_[i];
      if (dz_.valid()) e.dz = dz_[i];
      if (sip3d_.valid()) e.sip3d = sip3d_[i];
      if (cutBased_.valid()) { e.cutBased = cutBased_[i]; e.looseId = e.cutBased >= 2; e.mediumId = e.cutBased >= 3; e.tightId = e.cutBased >= 4; }
      if (mvaIso90_.valid()) e.mvaIsoWP90 = mvaIso90_[i];
      if (mvaIso80_.valid()) e.mvaIsoWP80 = mvaIso80_[i];
      if (mvaNoIso90_.valid()) e.mvaNoIsoWP90 = mvaNoIso90_[i];
      if (mvaNoIso80_.valid()) e.mvaNoIsoWP80 = mvaNoIso80_[i];
      e.etaSC = e.eta + (deltaEtaSC_.valid() ? deltaEtaSC_[i] : 0.f);
      if (convVeto_.valid()) e.convVeto = convVeto_[i];
      if (lostHits_.valid()) e.lostHits = lostHits_[i];
      if (r9_.valid()) e.r9 = r9_[i];
      if (sieie_.valid()) e.sieie = sieie_[i];
      if (hoe_.valid()) e.hoe = hoe_[i];
      if (eInvMinusPInv_.valid()) e.eInvMinusPInv = eInvMinusPInv_[i];
      if (scEtOverPt_.valid()) e.scEtOverPt = scEtOverPt_[i];
      if (genIdx_.valid() && !ev.isData) e.genPartIdx = genIdx_[i];
      out.push_back(e);
    }
    ev.put(label(), std::move(out));
  }
 private:
  ArrayHandle<float> pt_, eta_, phi_, mass_, iso_, miniIso_, dxy_, dz_, sip3d_, deltaEtaSC_, r9_, sieie_, hoe_, eInvMinusPInv_, scEtOverPt_;
  ArrayHandle<int> charge_, cutBased_, lostHits_, genIdx_;
  ArrayHandle<bool> mvaIso90_, mvaIso80_, mvaNoIso90_, mvaNoIso80_, convVeto_;
};

class NanoMETReader : public Producer {
 public:
  NanoMETReader(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx) {
    const std::string p = ps.get("prefix", "PuppiMET");
    ctx.produces<MET>();
    pt_ = ctx.consumesValue<float>(p + "_pt"); phi_ = ctx.consumesValue<float>(p + "_phi");
    sumEt_ = ctx.consumesValue<float>(p + "_sumEt", true);
  }
  void produce(Event& ev) override {
    MET m; m.pt = *pt_; m.phi = *phi_; m.sumEt = sumEt_.getOr(0.f); m.ptNominal = m.pt; m.phiNominal = m.phi;
    ev.put(label(), std::move(m));
  }
 private:
  ValueHandle<float> pt_, phi_, sumEt_;
};

class NanoGenParticleReader : public Producer {
 public:
  NanoGenParticleReader(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx), isData_(ctx.job().isData) {
    const std::string p = ps.get("prefix", "GenPart");
    ctx.produces<GenParticleCollection>();
    if (isData_) return;  // data has no generator information; produce an empty collection
    pt_ = ctx.consumesArray<float>(p + "_pt"); eta_ = ctx.consumesArray<float>(p + "_eta");
    phi_ = ctx.consumesArray<float>(p + "_phi"); mass_ = ctx.consumesArray<float>(p + "_mass");
    pdgId_ = ctx.consumesArray<int>(p + "_pdgId"); status_ = ctx.consumesArray<int>(p + "_status");
    flags_ = ctx.consumesArray<int>(p + "_statusFlags"); mother_ = ctx.consumesArray<int>(p + "_genPartIdxMother");
  }
  void produce(Event& ev) override {
    GenParticleCollection out;
    if (!isData_) {
      const size_t n = pt_.size();
      out.reserve(n);
      for (size_t i = 0; i < n; ++i) {
        GenParticle g;
        g.pt = pt_[i]; g.eta = eta_[i]; g.phi = phi_[i]; g.mass = mass_[i]; g.pdgId = pdgId_[i]; g.index = static_cast<int>(i);
        g.charge = (g.pdgId > 0) ? -1 : 1;  // sign convention for charged leptons/quarks, informative only
        g.status = status_[i]; g.statusFlags = flags_[i]; g.motherIndex = mother_[i];
        out.push_back(g);
      }
    }
    ev.put(label(), std::move(out));
  }
 private:
  bool isData_;
  ArrayHandle<float> pt_, eta_, phi_, mass_;
  ArrayHandle<int> pdgId_, status_, flags_, mother_;
};

}  // namespace hell

HELL_REGISTER_MODULE(hell::NanoEventInfoReader, "Reads run/lumi/event, genWeight, rho, PV and pileup info into an EventInfo product");
HELL_REGISTER_MODULE(hell::NanoJetReader, "Reads the NanoAOD Jet collection (prefix configurable) into a JetCollection");
HELL_REGISTER_MODULE(hell::NanoMuonReader, "Reads NanoAOD muons into a MuonCollection");
HELL_REGISTER_MODULE(hell::NanoElectronReader, "Reads NanoAOD electrons into an ElectronCollection");
HELL_REGISTER_MODULE(hell::NanoMETReader, "Reads a MET collection (PuppiMET by default) into a MET product");
HELL_REGISTER_MODULE(hell::NanoGenParticleReader, "Reads GenPart into a GenParticleCollection (empty for data)");
