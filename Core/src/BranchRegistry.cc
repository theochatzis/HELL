#include "Core/interface/BranchRegistry.h"
#include "Core/interface/Logger.h"
#include <TLeaf.h>
#include <TFile.h>
#include <algorithm>

namespace hell {

LeafType leafTypeFromName(const std::string& n) {
  if (n == "Float_t") return LeafType::Float;
  if (n == "Double_t") return LeafType::Double;
  if (n == "Int_t") return LeafType::Int;
  if (n == "UInt_t") return LeafType::UInt;
  if (n == "Short_t") return LeafType::Short;
  if (n == "UShort_t") return LeafType::UShort;
  if (n == "Char_t") return LeafType::Char;
  if (n == "UChar_t") return LeafType::UChar;
  if (n == "Bool_t") return LeafType::Bool;
  if (n == "Long64_t") return LeafType::Long64;
  if (n == "ULong64_t") return LeafType::ULong64;
  return LeafType::Unknown;
}
std::string leafTypeName(LeafType t) {
  switch (t) {
    case LeafType::Float: return "Float_t"; case LeafType::Double: return "Double_t"; case LeafType::Int: return "Int_t";
    case LeafType::UInt: return "UInt_t"; case LeafType::Short: return "Short_t"; case LeafType::UShort: return "UShort_t";
    case LeafType::Char: return "Char_t"; case LeafType::UChar: return "UChar_t"; case LeafType::Bool: return "Bool_t";
    case LeafType::Long64: return "Long64_t"; case LeafType::ULong64: return "ULong64_t"; default: return "unknown";
  }
}
size_t leafTypeSize(LeafType t) {
  switch (t) {
    case LeafType::Float: case LeafType::Int: case LeafType::UInt: return 4;
    case LeafType::Double: case LeafType::Long64: case LeafType::ULong64: return 8;
    case LeafType::Short: case LeafType::UShort: return 2;
    default: return 1;
  }
}

void BranchSlot::bind(TTree* tree, long long /*treeNumber*/) {
  loadedEntry_ = -1;
  size_ = 0;
  branch_ = tree ? tree->GetBranch(name_.c_str()) : nullptr;
  leaf_ = nullptr;
  if (!branch_) {
    if (!optional_) throw std::runtime_error("branch '" + name_ + "' not found in tree " + (tree ? tree->GetName() : "<null>") + " of " + (tree && tree->GetCurrentFile() ? tree->GetCurrentFile()->GetName() : "?"));
    return;
  }
  leaf_ = branch_->GetLeaf(name_.c_str());
  if (!leaf_) leaf_ = static_cast<TLeaf*>(branch_->GetListOfLeaves()->At(0));
  if (!leaf_) throw std::runtime_error("branch '" + name_ + "' has no leaf");
  const LeafType t = leafTypeFromName(leaf_->GetTypeName());
  if (t == LeafType::Unknown) throw std::runtime_error("branch '" + name_ + "' has unsupported type " + leaf_->GetTypeName());
  if (type_ != LeafType::Unknown && t != type_)
    HELL_DEBUG("branches") << "branch " << name_ << " changed type " << leafTypeName(type_) << " -> " << leafTypeName(t) << " (converted automatically)";
  type_ = t;
  const bool hasCounter = leaf_->GetLeafCount() != nullptr;
  if (array_ && !hasCounter && leaf_->GetLenStatic() <= 1)
    HELL_DEBUG("branches") << "branch " << name_ << " requested as array but is a scalar";
  if (!array_ && hasCounter) throw std::runtime_error("branch '" + name_ + "' is an array, request it with consumesArray");
  size_t need = 1;
  if (hasCounter) {
    const int mx = leaf_->GetLeafCount()->GetMaximum();
    need = std::max<size_t>(static_cast<size_t>(std::max(mx, 1)) * 2, 64);
  } else if (leaf_->GetLenStatic() > 1) {
    need = leaf_->GetLenStatic();
  }
  if (need > capacity_) {
    capacity_ = need;
    buffer_.assign(capacity_ * leafTypeSize(type_), 0);
  }
  tree->SetBranchStatus(name_.c_str(), 1);
  if (hasCounter) tree->SetBranchStatus(leaf_->GetLeafCount()->GetBranch()->GetName(), 1);
  branch_->SetAddress(buffer_.data());
}

double BranchSlot::asDouble(size_t i) const { return at<double>(i); }
long long BranchSlot::asLong(size_t i) const { return at<long long>(i); }

std::shared_ptr<BranchSlot> BranchRegistry::slot(const std::string& name, bool optional, bool array) {
  auto it = slots_.find(name);
  if (it != slots_.end()) {
    if (it->second->isArray() != array) throw std::runtime_error("branch '" + name + "' requested both as array and as scalar");
    if (tree_) it->second->bind(tree_, treeNumber_);
    return it->second;
  }
  auto s = std::make_shared<BranchSlot>(name, optional, array);
  if (tree_) s->bind(tree_, treeNumber_);
  slots_[name] = s;
  return s;
}

void BranchRegistry::bind(TTree* tree, long long treeNumber) {
  tree_ = tree;
  treeNumber_ = treeNumber;
  if (!tree) return;
  for (auto& kv : slots_) kv.second->bind(tree, treeNumber);
  for (auto& f : listeners_) f();
}

void BranchRegistry::loadAll() {
  for (auto& kv : slots_) kv.second->load(localEntry_);
}

std::vector<std::string> BranchRegistry::names() const {
  std::vector<std::string> v;
  for (const auto& kv : slots_) v.push_back(kv.first);
  return v;
}

}  // namespace hell
