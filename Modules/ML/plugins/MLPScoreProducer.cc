// Applies an exported MLP (JSON from `hell train`) to columns of a
// one-row FlatTable product and writes the outputs as a one-row table.
// The input names of the model are looked up as columns of the table.
//
// python:
//   process.mlScore = Module("MLPScoreProducer", model="models/st_classifier.json", table="stVars")
#include "Core/interface/FlatTable.h"
#include "Core/interface/Logger.h"
#include "Core/interface/ModuleFactory.h"
#include "Modules/ML/interface/MLP.h"

namespace hell {

class MLPScoreProducer : public Producer {
 public:
  MLPScoreProducer(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx) {
    mlp_.load(ps.get<std::string>("model"));
    table_ = ctx.consumesProduct(ps.get<std::string>("table"));
    prefix_ = ps.get("prefix", "");
    HELL_INFO(label()) << mlp_.summary();
    ctx.produces<FlatTable>();
  }
  void produce(Event& ev) override {
    const auto& t = ev.get<FlatTable>(table_);
    std::vector<double> x;
    x.reserve(mlp_.inputs().size());
    const std::string prefix = t.name() + "_";
    for (const auto& name : mlp_.inputs()) {
      bool found = false;
      // Model inputs are tree branch names (<table>_<column>) or bare column names.
      const std::string col = (name.compare(0, prefix.size(), prefix) == 0) ? name.substr(prefix.size()) : name;
      for (const auto& c : t.columns()) {
        if (c.name != col) continue;
        found = true;
        x.push_back(c.type == FlatTable::ColumnType::Float ? c.f.at(0) : c.type == FlatTable::ColumnType::Int ? c.i.at(0) : (c.b.at(0) ? 1.0 : 0.0));
      }
      if (!found) throw std::runtime_error("MLPScoreProducer: input '" + name + "' is not a column of table " + t.name());
    }
    const auto y = mlp_.evaluate(x);
    FlatTable out(label(), 1, true);
    for (size_t i = 0; i < y.size(); ++i) out.addColumn(prefix_ + mlp_.outputs()[i], std::vector<float>{static_cast<float>(y[i])}, "MLP output " + mlp_.outputs()[i]);
    ev.put(label(), std::move(out));
  }
 private:
  MLP mlp_;
  std::string table_, prefix_;
};

}  // namespace hell

HELL_REGISTER_MODULE(hell::MLPScoreProducer, "Evaluates an exported MLP (JSON) on columns of a one-row table");
