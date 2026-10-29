// hell-run: forge events through a configured path (skim, analysis tree, histograms).
#include "CliHelp.h"
#include "Core/interface/Forge.h"
#include "Core/interface/Logger.h"
#include "Core/interface/ModuleFactory.h"
#include "Core/interface/Version.h"
#include "DataFormats/interface/DataFormats.h"
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
  hell::Cli cli(argc, argv);
  const std::string options =
      "  -n, --max-events N     process at most N events (-1: all)\n"
      "  -s, --skip-events N    skip the first N events\n"
      "  -i, --input FILE       input file, @list.txt or directory (repeatable; overrides the config)\n"
      "  -o, --output FILE      output file (overrides the config)\n"
      "  --variation NAME       active object-level variation (nominal, jesUp, ...)\n"
      "  --dump                 print the resolved modules, parameters and products, then exit\n"
      "  --list-modules         list the available module types (JSON with --json)\n"
      "  -v, --verbose          debug logging\n";
  if (cli.has("--help") || cli.has("-h") || argc < 2)
    return hell::printHelp("hell-run " HELL_VERSION " (" HELL_GIT_HASH ")", "hell-run config.json [options]", options,
                           "  hell-run work/skim_singletop_cfg.json -n 1000 -o test_skim.root\n"
                           "  (normally called through `hell run configs/SingleTopMass/skim_cfg.py sample=... maxEvents=1000`)");
  hell::installRootMessageFilter();
  hell::loadDataFormats();
  if (cli.has("--version")) { std::cout << "HELL " << HELL_VERSION << " " << HELL_GIT_HASH << "\n"; return 0; }
  if (cli.has("--list-modules")) {
    hell::Logger::setLevel(hell::LogLevel::Warning);
    hell::PluginLoader::loadAll();
    const auto mods = hell::ModuleFactory::instance().available();
    if (cli.has("--json")) {
      nlohmann::json j = nlohmann::json::array();
      for (const auto& m : mods) j.push_back({{"type", m.type}, {"doc", m.doc}});
      std::cout << j.dump(2) << "\n";
    } else {
      std::cout << "[HELL] " << mods.size() << " module types available:\n";
      for (const auto& m : mods) std::cout << "  " << std::left << std::setw(26) << m.type << m.doc << "\n";
    }
    return 0;
  }
  const auto pos = cli.positionals({"-n", "--max-events", "-s", "--skip-events", "-i", "--input", "-o", "--output", "--variation"});
  if (pos.empty()) { std::cerr << "[HELL] ERROR: no configuration file given (try --help)\n"; return 1; }
  try {
    hell::ParameterSet cfg = hell::ParameterSet::fromFile(pos[0]);
    if (cli.has("-v") || cli.has("--verbose")) cfg.raw()["options"]["logLevel"] = "DEBUG";
    hell::Forge forge(cfg);
    forge.setCommandLine(argc, argv);
    const std::string n = cli.value("-n", cli.value("--max-events", ""));
    if (!n.empty()) forge.setMaxEvents(std::stoll(n));
    const std::string s = cli.value("-s", cli.value("--skip-events", ""));
    if (!s.empty()) forge.setSkipEvents(std::stoll(s));
    auto inputs = cli.values("-i"); for (const auto& x : cli.values("--input")) inputs.push_back(x);
    if (!inputs.empty()) forge.setInputFiles(inputs);
    const std::string o = cli.value("-o", cli.value("--output", ""));
    if (!o.empty()) forge.setOutputFile(o);
    const std::string var = cli.value("--variation", "");
    if (!var.empty()) forge.setVariation(var);
    if (cli.has("--dump")) {
      // No real output for a dump: write to a scratch file and remove it.
      const std::string scratch = std::string(std::getenv("TMPDIR") ? std::getenv("TMPDIR") : "/tmp") + "/hell_dump_" + std::to_string(getpid()) + ".root";
      forge.setOutputFile(scratch);
      forge.initialize();
      std::cout << forge.summary() << "\n" << forge.describe().dump(2) << "\n";
      std::remove(scratch.c_str());
      return 0;
    }
    return forge.run();
  } catch (const std::exception& e) {
    std::cerr << "[HELL] ERROR: " << e.what() << "\n[HELL] The forge went cold.\n";
    return 1;
  }
}
