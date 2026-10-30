#include "Corrections/interface/CorrectionService.h"
#include "Core/interface/Logger.h"
#include <stdexcept>

namespace hell {

CorrectionService& CorrectionService::instance() { static CorrectionService s; return s; }

const correction::CorrectionSet& CorrectionService::set(const std::string& file) {
  auto it = sets_.find(file);
  if (it == sets_.end()) {
    try {
      it = sets_.emplace(file, correction::CorrectionSet::from_file(file)).first;
    } catch (const std::exception& e) {
      throw std::runtime_error("cannot open correction file " + file + ": " + e.what());
    }
    HELL_INFO("corrections") << "loaded " << file << " (" << it->second->size() << " corrections)";
  }
  return *it->second;
}

CorrectionService::Ref CorrectionService::get(const std::string& file, const std::string& name) {
  const auto& s = set(file);
  try {
    return s.at(name);
  } catch (const std::out_of_range&) {
    std::string known;
    for (const auto& kv : s) known += (known.empty() ? "" : ", ") + kv.first;
    throw std::runtime_error("correction '" + name + "' not in " + file + ". Available: " + known);
  }
}

CorrectionService::CompoundRef CorrectionService::getCompound(const std::string& file, const std::string& name) {
  const auto& s = set(file);
  try {
    return s.compound().at(name);
  } catch (const std::out_of_range&) {
    throw std::runtime_error("compound correction '" + name + "' not in " + file);
  }
}

bool CorrectionService::has(const std::string& file, const std::string& name) {
  const auto& s = set(file);
  for (const auto& kv : s) if (kv.first == name) return true;
  return false;
}

std::vector<std::string> CorrectionService::names(const std::string& file) {
  std::vector<std::string> v;
  for (const auto& kv : set(file)) v.push_back(kv.first);
  return v;
}

}  // namespace hell
