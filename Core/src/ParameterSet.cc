#include "Core/interface/ParameterSet.h"
#include <fstream>

namespace hell {

ParameterSet ParameterSet::fromFile(const std::string& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot open configuration file " + path);
  json j;
  try { in >> j; } catch (const json::exception& e) { throw std::runtime_error("invalid JSON in " + path + ": " + e.what()); }
  ParameterSet ps(j);
  ps.setContext(path);
  return ps;
}

ParameterSet ParameterSet::fromString(const std::string& text) {
  try { return ParameterSet(json::parse(text)); } catch (const json::exception& e) { throw std::runtime_error(std::string("invalid JSON: ") + e.what()); }
}

std::vector<std::string> ParameterSet::keys() const {
  std::vector<std::string> k;
  for (auto it = data_.begin(); it != data_.end(); ++it) k.push_back(it.key());
  return k;
}

ParameterSet ParameterSet::getPSet(const std::string& key) const {
  if (!data_.contains(key)) throw std::runtime_error("missing mandatory parameter set '" + key + "' in " + context_);
  if (!data_.at(key).is_object()) throw std::runtime_error("parameter '" + key + "' in " + context_ + " is not an object");
  record(key, data_.at(key), false);
  ParameterSet ps(data_.at(key));
  ps.setContext(context_ + "." + key);
  return ps;
}

ParameterSet ParameterSet::getPSet(const std::string& key, const ParameterSet& def) const {
  if (!data_.contains(key)) { record(key, def.raw(), true); return def; }
  return getPSet(key);
}

std::vector<ParameterSet> ParameterSet::getVPSet(const std::string& key) const {
  if (!data_.contains(key)) throw std::runtime_error("missing mandatory list of parameter sets '" + key + "' in " + context_);
  if (!data_.at(key).is_array()) throw std::runtime_error("parameter '" + key + "' in " + context_ + " is not a list");
  record(key, data_.at(key), false);
  std::vector<ParameterSet> out;
  size_t i = 0;
  for (const auto& el : data_.at(key)) {
    if (!el.is_object()) throw std::runtime_error("element " + std::to_string(i) + " of '" + key + "' in " + context_ + " is not an object");
    ParameterSet ps(el);
    ps.setContext(context_ + "." + key + "[" + std::to_string(i++) + "]");
    out.push_back(ps);
  }
  return out;
}

ParameterSet::json ParameterSet::accessedAsJson() const {
  json j = json::object();
  for (const auto& kv : accessed_) j[kv.first] = {{"value", kv.second.value}, {"default", kv.second.isDefault}};
  return j;
}

}  // namespace hell
