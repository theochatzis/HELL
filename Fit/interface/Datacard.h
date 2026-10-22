// Writers of Higgs Combine inputs from HELL histogram files.
//
// TemplateDatacard: shape analysis with TH1 templates. Writes, per region,
//   <name>_<region>.root  with  <process>, <process>_<syst>(Up|Down), data_obs
//   <name>_<region>.txt   the datacard (shape systematics present in the
//                         file, lnN normalisation systematics from config)
//   <name>.txt            combined card if several regions.
// ParametricWorkspace: RooFit workspace with a parametric signal model whose
//   shape parameters depend linearly on the top mass (mtop as POI), built
//   from the mass-variation templates, plus binned backgrounds. The skeleton
//   provides the mtop-dependent Gaussian-core model; shapes can be swapped.
//
// JSON configuration (from `hell fit --datacards`):
// {"histograms": "hists.root", "variable": "topMass", "regions": ["2j1b_mu"],
//  "signal": ["tch"], "backgrounds": ["tt","wjets"], "data": "data",
//  "normalisation": {"lumi": {"value": 1.014, "processes": ["*"]}, "xsec_tt": {"value": 1.05, "processes": ["tt"]}},
//  "shapeSystematics": ["pu","jes"], "output": "cards/st", "autoMCStats": 10}
#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace hell {

class TemplateDatacard {
 public:
  explicit TemplateDatacard(const nlohmann::json& cfg) : cfg_(cfg) {}
  // Returns the paths of the written datacards.
  std::vector<std::string> write();

 private:
  nlohmann::json cfg_;
};

class ParametricWorkspace {
 public:
  explicit ParametricWorkspace(const nlohmann::json& cfg) : cfg_(cfg) {}
  // Builds the workspace file and a datacard using it; returns the datacard path.
  std::string write();

 private:
  nlohmann::json cfg_;
};

}  // namespace hell
