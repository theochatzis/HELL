#include "Hist/interface/Histogrammer.h"
#include "Core/interface/Logger.h"
#include "Core/interface/Provenance.h"
#include "Core/interface/Utils.h"
#include <TFile.h>
#include <TTree.h>
#include <TTreeFormula.h>
#include <TLeaf.h>
#include <TObjArray.h>
#include <TStopwatch.h>
#include <TObjString.h>

namespace hell {

Histogrammer::Histogrammer(const nlohmann::json& cfg) : config_(cfg) { parse(cfg); }

void Histogrammer::parse(const nlohmann::json& cfg) {
  output_ = cfg.value("output", "hists.root");
  treeName_ = cfg.value("treeName", "Events");
  lumi_ = cfg.value("lumi", 1.0);
  maxEvents_ = cfg.value("maxEvents", -1LL);
  for (auto it = cfg.at("variables").begin(); it != cfg.at("variables").end(); ++it) {
    HistVariable v; v.name = it.key(); v.expr = it.value().at("expr"); v.title = it.value().value("title", it.key());
    if (it.value().contains("edges")) v.edges = it.value()["edges"].get<std::vector<double>>();
    else {
      const auto b = it.value().at("bins");
      const int n = b.at(0); const double lo = b.at(1), hi = b.at(2);
      for (int i = 0; i <= n; ++i) v.edges.push_back(lo + (hi - lo) * i / n);
    }
    variables_.push_back(v);
  }
  for (auto it = cfg.at("regions").begin(); it != cfg.at("regions").end(); ++it) {
    HistRegion r; r.name = it.key(); r.selection = it.value().value("selection", "1"); r.weight = it.value().value("weight", "1");
    regions_.push_back(r);
  }
  if (cfg.contains("systematics"))
    for (auto it = cfg["systematics"].begin(); it != cfg["systematics"].end(); ++it) {
      HistSystematic s; s.name = it.key(); s.type = it.value().value("type", "weight"); s.up = it.value().at("up"); s.down = it.value().at("down");
      if (s.type != "weight" && s.type != "tree") throw std::runtime_error("systematic " + s.name + ": type must be weight or tree");
      systematics_.push_back(s);
    }
  for (const auto& p : cfg.at("processes")) {
    HistProcess pr; pr.name = p.at("name"); pr.label = p.value("label", pr.name); pr.color = p.value("color", "#000000");
    pr.isData = p.value("isData", false); pr.cut = p.value("cut", "");
    for (const auto& s : p.at("samples")) {
      HistSample sm; sm.name = s.at("name"); sm.files = s.at("files").get<std::vector<std::string>>();
      sm.xsec = s.value("xsec", 0.0); sm.sumw = s.value("sumw", 0.0);
      if (s.contains("variations")) for (auto it = s["variations"].begin(); it != s["variations"].end(); ++it) sm.variationFiles[it.key()] = it.value().get<std::vector<std::string>>();
      pr.samples.push_back(sm);
    }
    processes_.push_back(pr);
  }
  if (variables_.empty() || regions_.empty() || processes_.empty()) throw std::runtime_error("histogrammer needs variables, regions and processes");
}

double Histogrammer::readSumw(const std::vector<std::string>& files, const std::string& label) {
  double sum = 0;
  for (const auto& f : files) {
    std::unique_ptr<TFile> file(TFile::Open(f.c_str()));
    if (!file || file->IsZombie()) { HELL_WARN("hist") << "cannot open " << f << " for bookkeeping"; continue; }
    TH1* counts = dynamic_cast<TH1*>(file->Get("hell/counts"));
    if (!counts) continue;
    const int bin = counts->GetXaxis()->FindFixBin(label.c_str());
    if (bin > 0) sum += counts->GetBinContent(bin);
  }
  return sum;
}

TH1D* Histogrammer::hist(const std::string& region, const std::string& var, const std::string& process, const std::string& syst) {
  const std::string key = region + "/" + var + "/" + process + (syst.empty() ? "" : "_" + syst);
  auto it = hists_.find(key);
  if (it != hists_.end()) return it->second.get();
  const HistVariable* v = nullptr;
  for (const auto& x : variables_) if (x.name == var) v = &x;
  auto h = std::make_unique<TH1D>(key.c_str(), (";" + v->title + ";events").c_str(), static_cast<int>(v->edges.size()) - 1, v->edges.data());
  h->SetDirectory(nullptr);
  h->Sumw2();
  TH1D* raw = h.get();
  hists_[key] = std::move(h);
  return raw;
}

void Histogrammer::processFiles(const HistProcess& p, const HistSample& s, const std::vector<std::string>& files, const std::string& systName,
                                double scale, bool weightSystematics) {
  long long seen = 0;
  for (const auto& fname : files) {
    std::unique_ptr<TFile> file(TFile::Open(fname.c_str()));
    if (!file || file->IsZombie()) { HELL_WARN("hist") << "cannot open " << fname; continue; }
    TTree* tree = dynamic_cast<TTree*>(file->Get(treeName_.c_str()));
    if (!tree) { HELL_WARN("hist") << "no tree " << treeName_ << " in " << fname; continue; }
    // Formulas
    std::vector<std::unique_ptr<TTreeFormula>> regionSel, regionW, varF, systUp, systDown;
    std::unique_ptr<TTreeFormula> procCut;
    auto make = [&](const std::string& name, const std::string& expr) {
      auto f = std::make_unique<TTreeFormula>(name.c_str(), expr.c_str(), tree);
      if (f->GetNdim() == 0) throw std::runtime_error("invalid expression '" + expr + "' for " + name + " in " + fname);
      return f;
    };
    for (const auto& r : regions_) { regionSel.push_back(make("sel_" + r.name, r.selection)); regionW.push_back(make("w_" + r.name, p.isData ? "1" : r.weight)); }
    for (const auto& v : variables_) varF.push_back(make("var_" + v.name, v.expr));
    if (!p.cut.empty()) procCut = make("cut_" + p.name, p.cut);
    if (weightSystematics && !p.isData)
      for (const auto& sy : systematics_) {
        if (sy.type != "weight") { systUp.push_back(nullptr); systDown.push_back(nullptr); continue; }
        systUp.push_back(make("up_" + sy.name, sy.up)); systDown.push_back(make("down_" + sy.name, sy.down));
      }
    // Enable only the branches used.
    tree->SetBranchStatus("*", 0);
    auto enable = [&](TTreeFormula* f) { if (!f) return; for (int i = 0; i < f->GetNcodes(); ++i) if (TLeaf* l = f->GetLeaf(i)) tree->SetBranchStatus(l->GetBranch()->GetName(), 1); };
    for (auto& f : regionSel) enable(f.get());
    for (auto& f : regionW) enable(f.get());
    for (auto& f : varF) enable(f.get());
    enable(procCut.get());
    for (auto& f : systUp) enable(f.get());
    for (auto& f : systDown) enable(f.get());

    const long long n = tree->GetEntries();
    for (long long i = 0; i < n; ++i) {
      if (maxEvents_ >= 0 && seen >= maxEvents_) break;
      tree->GetEntry(i);
      ++seen;
      if (procCut && procCut->EvalInstance() == 0) continue;
      for (size_t r = 0; r < regions_.size(); ++r) {
        if (regionSel[r]->EvalInstance() == 0) continue;
        const double w = regionW[r]->EvalInstance() * scale;
        for (size_t v = 0; v < variables_.size(); ++v) {
          const double x = varF[v]->EvalInstance();
          hist(regions_[r].name, variables_[v].name, p.name, systName)->Fill(x, w);
          for (size_t sy = 0; sy < systUp.size(); ++sy) {
            if (!systUp[sy]) continue;
            hist(regions_[r].name, variables_[v].name, p.name, systematics_[sy].name + "Up")->Fill(x, w * systUp[sy]->EvalInstance());
            hist(regions_[r].name, variables_[v].name, p.name, systematics_[sy].name + "Down")->Fill(x, w * systDown[sy]->EvalInstance());
          }
        }
      }
    }
    tree->SetBranchStatus("*", 1);
  }
}

int Histogrammer::run() {
  TStopwatch sw; sw.Start();
  HELL_INFO("hist") << "Welcome to HELL. Histogramming " << processes_.size() << " processes x " << regions_.size() << " regions x " << variables_.size() << " variables, lumi " << lumi_ << " pb^-1";
  nlohmann::json normalisation = nlohmann::json::object();
  for (const auto& p : processes_) {
    for (const auto& s : p.samples) {
      double scale = 1.0;
      if (!p.isData) {
        const double sumw = s.sumw > 0 ? s.sumw : readSumw(s.files);
        if (sumw <= 0) throw std::runtime_error("sample " + s.name + ": sum of weights is zero (no hell/counts in the files? give 'sumw' explicitly)");
        if (s.xsec <= 0) throw std::runtime_error("sample " + s.name + ": cross section missing");
        scale = lumi_ * s.xsec / sumw;
        normalisation[s.name] = {{"xsec", s.xsec}, {"sumw", sumw}, {"lumi", lumi_}, {"scale", scale}};
      }
      HELL_INFO("hist") << p.name << " / " << s.name << ": " << s.files.size() << " files, scale " << scale;
      processFiles(p, s, s.files, "", scale, true);
      if (p.isData) continue;
      for (const auto& sy : systematics_) {
        if (sy.type != "tree") continue;
        for (const auto& dir : {std::make_pair(sy.up, sy.name + "Up"), std::make_pair(sy.down, sy.name + "Down")}) {
          auto it = s.variationFiles.find(dir.first);
          if (it == s.variationFiles.end()) { HELL_WARN("hist") << "no files for variation " << dir.first << " of sample " << s.name << "; using nominal"; processFiles(p, s, s.files, dir.second, scale, false); continue; }
          const double sumwV = readSumw(it->second);
          processFiles(p, s, it->second, dir.second, sumwV > 0 ? lumi_ * s.xsec / sumwV : scale, false);
        }
      }
    }
  }
  // Ensure every (region, variable, process, syst) exists even if empty, for the datacards.
  for (const auto& r : regions_) for (const auto& v : variables_) for (const auto& p : processes_) {
    hist(r.name, v.name, p.name, "");
    if (!p.isData) for (const auto& sy : systematics_) { hist(r.name, v.name, p.name, sy.name + "Up"); hist(r.name, v.name, p.name, sy.name + "Down"); }
  }
  // Negative yields (few events with large negative generator weights) break fits: say so loudly.
  for (const auto& kv : hists_)
    if (kv.second->Integral() < 0) HELL_WARN("hist") << kv.first << " has a negative integral (" << kv.second->Integral() << "): too few events or missing positive reweighting";
  std::unique_ptr<TFile> out(TFile::Open(output_.c_str(), "RECREATE"));
  if (!out || out->IsZombie()) throw std::runtime_error("cannot create " + output_);
  for (auto& kv : hists_) {
    const auto parts = splitString(kv.first, '/');
    TDirectory* d = out.get();
    for (size_t i = 0; i + 1 < parts.size(); ++i) { TDirectory* n = d->GetDirectory(parts[i].c_str()); d = n ? n : d->mkdir(parts[i].c_str()); }
    kv.second->SetName(parts.back().c_str());
    d->WriteTObject(kv.second.get());
  }
  // Provenance and the configuration used (processes/colours/lumi are needed by plotting and datacards).
  TDirectory* hellDir = out->mkdir("hell");
  Provenance prov;
  prov.setConfig(config_);
  prov.set("normalisation", normalisation);
  prov.set("job", {{"process", "HIST"}, {"output", output_}});
  prov.write(hellDir);
  hellDir->cd();
  TObjString cfgStr(config_.dump().c_str()); cfgStr.Write("hist_config");
  out->Close();
  sw.Stop();
  HELL_INFO("hist") << "wrote " << hists_.size() << " histograms to " << output_ << " in " << formatDuration(sw.RealTime());
  return 0;
}

}  // namespace hell
