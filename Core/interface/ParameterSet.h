// A thin, typed wrapper around nlohmann::json that plays the role of the
// CMSSW ParameterSet. Modules read their configuration through it. Every
// parameter that a module reads is recorded (name, value, whether it came
// from the config or from a default) so that `hell-run --dump` can document
// what a module actually used.
#pragma once
#include <nlohmann/json.hpp>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace hell {

class ParameterSet {
 public:
  using json = nlohmann::json;

  ParameterSet() : data_(json::object()) {}
  explicit ParameterSet(json j) : data_(std::move(j)) {
    if (!data_.is_object()) throw std::runtime_error("ParameterSet must be built from a JSON object");
  }
  static ParameterSet fromFile(const std::string& path);
  static ParameterSet fromString(const std::string& text);

  bool has(const std::string& key) const { return data_.contains(key); }
  std::vector<std::string> keys() const;
  bool empty() const { return data_.empty(); }

  // Mandatory parameter: throws with context if missing or of the wrong type.
  template <class T>
  T get(const std::string& key) const {
    if (!data_.contains(key)) throw std::runtime_error("missing mandatory parameter '" + key + "' in " + context_);
    try {
      T v = data_.at(key).get<T>();
      record(key, data_.at(key), false);
      return v;
    } catch (const json::exception& e) {
      throw std::runtime_error("parameter '" + key + "' in " + context_ + " has the wrong type: " + e.what());
    }
  }
  // Optional parameter with default.
  template <class T>
  T get(const std::string& key, const T& def) const {
    if (!data_.contains(key)) {
      record(key, json(def), true);
      return def;
    }
    return get<T>(key);
  }
  // Convenience for string literals.
  std::string get(const std::string& key, const char* def) const { return get<std::string>(key, std::string(def)); }

  ParameterSet getPSet(const std::string& key) const;
  ParameterSet getPSet(const std::string& key, const ParameterSet& def) const;
  std::vector<ParameterSet> getVPSet(const std::string& key) const;

  const json& raw() const { return data_; }
  json& raw() { return data_; }
  std::string dump(int indent = 2) const { return data_.dump(indent); }

  void setContext(const std::string& c) { context_ = c; }
  const std::string& context() const { return context_; }

  // Parameters accessed so far: key -> {value, isDefault}
  struct Access { json value; bool isDefault; };
  const std::map<std::string, Access>& accessed() const { return accessed_; }
  json accessedAsJson() const;

 private:
  void record(const std::string& key, const json& v, bool isDefault) const {
    auto it = accessed_.find(key);
    if (it == accessed_.end()) accessed_[key] = Access{v, isDefault};
  }
  json data_;
  std::string context_ = "parameter set";
  mutable std::map<std::string, Access> accessed_;
};

}  // namespace hell
