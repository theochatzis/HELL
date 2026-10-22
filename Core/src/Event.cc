#include "Core/interface/Event.h"
#include <cxxabi.h>
#include <cstdlib>

namespace hell {

std::string demangle(const char* name) {
  int status = 0;
  char* d = abi::__cxa_demangle(name, nullptr, nullptr, &status);
  std::string out = (status == 0 && d) ? d : name;
  std::free(d);
  return out;
}

std::vector<std::string> Event::labels() const {
  std::vector<std::string> v;
  for (const auto& kv : products_) v.push_back(kv.first);
  return v;
}

std::string Event::resolve(const std::string& label) const {
  if (products_.count(label)) return label;
  if (label.find(':') != std::string::npos) return label;
  std::string found;
  int n = 0;
  const std::string prefix = label + ":";
  for (const auto& kv : products_) {
    if (kv.first.compare(0, prefix.size(), prefix) == 0) { found = kv.first; ++n; }
  }
  return n == 1 ? found : label;
}

std::string Event::hint(const std::string& label) const {
  if (products_.empty()) return " (the event holds no products; is a producer missing from the path?)";
  std::string h = ". Available: ";
  bool first = true;
  for (const auto& kv : products_) { h += (first ? "" : ", ") + kv.first; first = false; }
  return h;
}

}  // namespace hell
