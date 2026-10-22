// Named event weights with their systematic variations. The nominal event
// weight is the product of all nominal factors. A variation of weight "pu"
// replaces the "pu" factor by its up/down value, all other factors unchanged.
#pragma once
#include <map>
#include <string>
#include <vector>

namespace hell {

struct WeightEntry {
  std::string name;
  double nominal = 1;
  double up = 1;
  double down = 1;
  bool hasVariation = false;
};

class Weights {
 public:
  void set(const std::string& name, double nominal) { entry(name).nominal = nominal; }
  void set(const std::string& name, double nominal, double up, double down) {
    WeightEntry& e = entry(name); e.nominal = nominal; e.up = up; e.down = down; e.hasVariation = true;
  }
  bool has(const std::string& name) const { for (const auto& e : entries_) if (e.name == name) return true; return false; }
  double get(const std::string& name) const { for (const auto& e : entries_) if (e.name == name) return e.nominal; return 1.0; }
  double nominal() const { double w = 1; for (const auto& e : entries_) w *= e.nominal; return w; }
  double varied(const std::string& name, bool up) const {
    double w = 1;
    for (const auto& e : entries_) w *= (e.name == name && e.hasVariation) ? (up ? e.up : e.down) : e.nominal;
    return w;
  }
  const std::vector<WeightEntry>& entries() const { return entries_; }

 private:
  WeightEntry& entry(const std::string& name) {
    for (auto& e : entries_) if (e.name == name) return e;
    entries_.push_back(WeightEntry{name, 1, 1, 1, false});
    return entries_.back();
  }
  std::vector<WeightEntry> entries_;
};

}  // namespace hell
