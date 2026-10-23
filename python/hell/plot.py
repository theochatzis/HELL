"""Stacked data/MC plots from a hell-hist output file, in the CMS TDR style
(hell.tdr). Colours, labels, lumi and axis titles come from the histogram
configuration stored in the file (hell/hist_config)."""
import json
import os

from hell import HellError, log
from hell import tdr


def stack_with_ratio(name, data_h, mc_hists, labels, colors, xlabel, text, out_path, logy=False, region_label="",
                     ratio_label="Data / MC", ratio_range=(0.5, 1.5), info_text="", normalize_to_data=False):
    """Generic TDR data/MC stack with a ratio panel. mc_hists are stacked in the given order."""
    import ROOT
    T = tdr.load()
    keep = []
    scale = 1.0
    if normalize_to_data and data_h is not None:
        tot = sum(h.Integral() for h in mc_hists)
        scale = data_h.Integral() / tot if tot > 0 else 1.0
    total = None
    stack = ROOT.THStack(name + "_stack", "")
    for h, col in zip(mc_hists, colors):
        hh = h.Clone(name + "_" + h.GetName()); hh.SetDirectory(0); hh.Scale(scale); keep.append(hh)
        hh.SetFillColor(tdr.color(col)); hh.SetLineColor(ROOT.kBlack); hh.SetLineWidth(1)
        stack.Add(hh)
        if total is None:
            total = hh.Clone(name + "_total"); total.SetDirectory(0)
        else:
            total.Add(hh)
    if total is None:
        return None
    x1, x2 = total.GetXaxis().GetXmin(), total.GetXaxis().GetXmax()
    ymax = max(total.GetMaximum(), data_h.GetMaximum() if data_h is not None else 0.0)
    ymax = ymax * (80 if logy else 1.75) if ymax > 0 else 1.0
    if data_h is None:
        ratio_label = "MC stat. unc."
    c, up, down = tdr.ratio_canvas(name, xlabel, x1, x2, "Events / bin", ymax, ratio_label, ratio_range, text, logy)
    c.cd(1)
    stack.Draw("hist same")
    total.SetFillStyle(1001); total.SetFillColorAlpha(ROOT.kGray + 2, 0.4); total.SetLineWidth(0); total.SetMarkerSize(0)
    total.Draw("e2 same")
    if data_h is not None:
        data_h.SetMarkerStyle(20); data_h.SetMarkerSize(0.9); data_h.SetLineColor(ROOT.kBlack); data_h.SetLineWidth(1)
        data_h.Draw("e x0 same")
    leg = tdr.legend(0.58, 0.60, 0.93, 0.90, 0.04)
    if data_h is not None:
        leg.AddEntry(data_h, "Data", "lep")
    for hh, lab in reversed(list(zip(keep, labels))):
        leg.AddEntry(hh, lab, "f")
    leg.AddEntry(total, "MC stat. unc.", "f")
    lab = ROOT.TLatex(); lab.SetNDC(); lab.SetTextFont(42); lab.SetTextSize(0.04)
    if region_label:
        lab.DrawLatex(0.19, 0.735, region_label)
    ROOT.gPad.RedrawAxis()
    c.cd(2)
    if info_text:  # metrics line in the free space of the ratio pad
        lab.SetTextSize(0.062); lab.DrawLatex(0.165, 0.80, info_text)
    num = data_h if data_h is not None else total
    ratio = num.Clone(name + "_ratio"); ratio.SetDirectory(0); ratio.Divide(total)
    band = total.Clone(name + "_band"); band.SetDirectory(0)
    for i in range(1, band.GetNbinsX() + 1):
        v = total.GetBinContent(i)
        band.SetBinContent(i, 1.0); band.SetBinError(i, total.GetBinError(i) / v if v > 0 else 0.0)
    band.SetFillStyle(1001); band.SetFillColorAlpha(ROOT.kGray + 2, 0.4); band.SetMarkerSize(0); band.SetLineWidth(0); band.Draw("e2 same")
    line = tdr.unity_line(x1, x2)
    if data_h is not None:
        ratio.SetMarkerStyle(20); ratio.SetMarkerSize(0.9); ratio.SetLineColor(ROOT.kBlack); ratio.Draw("e x0 same")
    ROOT.gPad.RedrawAxis()
    os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
    c.SaveAs(out_path)
    c.Close()
    del line, band, ratio, keep, total, stack, leg, up, down
    return out_path


def plot(hist_file, outdir="plots", regions=None, variables=None, fmt="png", logy=False, lumi_label="", extra="Preliminary"):
    import ROOT
    ROOT.gErrorIgnoreLevel = ROOT.kWarning
    tdr.load()
    f = ROOT.TFile.Open(hist_file)
    if not f or f.IsZombie():
        raise HellError(f"cannot open {hist_file}")
    cfg_str = f.Get("hell/hist_config")
    if not cfg_str:
        raise HellError(f"{hist_file} has no hell/hist_config (not a hell-hist output?)")
    cfg = json.loads(str(cfg_str.GetString()))
    os.makedirs(outdir, exist_ok=True)
    procs = cfg["processes"]
    has_data = any(p.get("isData") for p in procs)
    text = tdr.lumi_text(cfg.get("lumi", 0.0), extra if has_data else "Simulation " + extra if extra else "Simulation", right=lumi_label or None)
    n = 0
    for region in (regions or cfg["regions"].keys()):
        for var in (variables or cfg["variables"].keys()):
            mc, labels, colors, data = [], [], [], None
            for p in procs:
                h = f.Get(f"{region}/{var}/{p['name']}")
                if not h:
                    continue
                h = h.Clone(); h.SetDirectory(0)
                if p.get("isData"):
                    data = h if data is None else (data.Add(h) or data)
                else:
                    mc.append(h); labels.append(p.get("label", p["name"])); colors.append(p.get("color", "#888888"))
            if not mc:
                continue
            out = os.path.join(outdir, f"{region}_{var}.{fmt}")
            if stack_with_ratio(f"{region}_{var}", data, mc, labels, colors, cfg["variables"][var].get("title", var), text, out, logy, region):
                n += 1
    f.Close()
    log(f"{n} plots written to {outdir}")
    return n
