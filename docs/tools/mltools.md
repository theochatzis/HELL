# MLTools

PyTorch classifiers and regressors on the analysis trees, with validation, and application inside the C++
event loop.

```
hell train configs/SingleTopMass/ml_classifier.yaml            # train + validate + export
hell run configs/SingleTopMass/analysis_cfg.py ... model=models/st_classifier.json   # apply: ml_score_* branches
```
YAML: `task` (classifier | regressor), `input` trees, `preselection`, `features`, `spectators` (variables the
score must not sculpt, e.g. the top mass), `weight`, `classes` (classifier: class -> samples) or `samples` +
`target` (regressor), `model` (hidden layers, activation, dropout), `training` (epochs, batch size, lr, seed).

Pipeline (`python/hell/ml/`): `data.py` reads the trees with ROOT expressions and builds the arrays (class
weights equalised, |w| used), `models.py` builds the MLP, `train.py` trains (cross-entropy or Huber loss,
best epoch on the test loss), `validate.py` writes ROC/AUC, score distributions train vs test with a KS
statistic (overtraining), mean score vs spectators (sculpting), residuals and response for regressors, and a
JSON report; `export.py` writes the model as JSON (weights + input scaler) and TorchScript.

The C++ `MLPScoreProducer` evaluates the JSON model (`Modules/ML/interface/MLP.h`, no libtorch dependency) on
the columns of a one-row table (`stVars`) and writes the outputs as a table; the scores are then ordinary
branches for the histogrammer (`ml_score_tch`). The C++ and python scores agree to float precision (checked in
the quick test). Larger architectures can be exported to ONNX later and evaluated with ONNX Runtime or SOFIE.

Batch training: `hell submit train ml.yaml --output-dir ... --tag train_v1 [--gpu]`.
