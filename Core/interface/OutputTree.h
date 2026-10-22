// Writes an output event tree. Two kinds of content:
//  * input branches selected by keep/drop glob patterns (copied as they are),
//  * products converted to flat tables (n<Table>, <Table>_<col>).
// Also copies the auxiliary trees (Runs, LuminosityBlocks) from every input
// file so that bookkeeping survives the skim (see Bookkeeper).
#pragma once
#include "Core/interface/FlatTable.h"
#include <TChain.h>
#include <TDirectory.h>
#include <TTree.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace hell {

class BranchRegistry;

class OutputTree {
 public:
  OutputTree(TDirectory* dir, const std::string& treeName, TChain& input, BranchRegistry& registry,
             const std::vector<std::string>& keep, const std::vector<std::string>& drop);
  ~OutputTree();

  // Must be called when the input chain switches file (re-binds copied branches).
  void onTreeChange(TTree* tree);
  // Declare a table (first time) and set its content for the current event.
  void setTable(const FlatTable& table);
  // Set a scalar value (e.g. derived event-level variable).
  void setScalar(const std::string& name, double value, const std::string& doc = "");
  void setScalar(const std::string& name, int value, const std::string& doc = "");
  // Fill the current event (copies the enabled input branches too).
  void fill(long long chainEntry);
  TTree* tree() { return tree_; }
  size_t copiedBranches() const { return copied_.size(); }
  const std::vector<std::string>& copiedBranchNames() const { return copied_; }
  long long entries() const { return entries_; }

 private:
  struct TableBranches {
    bool singleton = false;
    int n = 0;
    std::map<std::string, std::vector<float>> f;
    std::map<std::string, std::vector<int>> i;
    std::map<std::string, std::vector<char>> b;  // bool as char for ROOT
    std::map<std::string, std::string> types;
  };
  void declareTable(const FlatTable& table);
  TDirectory* dir_;
  TTree* tree_ = nullptr;
  TChain& input_;
  BranchRegistry& registry_;
  std::vector<std::string> keep_, drop_;
  std::vector<std::string> copied_;
  std::map<std::string, TableBranches> tables_;
  std::map<std::string, double> scalarsD_;
  std::map<std::string, int> scalarsI_;
  long long entries_ = 0;
  bool branchesSelected_ = false;
};

}  // namespace hell
