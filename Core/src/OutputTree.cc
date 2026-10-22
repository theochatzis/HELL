#include "Core/interface/OutputTree.h"
#include "Core/interface/BranchRegistry.h"
#include "Core/interface/Logger.h"
#include "Core/interface/Utils.h"
#include <TObjArray.h>
#include <TLeaf.h>
#include <algorithm>

namespace hell {

OutputTree::OutputTree(TDirectory* dir, const std::string& treeName, TChain& input, BranchRegistry& registry,
                       const std::vector<std::string>& keep, const std::vector<std::string>& drop)
    : dir_(dir), input_(input), registry_(registry), keep_(keep), drop_(drop) {
  TDirectory* old = gDirectory;
  dir_->cd();
  tree_ = new TTree(treeName.c_str(), treeName.c_str());
  tree_->SetDirectory(dir_);
  tree_->SetAutoSave(0);
  if (old) old->cd();
}

OutputTree::~OutputTree() = default;

void OutputTree::onTreeChange(TTree* tree) {
  if (!tree) return;
  // Select the branches to copy once (from the first input tree); later trees re-bind addresses.
  if (!branchesSelected_) {
    branchesSelected_ = true;
    TObjArray* branches = tree->GetListOfBranches();
    for (int i = 0; i < branches->GetEntries(); ++i) {
      const std::string name = branches->At(i)->GetName();
      bool keep = false;
      for (const auto& p : keep_) if (globMatch(p, name)) { keep = true; break; }
      for (const auto& p : drop_) if (globMatch(p, name)) { keep = false; break; }
      if (!keep) continue;
      // Counter branches of kept arrays must be kept too.
      copied_.push_back(name);
    }
    // Add counters of kept array branches.
    std::vector<std::string> extra;
    for (const auto& name : copied_) {
      TBranch* b = tree->GetBranch(name.c_str());
      TLeaf* l = b ? static_cast<TLeaf*>(b->GetListOfLeaves()->At(0)) : nullptr;
      if (l && l->GetLeafCount()) {
        const std::string c = l->GetLeafCount()->GetBranch()->GetName();
        if (std::find(copied_.begin(), copied_.end(), c) == copied_.end() && std::find(extra.begin(), extra.end(), c) == extra.end()) extra.push_back(c);
      }
    }
    copied_.insert(copied_.end(), extra.begin(), extra.end());
    // Register every copied branch in the registry (so it is read lazily with the right buffer),
    // then create the output branches sharing those buffers.
    for (const auto& name : copied_) {
      TBranch* b = tree->GetBranch(name.c_str());
      TLeaf* l = static_cast<TLeaf*>(b->GetListOfLeaves()->At(0));
      const bool isArray = l->GetLeafCount() != nullptr;
      auto slot = isArray ? registry_.array<float>(name).valid() : registry_.value<float>(name).valid();
      (void)slot;
    }
    // Counter (scalar) branches must exist in the output before the arrays that use them.
    std::stable_sort(copied_.begin(), copied_.end(), [&](const std::string& a, const std::string& b) {
      auto isArr = [&](const std::string& n) { TLeaf* l = static_cast<TLeaf*>(tree->GetBranch(n.c_str())->GetListOfLeaves()->At(0)); return l->GetLeafCount() != nullptr; };
      return !isArr(a) && isArr(b);
    });
    for (const auto& name : copied_) {
      TBranch* b = tree->GetBranch(name.c_str());
      TLeaf* l = static_cast<TLeaf*>(b->GetListOfLeaves()->At(0));
      std::string leaflist = name;
      if (l->GetLeafCount()) leaflist += std::string("[") + l->GetLeafCount()->GetBranch()->GetName() + "]";
      const std::string tn = l->GetTypeName();
      char code = 'F';
      if (tn == "Float_t") code = 'F'; else if (tn == "Double_t") code = 'D'; else if (tn == "Int_t") code = 'I';
      else if (tn == "UInt_t") code = 'i'; else if (tn == "Short_t") code = 'S'; else if (tn == "UShort_t") code = 's';
      else if (tn == "Char_t") code = 'B'; else if (tn == "UChar_t") code = 'b'; else if (tn == "Bool_t") code = 'O';
      else if (tn == "Long64_t") code = 'L'; else if (tn == "ULong64_t") code = 'l';
      leaflist += "/"; leaflist += code;
      auto& slot = registry_.slots().at(name);
      TBranch* ob = tree_->Branch(name.c_str(), slot->data(), leaflist.c_str());
      if (!ob) throw std::runtime_error("cannot create output branch " + name + " (" + leaflist + ")");
      ob->SetTitle(b->GetTitle());
    }
    HELL_INFO("output") << "copying " << copied_.size() << " input branches to tree " << tree_->GetName();
  }
  // Re-point output branches to the (possibly re-allocated) buffers.
  for (const auto& name : copied_) {
    auto it = registry_.slots().find(name);
    if (it == registry_.slots().end()) continue;
    if (TBranch* ob = tree_->GetBranch(name.c_str())) ob->SetAddress(it->second->data());
  }
}

void OutputTree::declareTable(const FlatTable& table) {
  TableBranches& tb = tables_[table.name()];
  tb.singleton = table.singleton();
  if (!tb.singleton) tree_->Branch(("n" + table.name()).c_str(), &tb.n, ("n" + table.name() + "/I").c_str());
  for (const auto& c : table.columns()) {
    const std::string bname = table.name() + "_" + c.name;
    const std::string dim = tb.singleton ? "" : "[n" + table.name() + "]";
    TBranch* b = nullptr;
    if (c.type == FlatTable::ColumnType::Float) {
      auto& v = tb.f[c.name]; v.assign(std::max<size_t>(table.rows(), 1) * 4 + 16, 0.f);
      b = tree_->Branch(bname.c_str(), v.data(), (bname + dim + "/F").c_str());
    } else if (c.type == FlatTable::ColumnType::Int) {
      auto& v = tb.i[c.name]; v.assign(std::max<size_t>(table.rows(), 1) * 4 + 16, 0);
      b = tree_->Branch(bname.c_str(), v.data(), (bname + dim + "/I").c_str());
    } else {
      auto& v = tb.b[c.name]; v.assign(std::max<size_t>(table.rows(), 1) * 4 + 16, 0);
      b = tree_->Branch(bname.c_str(), v.data(), (bname + dim + "/O").c_str());
    }
    if (b && !c.doc.empty()) b->SetTitle(c.doc.c_str());
  }
}

void OutputTree::setTable(const FlatTable& table) {
  if (!tables_.count(table.name())) declareTable(table);
  TableBranches& tb = tables_[table.name()];
  tb.n = static_cast<int>(table.rows());
  for (const auto& c : table.columns()) {
    const std::string bname = table.name() + "_" + c.name;
    auto grow = [&](auto& vec) {
      if (vec.size() < table.rows()) { vec.resize(table.rows() * 2); tree_->GetBranch(bname.c_str())->SetAddress(vec.data()); }
    };
    if (c.type == FlatTable::ColumnType::Float) { auto& v = tb.f.at(c.name); grow(v); std::copy(c.f.begin(), c.f.end(), v.begin()); }
    else if (c.type == FlatTable::ColumnType::Int) { auto& v = tb.i.at(c.name); grow(v); std::copy(c.i.begin(), c.i.end(), v.begin()); }
    else { auto& v = tb.b.at(c.name); grow(v); for (size_t k = 0; k < c.b.size(); ++k) v[k] = c.b[k] ? 1 : 0; }
  }
}

void OutputTree::setScalar(const std::string& name, double value, const std::string& doc) {
  auto it = scalarsD_.find(name);
  if (it == scalarsD_.end()) {
    it = scalarsD_.emplace(name, value).first;
    TBranch* b = tree_->Branch(name.c_str(), &it->second, (name + "/D").c_str());
    if (!doc.empty()) b->SetTitle(doc.c_str());
  }
  it->second = value;
}
void OutputTree::setScalar(const std::string& name, int value, const std::string& doc) {
  auto it = scalarsI_.find(name);
  if (it == scalarsI_.end()) {
    it = scalarsI_.emplace(name, value).first;
    TBranch* b = tree_->Branch(name.c_str(), &it->second, (name + "/I").c_str());
    if (!doc.empty()) b->SetTitle(doc.c_str());
  }
  it->second = value;
}

void OutputTree::fill(long long /*chainEntry*/) {
  // Make sure every copied branch is loaded for this entry (lazy readers).
  registry_.loadAll();
  tree_->Fill();
  ++entries_;
}

}  // namespace hell
