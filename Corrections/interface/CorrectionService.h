// Access to correctionlib JSONs (the central POG corrections). Files are opened once per job and shared by
// all modules; corrections are looked up by file and name. Inputs are passed as variants of int, double
// and string in the order the correction declares them.
//
//   const auto& c = CorrectionService::instance();
//   auto wp = c.get(file, "particleNet_wp_values");
//   double medium = wp->evaluate({std::string("M")});
#pragma once
#include <correction.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace hell {

class CorrectionService {
 public:
  using Ref = correction::Correction::Ref;
  using CompoundRef = correction::CompoundCorrection::Ref;
  using Value = correction::Variable::Type;

  static CorrectionService& instance();
  const correction::CorrectionSet& set(const std::string& file);
  Ref get(const std::string& file, const std::string& name);
  CompoundRef getCompound(const std::string& file, const std::string& name);
  bool has(const std::string& file, const std::string& name);
  std::vector<std::string> names(const std::string& file);

 private:
  std::map<std::string, std::unique_ptr<correction::CorrectionSet>> sets_;
};

}  // namespace hell
