// NanoAOD-style flat table: N rows x columns of float/int/bool, each with a
// doc string. Products are converted to FlatTables to be written to trees
// as n<Table>, <Table>_<column>[n<Table>]. Singleton tables (one row)
// are written as plain scalar branches <Table>_<column>.
#pragma once
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace hell {

class FlatTable {
 public:
  enum class ColumnType { Float, Int, Bool };
  struct Column {
    std::string name;
    ColumnType type;
    std::string doc;
    std::vector<float> f;
    std::vector<int> i;
    std::vector<bool> b;
    size_t size() const { return type == ColumnType::Float ? f.size() : type == ColumnType::Int ? i.size() : b.size(); }
  };

  FlatTable() = default;
  FlatTable(const std::string& name, size_t rows, bool singleton = false) : name_(name), rows_(rows), singleton_(singleton) {}

  const std::string& name() const { return name_; }
  size_t rows() const { return rows_; }
  bool singleton() const { return singleton_; }
  const std::vector<Column>& columns() const { return columns_; }

  void addColumn(const std::string& name, std::vector<float> v, const std::string& doc = "");
  void addColumn(const std::string& name, std::vector<int> v, const std::string& doc = "");
  void addColumn(const std::string& name, std::vector<bool> v, const std::string& doc = "");
  void addColumn(const std::string& name, std::vector<double> v, const std::string& doc = "");

 private:
  void check(const std::string& name, size_t n) const;
  std::string name_;
  size_t rows_ = 0;
  bool singleton_ = false;
  std::vector<Column> columns_;
};

}  // namespace hell
