#include "Fit/interface/Datacard.h"
#include "Core/interface/Logger.h"
#include "Core/interface/Utils.h"
#include <TFile.h>
#include <TH1D.h>
#include <TKey.h>
#include <RooAddPdf.h>
#include <RooArgList.h>
#include <RooDataHist.h>
#include <RooFormulaVar.h>
#include <RooGaussian.h>
#include <RooCBShape.h>
#include <RooFitResult.h>
#include <RooHistPdf.h>
#include <RooRealVar.h>
#include <RooWorkspace.h>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <sys/stat.h>

namespace hell {
namespace {

TH1D* getHist(TFile& f, const std::string& path, bool required = true) {
  TH1D* h = dynamic_cast<TH1D*>(f.Get(path.c_str()));
  if (!h && required) throw std::runtime_error("histogram " + path + " not found in " + f.GetName());
  if (h) h->SetDirectory(nullptr);
  return h;
}
void ensureDir(const std::string& path) {
  std::string cur;
  for (const auto& part : splitString(path, '/')) { cur += (cur.empty() && path[0] != '/' ? "" : "/") + part; mkdir(cur.c_str(), 0755); }
}
std::string dirOf(const std::string& p) { const auto i = p.find_last_of('/'); return i == std::string::npos ? "." : p.substr(0, i); }

}  // namespace

std::vector<std::string> TemplateDatacard::write() {
  const std::string histFile = cfg_.at("histograms");
  const std::string var = cfg_.at("variable");
  const std::vector<std::string> regions = cfg_.at("regions");
  const std::vector<std::string> signals = cfg_.at("signal");
  const std::vector<std::string> backgrounds = cfg_.value("backgrounds", std::vector<std::string>{});
  const std::string data = cfg_.value("data", "data");
  const std::vector<std::string> shapeSysts = cfg_.value("shapeSystematics", std::vector<std::string>{});
  const std::string output = cfg_.value("output", "cards/hell");
  const int autoMCStats = cfg_.value("autoMCStats", 10);
  ensureDir(dirOf(output));
  std::unique_ptr<TFile> in(TFile::Open(histFile.c_str()));
  if (!in || in->IsZombie()) throw std::runtime_error("cannot open " + histFile);

  std::vector<std::string> cards;
  std::vector<std::string> processes = signals;
  processes.insert(processes.end(), backgrounds.begin(), backgrounds.end());
  for (const auto& region : regions) {
    const std::string stem = output + "_" + region;
    std::unique_ptr<TFile> shapes(TFile::Open((stem + ".root").c_str(), "RECREATE"));
    std::vector<double> rates;
    std::vector<std::vector<bool>> hasShape(shapeSysts.size(), std::vector<bool>(processes.size(), false));
    for (size_t ip = 0; ip < processes.size(); ++ip) {
      const auto& p = processes[ip];
      TH1D* h = getHist(*in, region + "/" + var + "/" + p);
      h->SetName(p.c_str()); shapes->WriteTObject(h);
      rates.push_back(h->Integral());
      for (size_t is = 0; is < shapeSysts.size(); ++is) {
        TH1D* up = getHist(*in, region + "/" + var + "/" + p + "_" + shapeSysts[is] + "Up", false);
        TH1D* dn = getHist(*in, region + "/" + var + "/" + p + "_" + shapeSysts[is] + "Down", false);
        if (!up || !dn) continue;
        hasShape[is][ip] = true;
        up->SetName((p + "_" + shapeSysts[is] + "Up").c_str()); dn->SetName((p + "_" + shapeSysts[is] + "Down").c_str());
        shapes->WriteTObject(up); shapes->WriteTObject(dn);
      }
    }
    TH1D* dobs = getHist(*in, region + "/" + var + "/" + data, false);
    double obs = -1;
    if (dobs) { dobs->SetName("data_obs"); shapes->WriteTObject(dobs); obs = dobs->Integral(); }
    else {
      // Asimov: sum of the expected templates.
      std::unique_ptr<TH1D> asimov;
      for (const auto& p : processes) { TH1D* h = getHist(*in, region + "/" + var + "/" + p); if (!asimov) asimov.reset(static_cast<TH1D*>(h->Clone("data_obs"))); else asimov->Add(h); }
      asimov->SetDirectory(nullptr); shapes->WriteTObject(asimov.get()); obs = asimov->Integral();
      HELL_WARN("datacard") << region << ": no data histogram '" << data << "', wrote an Asimov data_obs";
    }
    shapes->Close();

    std::ofstream card(stem + ".txt");
    card << "# HELL datacard: region " << region << ", variable " << var << ", from " << histFile << "\n";
    card << "imax 1 number of channels\njmax " << processes.size() - 1 << " number of backgrounds\nkmax * number of nuisance parameters\n";
    card << "---------------\nshapes * " << region << " " << baseName(stem) << ".root $PROCESS $PROCESS_$SYSTEMATIC\n---------------\n";
    card << "bin " << region << "\nobservation " << std::fixed << std::setprecision(3) << obs << "\n---------------\n";
    card << std::left << std::setw(28) << "bin"; for (size_t i = 0; i < processes.size(); ++i) card << std::setw(16) << region; card << "\n";
    card << std::setw(28) << "process"; for (const auto& p : processes) card << std::setw(16) << p; card << "\n";
    card << std::setw(28) << "process";
    for (size_t i = 0; i < processes.size(); ++i) card << std::setw(16) << (static_cast<int>(i) - static_cast<int>(signals.size()) + 1); card << "\n";
    card << std::setw(28) << "rate"; for (double r : rates) card << std::setw(16) << std::setprecision(4) << r; card << "\n---------------\n";
    if (cfg_.contains("normalisation"))
      for (auto it = cfg_["normalisation"].begin(); it != cfg_["normalisation"].end(); ++it) {
        const double value = it.value().at("value");
        const std::vector<std::string> targets = it.value().value("processes", std::vector<std::string>{"*"});
        card << std::setw(20) << it.key() << std::setw(8) << "lnN";
        for (const auto& p : processes) {
          bool applies = false; for (const auto& t : targets) if (globMatch(t, p)) applies = true;
          if (applies) card << std::setw(16) << std::setprecision(4) << value; else card << std::setw(16) << "-";
        }
        card << "\n";
      }
    for (size_t is = 0; is < shapeSysts.size(); ++is) {
      card << std::setw(20) << shapeSysts[is] << std::setw(8) << "shape";
      for (size_t ip = 0; ip < processes.size(); ++ip) card << std::setw(16) << (hasShape[is][ip] ? "1" : "-");
      card << "\n";
    }
    if (autoMCStats >= 0) card << region << " autoMCStats " << autoMCStats << "\n";
    card.close();
    cards.push_back(stem + ".txt");
    HELL_INFO("datacard") << "wrote " << stem << ".txt";
  }
  return cards;
}

std::string ParametricWorkspace::write() {
  // Signal: mass-dependent Gaussian core + Crystal-Ball tail is the usual
  // choice for m(l nu b); the skeleton fits a Crystal Ball whose mean and
  // width depend linearly on mtop, calibrated on the mass-variation templates.
  const std::string histFile = cfg_.at("histograms");
  const std::string var = cfg_.at("variable");
  const std::string region = cfg_.at("region");
  const std::string signal = cfg_.at("signal");
  const std::vector<std::string> backgrounds = cfg_.value("backgrounds", std::vector<std::string>{});
  const std::string data = cfg_.value("data", "data");
  const std::string output = cfg_.value("output", "cards/parametric");
  // {"166.5": "tch_mt1665", ...}: process names of the mass variations
  const nlohmann::json massPoints = cfg_.value("massPoints", nlohmann::json::object());
  ensureDir(dirOf(output));
  std::unique_ptr<TFile> in(TFile::Open(histFile.c_str()));
  if (!in || in->IsZombie()) throw std::runtime_error("cannot open " + histFile);
  TH1D* hSig = getHist(*in, region + "/" + var + "/" + signal);
  const double xlo = hSig->GetXaxis()->GetXmin(), xhi = hSig->GetXaxis()->GetXmax();

  RooWorkspace w("w", "HELL parametric workspace");
  RooRealVar x(var.c_str(), hSig->GetXaxis()->GetTitle(), xlo, xhi);
  RooRealVar mtop("mtop", "top quark mass [GeV]", 172.5, 150.0, 200.0);
  // Linear dependence of mean and width on mtop: mean = m0 + a*(mtop - 172.5)
  RooRealVar meanOffset("sig_mean0", "mean at 172.5", hSig->GetMean(), xlo, xhi);
  RooRealVar meanSlope("sig_mean_slope", "d mean / d mtop", 1.0, -5.0, 5.0);
  RooRealVar sigma0("sig_sigma0", "width at 172.5", hSig->GetRMS() / 2, 1.0, 100.0);
  RooRealVar sigmaSlope("sig_sigma_slope", "d sigma / d mtop", 0.0, -5.0, 5.0);
  RooFormulaVar mean("sig_mean", "@0 + @1*(@2-172.5)", RooArgList(meanOffset, meanSlope, mtop));
  RooFormulaVar sigma("sig_sigma", "@0 + @1*(@2-172.5)", RooArgList(sigma0, sigmaSlope, mtop));
  RooRealVar alpha("sig_alpha", "alpha", -1.0, -5.0, 0.0);
  RooRealVar n("sig_n", "n", 3.0, 0.5, 20.0);
  RooCBShape sigPdf((signal + "_pdf").c_str(), "signal", x, mean, sigma, alpha, n);

  // Calibrate the mtop dependence: fit each mass-point template with mtop fixed.
  if (!massPoints.empty()) {
    std::map<double, std::pair<double, double>> fitted;  // mtop -> (mean, sigma)
    for (auto it = massPoints.begin(); it != massPoints.end(); ++it) {
      const double m = std::stod(it.key());
      TH1D* h = getHist(*in, region + "/" + var + "/" + it.value().get<std::string>());
      RooDataHist dh(("dh_" + it.key()).c_str(), "", RooArgList(x), h);
      RooRealVar mu("mu", "", h->GetMean(), xlo, xhi), sg("sg", "", h->GetRMS() / 2, 1.0, 100.0);
      RooCBShape cb("cb", "", x, mu, sg, alpha, n);
      cb.fitTo(dh, RooFit::PrintLevel(-1), RooFit::SumW2Error(true), RooFit::Save(false));
      fitted[m] = {mu.getVal(), sg.getVal()};
      HELL_INFO("parametric") << "mtop " << m << ": mean " << mu.getVal() << " sigma " << sg.getVal();
    }
    // Straight-line fit (least squares) of mean and sigma vs mtop.
    auto line = [&](bool useMean) {
      double sx = 0, sy = 0, sxx = 0, sxy = 0; const double nn = fitted.size();
      for (const auto& kv : fitted) { const double xx = kv.first - 172.5, yy = useMean ? kv.second.first : kv.second.second; sx += xx; sy += yy; sxx += xx * xx; sxy += xx * yy; }
      const double slope = (nn * sxy - sx * sy) / std::max(nn * sxx - sx * sx, 1e-9);
      return std::make_pair((sy - slope * sx) / nn, slope);
    };
    const auto lm = line(true), ls = line(false);
    meanOffset.setVal(lm.first); meanSlope.setVal(lm.second); sigma0.setVal(ls.first); sigmaSlope.setVal(ls.second);
    meanOffset.setConstant(true); meanSlope.setConstant(true); sigma0.setConstant(true); sigmaSlope.setConstant(true);
    HELL_INFO("parametric") << "mean = " << lm.first << " + " << lm.second << " (mtop-172.5); sigma = " << ls.first << " + " << ls.second << " (mtop-172.5)";
  }
  w.import(sigPdf);
  // Backgrounds as binned templates.
  for (const auto& b : backgrounds) {
    TH1D* h = getHist(*in, region + "/" + var + "/" + b);
    RooDataHist dh((b + "_dh").c_str(), "", RooArgList(x), h);
    RooHistPdf pdf((b + "_pdf").c_str(), b.c_str(), RooArgSet(x), dh);
    w.import(pdf);
  }
  TH1D* hData = getHist(*in, region + "/" + var + "/" + data, false);
  std::unique_ptr<TH1D> asimov;
  if (!hData) {
    asimov.reset(static_cast<TH1D*>(hSig->Clone("data_obs")));
    for (const auto& b : backgrounds) asimov->Add(getHist(*in, region + "/" + var + "/" + b));
    hData = asimov.get();
    HELL_WARN("parametric") << "no data histogram, using Asimov data";
  }
  RooDataHist dobs("data_obs", "data", RooArgList(x), hData);
  w.import(dobs);
  w.writeToFile((output + ".root").c_str());

  std::ofstream card(output + ".txt");
  card << "# HELL parametric datacard: region " << region << ", variable " << var << "\n";
  card << "imax 1\njmax " << backgrounds.size() << "\nkmax *\n---------------\n";
  card << "shapes " << signal << " " << region << " " << baseName(output) << ".root w:" << signal << "_pdf\n";
  for (const auto& b : backgrounds) card << "shapes " << b << " " << region << " " << baseName(output) << ".root w:" << b << "_pdf\n";
  card << "shapes data_obs " << region << " " << baseName(output) << ".root w:data_obs\n---------------\n";
  card << "bin " << region << "\nobservation -1\n---------------\n";
  card << std::left << std::setw(20) << "bin"; card << std::setw(16) << region; for (size_t i = 0; i < backgrounds.size(); ++i) card << std::setw(16) << region; card << "\n";
  card << std::setw(20) << "process" << std::setw(16) << signal; for (const auto& b : backgrounds) card << std::setw(16) << b; card << "\n";
  card << std::setw(20) << "process" << std::setw(16) << 0; for (size_t i = 0; i < backgrounds.size(); ++i) card << std::setw(16) << i + 1; card << "\n";
  card << std::setw(20) << "rate" << std::setw(16) << std::setprecision(4) << hSig->Integral();
  for (const auto& b : backgrounds) card << std::setw(16) << getHist(*in, region + "/" + var + "/" + b)->Integral(); card << "\n---------------\n";
  if (cfg_.contains("normalisation"))
    for (auto it = cfg_["normalisation"].begin(); it != cfg_["normalisation"].end(); ++it) {
      const double value = it.value().at("value");
      const std::vector<std::string> targets = it.value().value("processes", std::vector<std::string>{"*"});
      card << std::setw(20) << it.key() << std::setw(8) << "lnN";
      std::vector<std::string> procs{signal}; procs.insert(procs.end(), backgrounds.begin(), backgrounds.end());
      for (const auto& p : procs) { bool a = false; for (const auto& t : targets) if (globMatch(t, p)) a = true; card << std::setw(16) << (a ? std::to_string(value) : "-"); }
      card << "\n";
    }
  card << "mtop flatParam\n";
  card.close();
  HELL_INFO("parametric") << "wrote " << output << ".root and " << output << ".txt (POI: mtop)";
  return output + ".txt";
}

}  // namespace hell
