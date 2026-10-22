#include "Core/interface/ProductRegistry.h"
#include <stdexcept>

namespace hell {
void ProductRegistry::add(const ProductInfo& p) {
  if (has(p.label)) throw std::runtime_error("product label '" + p.label + "' declared twice");
  products_.push_back(p);
}
bool ProductRegistry::has(const std::string& label) const { return find(label) != nullptr; }
const ProductInfo* ProductRegistry::find(const std::string& label) const {
  for (const auto& p : products_) if (p.label == label) return &p;
  // "module" resolves to its single product
  const ProductInfo* found = nullptr; int n = 0;
  for (const auto& p : products_) if (p.module == label) { found = &p; ++n; }
  return n == 1 ? found : nullptr;
}
std::vector<const ProductInfo*> ProductRegistry::byModule(const std::string& module) const {
  std::vector<const ProductInfo*> v;
  for (const auto& p : products_) if (p.module == module) v.push_back(&p);
  return v;
}
}  // namespace hell
