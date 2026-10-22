// Tiny argument helper shared by the HELL executables. Every executable has
// a --help that shows the usage AND an example, as a matter of principle.
#pragma once
#include <iostream>
#include <string>
#include <vector>

namespace hell {
struct Cli {
  std::vector<std::string> args;
  Cli(int argc, char** argv) : args(argv + 1, argv + argc) {}
  bool has(const std::string& f) const { for (const auto& a : args) if (a == f) return true; return false; }
  std::string value(const std::string& f, const std::string& def = "") const {
    for (size_t i = 0; i + 1 < args.size(); ++i) if (args[i] == f) return args[i + 1];
    return def;
  }
  std::vector<std::string> values(const std::string& f) const {
    std::vector<std::string> v;
    for (size_t i = 0; i + 1 < args.size(); ++i) if (args[i] == f) v.push_back(args[i + 1]);
    return v;
  }
  std::vector<std::string> positionals(const std::vector<std::string>& flagsWithValue) const {
    std::vector<std::string> v;
    for (size_t i = 0; i < args.size(); ++i) {
      bool isFlag = !args[i].empty() && args[i][0] == '-';
      if (isFlag) { for (const auto& f : flagsWithValue) if (args[i] == f) { ++i; break; } continue; }
      v.push_back(args[i]);
    }
    return v;
  }
};
inline int printHelp(const std::string& name, const std::string& usage, const std::string& options, const std::string& example) {
  std::cout << "[HELL] " << name << "\n\nUsage:\n  " << usage << "\n\nOptions:\n" << options << "\nExample:\n" << example << "\n";
  return 0;
}
}  // namespace hell
