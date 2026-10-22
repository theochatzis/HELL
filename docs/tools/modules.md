# Writing a module

A module is a C++ class in `Modules/<Package>/plugins/*.cc`, registered with `HELL_REGISTER_MODULE`. The package's
`CMakeLists.txt` lists the sources: `hell_add_plugin(Package SOURCES plugins/A.cc plugins/B.cc)`. New packages
are picked up automatically (a directory with a `CMakeLists.txt` under `Modules/`).

```cpp
// Modules/Top/plugins/LeadingJetAnalyzer.cc
// Histograms the pt of the leading selected jet.
// python: process.leadJet = Module("LeadingJetAnalyzer", jets="bTags")
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/DataFormats.h"
#include <TH1D.h>

namespace hell {
class LeadingJetAnalyzer : public Analyzer {
 public:
  LeadingJetAnalyzer(const ParameterSet& ps, ModuleContext& ctx) : Analyzer(ctx) {
    jets_ = ctx.consumesProduct(ps.get<std::string>("jets"));         // product read (validated against the path)
    minPt_ = ps.get<double>("minPt", 0.0);                            // optional parameter with default
    h_ = ctx.hists().book1D("leadingPt", ";leading jet p_{T} [GeV];events", 60, 0, 300);  // in directory <label>/
  }
  void analyze(const Event& ev) override {
    const auto& jets = ev.get<JetCollection>(jets_);
    if (!jets.empty() && jets[0].pt > minPt_) h_->Fill(jets[0].pt, ev.weight);
  }
 private:
  std::string jets_; double minPt_; TH1D* h_ = nullptr;
};
}  // namespace hell
HELL_REGISTER_MODULE(hell::LeadingJetAnalyzer, "Histogram of the leading jet pt");
```

## Kinds
- `Producer::produce(Event&)`: declare products with `ctx.produces<T>(instance)` in the constructor and put them
  with `ev.put(label(), std::move(obj))` (or the full label returned by `produces`).
- `Filter::filter(Event&) -> bool`: `false` stops the path for the event; the cutflow records the pass counts.
- `Analyzer::analyze(const Event&)`: histograms through `ctx.hists()` (one directory per module), trees through
  `OutputTree` (see `TreeWriter`).

## Reading input branches
Declare in the constructor: `pt_ = ctx.consumesArray<float>("Jet_pt")`, `n_ = ctx.consumesValue<int>("nJet")`.
Optional branches: `ctx.consumesArray<float>("Jet_x", true)` and test `.valid()`. Types are converted from the
on-disk type. Only reader modules should do this; physics modules consume products.

## Job context
`ctx.job()` gives `isData`, `era`, `sample`, `variation`, `crossSection`. A module that implements an object
variation checks `ctx.job().variation` and is transparent otherwise (`JetVariation` is the example).

## Lifecycle
`beginJob()`, `beginFile(path)`, per-event method, `endFile`, `endJob()`. Histograms are written by the framework.

## Making a product persistent
A product type is writable by `TreeWriter` when it has a converter in `DataFormats/src/Persistence.cc`
(`PersistenceRegistry::instance().add<T>(...)`) and a line in the `TreeWriter` lookup. `FlatTable` products
(one row or many) need nothing: build the table directly, as `SingleTopVariables` does.

## Checklist
- parameters read in the constructor with defaults; errors thrown there with a clear message
- no static state; several instances must coexist
- header comment with the python usage line; registration with a one-line doc
- histogram and column names are stable identifiers with doc strings
- add the module to `docs/tools/` if it introduces a new concept
