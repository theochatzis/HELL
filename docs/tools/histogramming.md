# Histogramming

`hell hist configs/SingleTopMass/hist.yaml` builds the JSON for `hell-hist` and runs it. Cuts, weights and
variables are TTree expressions evaluated with `TTreeFormula` on the analysis trees, so a new region or
variable is a line of YAML.

```yaml
era: "2022"                        # lumi and cross sections from samples/2022.yaml and samples/xsec.yaml
input: /eos/user/x/HELL/trees/2022 # <input>/<sample>/*.root
variationInput: /eos/user/x/HELL/trees/2022_{variation}
output: hists/singletop_2022.root
processes:
  - {name: tch, label: "t-channel", color: "#e42536", samples: [TBbarQtoLNu_tch, TbarBQtoLNu_tch]}
  - {name: ttB, label: "t#bar{t}+b", color: "#f89c20", samples: [TTtoLNu2Q], cut: "Jet_hadronFlavour[0]==5"}   # sub-process by a cut
  - {name: data, label: "Data", isData: true, samples: [Muon_2022C]}
regions:
  2j1b_mu: {selection: "st_nJet==2 && st_nBJet==1 && st_isMuon==1 && st_mtw>50", weight: "Weight_nominal"}
variables:
  topMass: {expr: st_topMass, bins: [40, 100, 300], title: "m_{l#nub} [GeV]"}      # or edges: [100, 120, ...]
systematics:
  pu:  {type: weight, up: "Weight_puUp/Weight_nominal", down: "Weight_puDown/Weight_nominal"}
  jes: {type: tree, up: jesUp, down: jesDown}
```
- MC scale per sample: `lumi * xsec / sumw`, `sumw` = sum of `genEventSumw` over the sample files (`hell/counts`).
  Data is unscaled. The scale factors used are stored in the output provenance (`normalisation`).
- Scale factors and weight systematics are columns of the `Weight` table written by the analysis step
  (`Weight_nominal`, `Weight_puUp`, ...): the region weight and the systematic expressions refer to them.
- Object systematics (`type: tree`) use the trees produced with `variation=jesUp` etc. (`hell unleash-systematics`).
- Output: `<region>/<variable>/<process>` and `<process>_<syst>Up|Down`, plus `hell/hist_config` and provenance.
- `hell plot hists.root -o plots` makes stacked data/MC plots with ratio from the stored configuration.
- Batch: `hell submit hist hist.yaml --output-dir ... --tag hist_v1` runs one job per process; `hadd` the outputs.
- Tests: `hell hist hist.yaml -n 10000 --process tch`.
