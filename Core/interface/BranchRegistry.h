// Access to the branches of the input tree (NanoAOD-like flat trees).
//
// Design:
//  * Modules declare the branches they read in their constructor through
//    ModuleContext::consumesArray<T>/consumesValue<T>. Nothing is read that
//    was not declared, and only declared branches are enabled on the chain.
//  * Reading is lazy: a branch is read from disk the first time a module
//    touches it in a given entry. Filters that reject early therefore avoid
//    reading the heavy collections.
//  * The on-disk type is discovered at run time and converted to the
//    requested type. NanoAOD changes types between versions (Jet_jetId was
//    Int_t and is now UChar_t; GenPart_genPartIdxMother is now Short_t). A
//    handle requested as int or float works for all of them.
//  * Buffers are allocated per branch with the capacity of the current
//    tree's counter leaf and grow if a later file needs more.
#pragma once
#include <TBranch.h>
#include <TLeaf.h>
#include <TTree.h>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace hell {

enum class LeafType { Float, Double, Int, UInt, Short, UShort, Char, UChar, Bool, Long64, ULong64, Unknown };

LeafType leafTypeFromName(const std::string& rootTypeName);
std::string leafTypeName(LeafType t);
size_t leafTypeSize(LeafType t);

// One registered branch. Shared between all handles that use it.
class BranchSlot {
 public:
  BranchSlot(const std::string& name, bool optional, bool array) : name_(name), optional_(optional), array_(array) {}

  const std::string& name() const { return name_; }
  bool optional() const { return optional_; }
  bool isArray() const { return array_; }
  bool present() const { return branch_ != nullptr; }
  LeafType type() const { return type_; }
  void* data() { return buffer_.data(); }
  const void* data() const { return buffer_.data(); }
  size_t capacity() const { return capacity_; }

  // Bind to the current tree of the chain (called at every tree change).
  void bind(TTree* tree, long long treeNumber);
  // Make sure the branch content for `entry` (local entry in the current tree) is loaded.
  void load(long long localEntry) {
    if (loadedEntry_ == localEntry) return;
    loadedEntry_ = localEntry;
    if (!branch_) { size_ = 0; return; }
    branch_->GetEntry(localEntry);
    size_ = array_ ? static_cast<size_t>(leaf_->GetLen()) : 1;
    if (size_ > capacity_) throw std::runtime_error("branch " + name_ + " exceeded its buffer capacity");
  }
  size_t size() const { return size_; }
  void invalidate() { loadedEntry_ = -1; }

  // Typed element access with conversion from the on-disk type.
  template <class T> T at(size_t i) const;
  double asDouble(size_t i) const;
  long long asLong(size_t i) const;

 private:
  std::string name_;
  bool optional_;
  bool array_;
  TBranch* branch_ = nullptr;
  TLeaf* leaf_ = nullptr;
  LeafType type_ = LeafType::Unknown;
  std::vector<char> buffer_;
  size_t capacity_ = 0;
  size_t size_ = 0;
  long long loadedEntry_ = -1;
};

template <class T> T BranchSlot::at(size_t i) const {
  switch (type_) {
    case LeafType::Float:   return static_cast<T>(reinterpret_cast<const float*>(buffer_.data())[i]);
    case LeafType::Double:  return static_cast<T>(reinterpret_cast<const double*>(buffer_.data())[i]);
    case LeafType::Int:     return static_cast<T>(reinterpret_cast<const int32_t*>(buffer_.data())[i]);
    case LeafType::UInt:    return static_cast<T>(reinterpret_cast<const uint32_t*>(buffer_.data())[i]);
    case LeafType::Short:   return static_cast<T>(reinterpret_cast<const int16_t*>(buffer_.data())[i]);
    case LeafType::UShort:  return static_cast<T>(reinterpret_cast<const uint16_t*>(buffer_.data())[i]);
    case LeafType::Char:    return static_cast<T>(reinterpret_cast<const int8_t*>(buffer_.data())[i]);
    case LeafType::UChar:   return static_cast<T>(reinterpret_cast<const uint8_t*>(buffer_.data())[i]);
    case LeafType::Bool:    return static_cast<T>(reinterpret_cast<const bool*>(buffer_.data())[i]);
    case LeafType::Long64:  return static_cast<T>(reinterpret_cast<const int64_t*>(buffer_.data())[i]);
    case LeafType::ULong64: return static_cast<T>(reinterpret_cast<const uint64_t*>(buffer_.data())[i]);
    default: throw std::runtime_error("branch " + name_ + " has an unsupported type");
  }
}

