#include "Core/interface/Persistence.h"
#include <stdexcept>

namespace hell {

PersistenceRegistry& PersistenceRegistry::instance() { static PersistenceRegistry r; return r; }

PersistenceRegistry::PersistenceRegistry() {
  add<double>([](const double& v, const std::string& n) { FlatTable t(n, 1, true); t.addColumn("value", std::vector<double>{v}); return t; });
  add<float>([](const float& v, const std::string& n) { FlatTable t(n, 1, true); t.addColumn("value", std::vector<float>{v}); return t; });
  add<int>([](const int& v, const std::string& n) { FlatTable t(n, 1, true); t.addColumn("value", std::vector<int>{v}); return t; });
  add<bool>([](const bool& v, const std::string& n) { FlatTable t(n, 1, true); t.addColumn("value", std::vector<bool>{v}); return t; });
  add<std::vector<float>>([](const std::vector<float>& v, const std::string& n) { FlatTable t(n, v.size()); t.addColumn("value", v); return t; });
  add<std::vector<double>>([](const std::vector<double>& v, const std::string& n) { FlatTable t(n, v.size()); t.addColumn("value", v); return t; });
  add<std::vector<int>>([](const std::vector<int>& v, const std::string& n) { FlatTable t(n, v.size()); t.addColumn("value", v); return t; });
  add<FlatTable>([](const FlatTable& v, const std::string& n) {
    if (n.empty() || n == v.name()) return v;
    FlatTable t(n, v.rows(), v.singleton());
    for (const auto& c : v.columns()) {
      if (c.type == FlatTable::ColumnType::Float) t.addColumn(c.name, c.f, c.doc);
      else if (c.type == FlatTable::ColumnType::Int) t.addColumn(c.name, c.i, c.doc);
      else t.addColumn(c.name, c.b, c.doc);
    }
    return t;
  });
}

FlatTable PersistenceRegistry::convert(const ProductHolderBase& holder, const std::string& tableName) const {
  auto it = converters_.find(holder.type());
  if (it == converters_.end()) throw std::runtime_error("no persistence converter registered for type " + demangle(holder.type().name()) + " (table " + tableName + ")");
  return it->second(holder, tableName);
}

std::vector<std::string> PersistenceRegistry::convertibleTypes() const {
  std::vector<std::string> v;
  for (const auto& kv : converters_) v.push_back(demangle(kv.first.name()));
  return v;
}

}  // namespace hell
