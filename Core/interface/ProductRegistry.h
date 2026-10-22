// Records which module produces which product (label, type). Filled when
// modules are constructed, used to validate the path and for --dump.
#pragma once
#include <string>
#include <typeindex>
#include <vector>

namespace hell {

struct ProductInfo {
  std::string label;     // full label, e.g. "jets" or "jets:clean"
  std::string module;    // module label
  std::string instance;  // instance name (may be empty)
  std::type_index type;
  std::string typeName;  // demangled
};

class ProductRegistry {
 public:
  void add(const ProductInfo& p);
  const std::vector<ProductInfo>& products() const { return products_; }
  bool has(const std::string& label) const;
  const ProductInfo* find(const std::string& label) const;
  std::vector<const ProductInfo*> byModule(const std::string& module) const;

 private:
  std::vector<ProductInfo> products_;
};

}  // namespace hell
