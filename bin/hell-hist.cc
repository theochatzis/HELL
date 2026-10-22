// hell-hist: config-driven histogramming of HELL analysis trees.
#include "CliHelp.h"
#include "Core/interface/ParameterSet.h"
#include "Core/interface/Version.h"
#include "Hist/interface/Histogrammer.h"
#include <iostream>

int main(int argc, char** argv) {
  hell::Cli cli(argc, argv);
  if (cli.has("--help") || cli.has("-h") || argc < 2)
    return hell::printHelp("hell-hist " HELL_VERSION, "hell-hist hist_config.json [-o out.root] [-n N]",
                           "  -o FILE      output file (overrides the config)\n  -n N         at most N events per sample (tests)\n",
                           "  hell-hist work/hist_singletop.json -o hists.root\n  (normally called through `hell hist configs/SingleTopMass/hist.yaml`)");
  const auto pos = cli.positionals({"-o", "-n"});
  if (pos.empty()) { std::cerr << "[HELL] ERROR: no configuration file given\n"; return 1; }
  try {
    hell::ParameterSet cfg = hell::ParameterSet::fromFile(pos[0]);
    nlohmann::json j = cfg.raw();
    if (!cli.value("-o").empty()) j["output"] = cli.value("-o");
    if (!cli.value("-n").empty()) j["maxEvents"] = std::stoll(cli.value("-n"));
    hell::Histogrammer h(j);
    return h.run();
  } catch (const std::exception& e) {
    std::cerr << "[HELL] ERROR: " << e.what() << "\n";
    return 1;
  }
}
