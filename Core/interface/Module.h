// Module base classes. Three kinds, as in CMSSW:
//   Producer : produce(Event&)        puts products into the event
//   Filter   : filter(Event&) -> bool false stops the path for this event
//   Analyzer : analyze(const Event&)  fills histograms / trees, no products
// Every module is constructed as Module(const ParameterSet&, ModuleContext&).
// Through the context it declares the branches it reads (consumes...), the
// products it makes (produces<T>) and the products it reads (consumesProduct).
#pragma once
#include "Core/interface/BranchRegistry.h"
#include "Core/interface/Event.h"
#include "Core/interface/HistService.h"
#include "Core/interface/JobContext.h"
#include "Core/interface/ParameterSet.h"
#include "Core/interface/ProductRegistry.h"
#include <TChain.h>
#include <TTree.h>
#include <functional>
#include <string>
#include <vector>

namespace hell {

enum class ModuleKind { Producer, Filter, Analyzer };
std::string moduleKindName(ModuleKind k);

class ModuleContext {
 public:
  ModuleContext(const std::string& label, const std::string& type, BranchRegistry& branches, ProductRegistry& products,
                HistService& hists, const JobContext& job)
      : label_(label), type_(type), branches_(branches), products_(products), hists_(hists), job_(job) {}

  const std::string& label() const { return label_; }
  const std::string& type() const { return type_; }
  const JobContext& job() const { return job_; }

  template <class T>
  ArrayHandle<T> consumesArray(const std::string& branch, bool optional = false) {
    consumedBranches_.push_back(branch);
    return branches_.array<T>(branch, optional);
  }
  template <class T>
  ValueHandle<T> consumesValue(const std::string& branch, bool optional = false) {
    consumedBranches_.push_back(branch);
    return branches_.value<T>(branch, optional);
  }
  // Declare a product; returns its full label ("label" or "label:instance").
  template <class T>
  std::string produces(const std::string& instance = "") {
    const std::string full = instance.empty() ? label_ : label_ + ":" + instance;
    products_.add(ProductInfo{full, label_, instance, std::type_index(typeid(T)), demangle(typeid(T).name())});
    produced_.push_back(full);
    return full;
  }
  // Declare a product read from the event (dependency validation + documentation).
  std::string consumesProduct(const std::string& label) {
    consumedProducts_.push_back(label);
    return label;
  }
  HistDir hists() { if (!histDir_.valid()) histDir_ = hists_.moduleDir(label_); return histDir_; }
  // Input chain and tree-change notifications (for modules that write trees).
  TChain* inputChain() const { return chain_; }
  void onTreeChange(std::function<void(TTree*)> f) { if (treeListeners_) treeListeners_->push_back(std::move(f)); }
  void setInput(TChain* chain, std::vector<std::function<void(TTree*)>>* listeners) { chain_ = chain; treeListeners_ = listeners; }
  HistService& histService() { return hists_; }
  BranchRegistry& branches() { return branches_; }
  const ProductRegistry& productRegistry() const { return products_; }

  const std::vector<std::string>& consumedBranches() const { return consumedBranches_; }
  const std::vector<std::string>& consumedProducts() const { return consumedProducts_; }
  const std::vector<std::string>& producedProducts() const { return produced_; }

 private:
  std::string label_, type_;
  BranchRegistry& branches_;
  ProductRegistry& products_;
  HistService& hists_;
  const JobContext& job_;
  HistDir histDir_;
  TChain* chain_ = nullptr;
  std::vector<std::function<void(TTree*)>>* treeListeners_ = nullptr;
  std::vector<std::string> consumedBranches_, consumedProducts_, produced_;
};

class ModuleBase {
 public:
  explicit ModuleBase(ModuleContext& ctx) : label_(ctx.label()), type_(ctx.type()) {}
  virtual ~ModuleBase() = default;
  virtual ModuleKind kind() const = 0;
  virtual void beginJob() {}
  virtual void beginFile(const std::string& path) {}
  virtual void endFile(const std::string& path) {}
  virtual void endJob() {}
  const std::string& label() const { return label_; }
  const std::string& type() const { return type_; }

 private:
  std::string label_, type_;
};

class Producer : public ModuleBase {
 public:
  using ModuleBase::ModuleBase;
  ModuleKind kind() const override { return ModuleKind::Producer; }
  virtual void produce(Event& event) = 0;
};

class Filter : public ModuleBase {
 public:
  using ModuleBase::ModuleBase;
  ModuleKind kind() const override { return ModuleKind::Filter; }
  virtual bool filter(Event& event) = 0;
};

class Analyzer : public ModuleBase {
 public:
  using ModuleBase::ModuleBase;
  ModuleKind kind() const override { return ModuleKind::Analyzer; }
  virtual void analyze(const Event& event) = 0;
};

}  // namespace hell
