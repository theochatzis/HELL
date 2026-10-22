#include "Modules/ML/interface/MLP.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace hell {

void MLP::load(const std::string& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("MLP: cannot open " + path);
  nlohmann::json j; in >> j;
  inputs_ = j.at("inputs").get<std::vector<std::string>>();
  outputs_ = j.value("outputs", std::vector<std::string>{"score"});
  task_ = j.value("task", "classifier");
  mean_ = j.contains("scaler") ? j["scaler"].value("mean", std::vector<double>(inputs_.size(), 0.0)) : std::vector<double>(inputs_.size(), 0.0);
  scale_ = j.contains("scaler") ? j["scaler"].value("scale", std::vector<double>(inputs_.size(), 1.0)) : std::vector<double>(inputs_.size(), 1.0);
  layers_.clear();
  for (const auto& l : j.at("layers")) {
    Layer L; L.weight = l.at("weight").get<std::vector<std::vector<double>>>(); L.bias = l.at("bias").get<std::vector<double>>();
    L.activation = l.value("activation", "linear");
    if (L.weight.size() != L.bias.size()) throw std::runtime_error("MLP: layer weight/bias size mismatch in " + path);
    layers_.push_back(L);
  }
  if (layers_.empty()) throw std::runtime_error("MLP: no layers in " + path);
  if (layers_.front().weight.front().size() != inputs_.size()) throw std::runtime_error("MLP: first layer does not match the number of inputs in " + path);
}

double MLP::activate(double x, const std::string& a) {
  if (a == "relu") return x > 0 ? x : 0;
  if (a == "sigmoid") return 1.0 / (1.0 + std::exp(-x));
  if (a == "tanh") return std::tanh(x);
  if (a == "leaky_relu") return x > 0 ? x : 0.01 * x;
  if (a == "softplus") return std::log1p(std::exp(x));
  return x;  // linear
}

std::vector<double> MLP::evaluate(const std::vector<double>& inputs) const {
  if (inputs.size() != inputs_.size()) throw std::runtime_error("MLP: expected " + std::to_string(inputs_.size()) + " inputs, got " + std::to_string(inputs.size()));
  std::vector<double> x(inputs.size());
  for (size_t i = 0; i < x.size(); ++i) x[i] = (inputs[i] - mean_[i]) / (scale_[i] != 0 ? scale_[i] : 1.0);
  for (const auto& L : layers_) {
    std::vector<double> y(L.bias.size());
    for (size_t o = 0; o < y.size(); ++o) {
      double s = L.bias[o];
      const auto& w = L.weight[o];
      for (size_t i = 0; i < x.size(); ++i) s += w[i] * x[i];
      y[o] = activate(s, L.activation);
    }
    x.swap(y);
  }
  if (task_ == "classifier" && x.size() > 1) {  // logits -> softmax probabilities
    double mx = x[0]; for (double v : x) mx = std::max(mx, v);
    double sum = 0; for (double& v : x) { v = std::exp(v - mx); sum += v; }
    for (double& v : x) v /= sum;
  } else if (task_ == "classifier" && x.size() == 1) {
    x[0] = 1.0 / (1.0 + std::exp(-x[0]));
  }
  return x;
}

std::string MLP::summary() const {
  std::ostringstream os;
  os << task_ << " MLP: " << inputs_.size() << " inputs ->";
  for (const auto& L : layers_) os << " " << L.bias.size() << "(" << L.activation << ")";
  os << " -> " << outputs_.size() << " outputs";
  return os.str();
}

}  // namespace hell
