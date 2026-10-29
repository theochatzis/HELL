"""Export trained models for the C++ event loop (MLPScoreProducer) and for
other tools (TorchScript). The JSON holds the scaler, so the C++ side
reproduces the python scores exactly."""
import json

from hell import generated_by, log
from hell.ml.models import layers_for_export, _torch


def to_json(model, scaler, feature_names, class_names, task, path):
    layers = layers_for_export(model)
    outputs = ["value"] if task == "regressor" else [f"score_{c}" for c in class_names]
    doc = {"generatedBy": generated_by(), "task": task, "inputs": list(feature_names), "outputs": outputs,
           "scaler": {"mean": scaler.mean.tolist(), "scale": scaler.scale.tolist()},
           "layers": layers, "note": "classifier: the C++ MLP applies softmax over the outputs (sigmoid if one output)"}
    with open(path, "w") as f:
        json.dump(doc, f)
    log(f"exported {task} MLP with {len(layers)} layers to {path}")
    return path


def to_torchscript(model, n_inputs, path):
    torch = _torch()
    model.eval()
    example = torch.zeros(1, n_inputs)
    ts = torch.jit.trace(model.cpu(), example)
    ts.save(path)
    log(f"TorchScript model saved to {path}")
    return path
