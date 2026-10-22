// The HELL event: a type-erased store of products exchanged between modules,
// in the spirit of edm::Event. Products are owned by the event and cleared
// at the start of every entry. Access is by label; the label of a product is
// "<moduleLabel>" or "<moduleLabel>:<instance>" when a module produces several.
#pragma once
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

namespace hell {

struct EventID {
  unsigned int run = 0;
  unsigned int lumi = 0;
  unsigned long long event = 0;
};

std::string demangle(const char* name);

class ProductHolderBase {
 public:
  virtual ~ProductHolderBase() = default;
  virtual std::type_index type() const = 0;
};

template <class T>
class ProductHolder : public ProductHolderBase {
 public:
  explicit ProductHolder(T&& v) : value(std::move(v)) {}
  explicit ProductHolder(const T& v) : value(v) {}
  std::type_index type() const override { return std::type_index(typeid(T)); }
  T value;
};

class Event {
 public:
  template <class T>
  void put(const std::string& label, T&& obj) {
    using U = typename std::decay<T>::type;
    if (products_.count(label)) throw std::runtime_error("product '" + label + "' was already put into the event");
    products_[label] = std::make_unique<ProductHolder<U>>(std::forward<T>(obj));
  }

  template <class T>
  const T& get(const std::string& label) const {
    const std::string resolved = resolve(label);
    auto it = products_.find(resolved);
    if (it == products_.end()) throw std::runtime_error("product '" + label + "' not found in the event" + hint(label));
    auto* holder = dynamic_cast<ProductHolder<T>*>(it->second.get());
    if (!holder)
      throw std::runtime_error("product '" + label + "' has type " + demangle(it->second->type().name()) +
                               ", requested " + demangle(typeid(T).name()));
    return holder->value;
  }

  template <class T>
  T& getMutable(const std::string& label) {
    return const_cast<T&>(get<T>(label));
  }

  template <class T>
  const T* getIfExists(const std::string& label) const {
    const std::string resolved = resolve(label);
    auto it = products_.find(resolved);
    if (it == products_.end()) return nullptr;
    auto* holder = dynamic_cast<ProductHolder<T>*>(it->second.get());
    return holder ? &holder->value : nullptr;
  }

  bool has(const std::string& label) const { return products_.count(resolve(label)) > 0; }
  std::vector<std::string> labels() const;
  void clear() { products_.clear(); }

  EventID id;
  long long entry = -1;      // entry in the input chain
  bool isData = false;
  bool isMC() const { return !isData; }
  // Nominal event weight accumulated by weight producers (1 for data).
  double weight = 1.0;

 private:
  // "label" resolves to "label:instance" when that module has exactly one product.
  std::string resolve(const std::string& label) const;
  std::string hint(const std::string& label) const;
  std::map<std::string, std::unique_ptr<ProductHolderBase>> products_;
};

}  // namespace hell
