"""Merge HELL output files. hadd-compatible by construction:
histograms add, trees (Events, Runs, LuminosityBlocks, hell/provenance) concatenate.
`hell merge` uses ROOT's TFileMerger and then checks that the merged
bookkeeping equals the sum of the inputs."""
import os

from hell import HellError, log, warn


def read_counts(path):
    from hell.rootutils import quiet_root
    ROOT = quiet_root()
    f = ROOT.TFile.Open(path)
    if not f or f.IsZombie():
        raise HellError(f"cannot open {path}")
    h = f.Get("hell/counts")
    out = {}
    if h:
        for i in range(1, h.GetNbinsX() + 1):
            lab = h.GetXaxis().GetBinLabel(i)
            if lab:
                out[lab] = h.GetBinContent(i)
    f.Close()
    return out


def merge(inputs, output, force=False):
    from hell.rootutils import quiet_root
    ROOT = quiet_root()
    if os.path.exists(output) and not force:
        raise HellError(f"{output} exists (use --force)")
    os.makedirs(os.path.dirname(os.path.abspath(output)), exist_ok=True)
    expected = {}
    good = []
    for p in inputs:
        try:
            c = read_counts(p)
        except HellError as e:
            warn(str(e))
            continue
        good.append(p)
        for k, v in c.items():
            expected[k] = expected.get(k, 0.0) + v
    if not good:
        raise HellError("no readable input files")
    m = ROOT.TFileMerger(False)
    m.SetFastMethod(True)
    m.OutputFile(output, "RECREATE")
    for p in good:
        m.AddFile(p)
    if not m.Merge():
        raise HellError("TFileMerger failed")
    got = read_counts(output)
    for k, v in expected.items():
        if abs(got.get(k, 0.0) - v) > 1e-6 * max(1.0, abs(v)):
            warn(f"bookkeeping mismatch after merge for {k}: {got.get(k)} vs expected {v}")
    log(f"merged {len(good)} files -> {output} (events_in={got.get('events_in', 0):.0f}, genEventSumw={got.get('genEventSumw', 0):.6g})")
    return output
