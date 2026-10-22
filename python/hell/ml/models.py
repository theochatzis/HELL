"""Models. Kept simple on purpose: an MLP covers classification and regression
and can be exported to JSON for the C++ MLPScoreProducer. Add other
architectures here and give them the same `layers_for_export` contract."""


def _torch():
    try:
        import torch
    except ImportError:
        from hell import HellError
        raise HellError("PyTorch not available: use the LCG environment (source env/setup.sh with HELL_ENV=lcg)")
    return torch


def make_mlp(n_inputs, n_outputs, hidden=(64, 64), activation="relu", dropout=0.0, output_activation=None):
    torch = _torch()
    nn = torch.nn
    acts = {"relu": nn.ReLU, "tanh": nn.Tanh, "sigmoid": nn.Sigmoid, "leaky_relu": nn.LeakyReLU, "softplus": nn.Softplus}
    layers = []
    n_in = n_inputs
    for h in hidden:
        layers.append(nn.Linear(n_in, h))
        layers.append(acts[activation]())
        if dropout > 0:
            layers.append(nn.Dropout(dropout))
        n_in = h
    layers.append(nn.Linear(n_in, n_outputs))
    if output_activation:
        layers.append(acts[output_activation]())
    return nn.Sequential(*layers)


def layers_for_export(model):
    """List of {weight, bias, activation} for the JSON export (Sequential of Linear/activation)."""
    torch = _torch()
    nn = torch.nn
    names = {nn.ReLU: "relu", nn.Tanh: "tanh", nn.Sigmoid: "sigmoid", nn.LeakyReLU: "leaky_relu", nn.Softplus: "softplus"}
    out = []
    mods = list(model)
    for i, m in enumerate(mods):
        if isinstance(m, nn.Linear):
            act = "linear"
            for nxt in mods[i + 1:]:
                if isinstance(nxt, nn.Dropout):
                    continue
                if type(nxt) in names:
                    act = names[type(nxt)]
                break
            out.append({"weight": m.weight.detach().cpu().numpy().tolist(), "bias": m.bias.detach().cpu().numpy().tolist(), "activation": act})
    return out
