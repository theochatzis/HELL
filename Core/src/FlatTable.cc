#include "Core/interface/FlatTable.h"

namespace hell {
void FlatTable::check(const std::string& name, size_t n) const {
  if (n != rows_) throw std::runtime_error("FlatTable " + name_ + ": column " + name + " has " + std::to_string(n) + " rows, expected " + std::to_string(rows_));
  for (const auto& c : columns_) if (c.name == name) throw std::runtime_error("FlatTable " + name_ + ": duplicate column " + name);
}
void FlatTable::addColumn(const std::string& name, std::vector<float> v, const std::string& doc) {
  check(name, v.size()); Column c; c.name = name; c.type = ColumnType::Float; c.doc = doc; c.f = std::move(v); columns_.push_back(std::move(c));
}
void FlatTable::addColumn(const std::string& name, std::vector<int> v, const std::string& doc) {
  check(name, v.size()); Column c; c.name = name; c.type = ColumnType::Int; c.doc = doc; c.i = std::move(v); columns_.push_back(std::move(c));
}
void FlatTable::addColumn(const std::string& name, std::vector<bool> v, const std::string& doc) {
  check(name, v.size()); Column c; c.name = name; c.type = ColumnType::Bool; c.doc = doc; c.b = std::move(v); columns_.push_back(std::move(c));
}
void FlatTable::addColumn(const std::string& name, std::vector<double> v, const std::string& doc) {
  std::vector<float> f(v.begin(), v.end()); addColumn(name, std::move(f), doc);
}
}  // namespace hell
