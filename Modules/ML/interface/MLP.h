// Dependency-free inference of a multilayer perceptron exported by
// python/hell/ml/export.py as JSON:
// {"inputs": ["topMass", ...], "scaler": {"mean": [...], "scale": [...]},
//  "layers": [{"weight": [[...]], "bias": [...], "activation": "relu"}, ...],
//  "outputs": ["score"], "task": "classifier"|"regressor"}
// Lets a trained PyTorch model run inside the C++ event loop without
// linking libtorch. (Larger architectures can be exported to ONNX later.)
#pragma once
#include <string>
#include <vector>

namespace hell {

class MLP {
 public:
  void load(const std::string& jsonPath);
  std::vector<double> evaluate(const std::vector<double>& inputs) const;
  const std::vector<std::string>& inputs() const { return inputs_; }
  const std::vector<std::string>& outputs() const { return outputs_; }
  const std::string& task() const { return task_; }
  std::string summary() const;

 private:
  struct Layer { std::vector<std::vector<double>> weight; std::vector<double> bias; std::string activation; };
  static double activate(double x, const std::string& a);
  std::vector<std::string> inputs_, outputs_;
  std::vector<double> mean_, scale_;
  std::vector<Layer> layers_;
  std::string task_;
};

}  // namespace hell
