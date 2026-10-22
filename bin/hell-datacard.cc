// hell-datacard: write Combine datacards (template or parametric) from HELL histograms.
#include "CliHelp.h"
#include "Core/interface/ParameterSet.h"
#include "Core/interface/Version.h"
#include "Fit/interface/Datacard.h"
#include <iostream>

int main(int argc, char** argv) {
  hell::Cli cli(argc, argv);
  if (cli.has("--help") || cli.has("-h") || argc < 2)
    return hell::printHelp("hell-datacard " HELL_VERSION, "hell-datacard fit_config.json [--parametric]",
                           "  --parametric   build a RooFit workspace with an mtop-dependent signal model instead of templates\n",
                           "  hell-datacard work/fit_singletop.json\n  hell-datacard work/fit_singletop.json --parametric\n  (normally called through `hell fit --datacards`)");
  const auto pos = cli.positionals({});
  if (pos.empty()) { std::cerr << "[HELL] ERROR: no configuration file given\n"; return 1; }
  try {
    hell::ParameterSet cfg = hell::ParameterSet::fromFile(pos[0]);
    if (cli.has("--parametric")) { hell::ParametricWorkspace w(cfg.raw()); std::cout << w.write() << "\n"; }
    else { hell::TemplateDatacard d(cfg.raw()); for (const auto& c : d.write()) std::cout << c << "\n"; }
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "[HELL] ERROR: " << e.what() << "\n";
    return 1;
  }
}