class BranchRegistry;

// Handle to an array branch (Jet_pt[nJet]). Cheap to copy. Converts on access.
template <class T>
class ArrayHandle {
 public:
  ArrayHandle() = default;
  ArrayHandle(std::shared_ptr<BranchSlot> slot, const long long* localEntry) : slot_(std::move(slot)), localEntry_(localEntry) {}
  bool valid() const { return slot_ && slot_->present(); }
  size_t size() const { if (!valid()) return 0; slot_->load(*localEntry_); return slot_->size(); }
  T operator[](size_t i) const { slot_->load(*localEntry_); return slot_->template at<T>(i); }
  T at(size_t i) const {
    if (i >= size()) throw std::out_of_range("index " + std::to_string(i) + " out of range for branch " + slot_->name());
    return (*this)[i];
  }
  std::vector<T> toVector() const {
    std::vector<T> v; const size_t n = size(); v.reserve(n);
    for (size_t i = 0; i < n; ++i) v.push_back((*this)[i]);
    return v;
  }
  const std::string& name() const { static const std::string none; return slot_ ? slot_->name() : none; }

 private:
  std::shared_ptr<BranchSlot> slot_;
  const long long* localEntry_ = nullptr;
};

// Handle to a scalar branch (nJet, genWeight, HLT_IsoMu24 ...).
template <class T>
class ValueHandle {
 public:
  ValueHandle() = default;
  ValueHandle(std::shared_ptr<BranchSlot> slot, const long long* localEntry) : slot_(std::move(slot)), localEntry_(localEntry) {}
  bool valid() const { return slot_ && slot_->present(); }
  T operator*() const { slot_->load(*localEntry_); return slot_->template at<T>(0); }
  T get() const { return **this; }
  T getOr(const T& def) const { return valid() ? **this : def; }
  const std::string& name() const { static const std::string none; return slot_ ? slot_->name() : none; }

 private:
  std::shared_ptr<BranchSlot> slot_;
  const long long* localEntry_ = nullptr;
};

class BranchRegistry {
 public:
  template <class T>
  ArrayHandle<T> array(const std::string& name, bool optional = false) {
    return ArrayHandle<T>(slot(name, optional, true), &localEntry_);
  }
  template <class T>
  ValueHandle<T> value(const std::string& name, bool optional = false) {
    return ValueHandle<T>(slot(name, optional, false), &localEntry_);
  }

  // Called by the input source.
  void bind(TTree* tree, long long treeNumber);
  void setEntry(long long localEntry) {
    localEntry_ = localEntry;
    for (auto& kv : slots_) kv.second->invalidate();
  }
  // Load every registered branch (used before writing a skim so that all buffers are current).
  void loadAll();
  const std::map<std::string, std::shared_ptr<BranchSlot>>& slots() const { return slots_; }
  std::vector<std::string> names() const;
  size_t size() const { return slots_.size(); }
  // Listeners notified after each bind (e.g. output trees that share buffers).
  void addBindListener(std::function<void()> f) { listeners_.push_back(std::move(f)); }

 private:
  std::shared_ptr<BranchSlot> slot(const std::string& name, bool optional, bool array);
  std::map<std::string, std::shared_ptr<BranchSlot>> slots_;
  std::vector<std::function<void()>> listeners_;
  long long localEntry_ = -1;
  TTree* tree_ = nullptr;
  long long treeNumber_ = -1;
};

}  // namespace hell
