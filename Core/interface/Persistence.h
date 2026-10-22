// Registry of converters product type -> FlatTable. Each DataFormat that
// should be writable to a tree registers a converter (see
// DataFormats/src/Persistence.cc). Core registers scalars and vectors.
#pragma once
#include "Core/interface/Event.h"
#include "Core/interface/FlatTable.h"
#include <functional>
#include <map>
#include <string>
#include <typeindex>

namespace hell {

class PersistenceRegistry {
 public:
  using Converter = std::function<FlatTable(const ProductHolderBase&, const std::string& tableName)>;
  static PersistenceRegistry& instance();

  template <class T>
  void add(std::function<FlatTable(const T&, const std::string&)> fn) {
    converters_[std::type_index(typeid(T))] = [fn](const ProductHolderBase& h, const std::string& n) {
      return fn(static_cast<const ProductHolder<T>&>(h).value, n);
    };
  }
  bool canConvert(std::type_index t) const { return converters_.count(t) > 0; }
  FlatTable convert(const ProductHolderBase& holder, const std::string& tableName) const;
  std::vector<std::string> convertibleTypes() const;

 private:
  PersistenceRegistry();
  std::map<std::type_index, Converter> converters_;
};

}  // namespace hell
