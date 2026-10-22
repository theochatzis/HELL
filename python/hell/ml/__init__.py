"""MLTools: PyTorch classifiers and regressors on HELL analysis trees.

Pipeline: data.py (trees -> arrays with preselection and variables) ->
models.py (MLP) -> train.py -> validate.py (ROC, overtraining, residuals)
-> export.py (JSON for the C++ MLPScoreProducer, TorchScript/ONNX).
PyTorch is loaded lazily so that the rest of HELL works without it.
"""
