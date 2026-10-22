"""Stacked data/MC plots from a hell-hist output file (PyROOT, no cmsstyle
dependency; cmsstyle can be plugged in later). Colours and labels come from
the histogram configuration stored in the file (hell/hist_config)."""
import json
import os

from hell import HellError, log


def _color(hexstr):
    import ROOT
    return ROOT.TColor.GetColor(hexstr) if hexstr.startswith("#") else int(hexstr)


def plot(hist_file, outdir="plots", regions=None, variables=None, fmt="png", logy=False, lumi_label=""):
    import ROOT
    ROOT.gROOT.SetBatch(True)
    ROOT.gStyle.SetOptStat(0)
    ROOT.gErrorIgnoreLevel = ROOT.kWarning
    ROOT.TH1.AddDirectory(False)   # clones live in memory, not in the input file
    f = ROOT.TFile.Open(hist_file)
    if not f or f.IsZombie():
        raise HellError(f"cannot open {hist_file}")
    cfg_str = f.Get("hell/hist_config")
    if not cfg_str:
        raise HellError(f"{hist_file} has no hell/hist_config (not a hell-hist output?)")
    cfg = json.loads(str(cfg_str.GetString()))
    os.makedirs(outdir, exist_ok=True)
    procs = cfg["processes"]
    n = 0
    for region in (regions or cfg["regions"].keys()):
        for var in (variables or cfg["variables"].keys()):
            c = ROOT.TCanvas("c", "", 800, 800)
            pad1 = ROOT.TPad("p1", "", 0, 0.3, 1, 1); pad1.SetBottomMargin(0.02); pad1.Draw()
            pad2 = ROOT.TPad("p2", "", 0, 0, 1, 0.3); pad2.SetTopMargin(0.04); pad2.SetBottomMargin(0.35); pad2.Draw()
            ROOT.SetOwnership(pad1, False); ROOT.SetOwnership(pad2, False)  # the canvas owns its pads
            pad1.cd()
            if logy:
                pad1.SetLogy()
            stack = ROOT.THStack("s", "")
            leg = ROOT.TLegend(0.6, 0.6, 0.88, 0.88); leg.SetBorderSize(0); leg.SetFillStyle(0)
            total = None
            data = None
            keep = []
            for p in procs:
                h = f.Get(f"{region}/{var}/{p['name']}")
                if not h:
                    continue
                h = h.Clone(); keep.append(h)
                if p.get("isData"):
                    data = h; h.SetMarkerStyle(20); h.SetLineColor(ROOT.kBlack)
                    continue
                h.SetFillColor(_color(p.get("color", "#888888"))); h.SetLineColor(ROOT.kBlack)
                stack.Add(h)
                if total is None:
                    total = h.Clone("total"); keep.append(total)
                else:
                    total.Add(h)
            for p in reversed(procs):
                for h in keep:
                    if h.GetName() == p["name"]:
                        leg.AddEntry(h, p.get("label", p["name"]), "f" if not p.get("isData") else "lep")
            if total is None:
                continue
            ymax = max(total.GetMaximum(), data.GetMaximum() if data else 0)
            stack.SetMaximum(ymax * (50 if logy else 1.5)); stack.SetMinimum(0.1 if logy else 0)
            stack.Draw("hist")
            stack.GetXaxis().SetLabelSize(0)
            stack.GetYaxis().SetTitle("Events")
            total.SetFillStyle(3354); total.SetFillColor(ROOT.kGray + 2); total.SetMarkerSize(0); total.Draw("e2 same")
            if data:
                data.Draw("e same")
            leg.Draw()
            lab = ROOT.TLatex(); lab.SetNDC(); lab.SetTextSize(0.045)
            lab.DrawLatex(0.12, 0.92, "#bf{CMS} #it{HELL}  " + region)
            if lumi_label:
                lab.DrawLatex(0.62, 0.92, lumi_label)
            pad2.cd()
            ratio = (data if data else total).Clone("ratio")
            ratio.Divide(total)
            ratio.SetTitle(""); ratio.GetYaxis().SetTitle("Data / MC" if data else "MC / MC")
            ratio.GetYaxis().SetNdivisions(505); ratio.GetYaxis().SetRangeUser(0.5, 1.5)
            ratio.GetXaxis().SetTitle(cfg["variables"][var].get("title", var))
            for ax in (ratio.GetXaxis(), ratio.GetYaxis()):
                ax.SetTitleSize(0.12); ax.SetLabelSize(0.1)
            ratio.GetYaxis().SetTitleOffset(0.45)
            ratio.Draw("e")
            line = ROOT.TLine(ratio.GetXaxis().GetXmin(), 1, ratio.GetXaxis().GetXmax(), 1); line.SetLineStyle(2); line.Draw()
            out = os.path.join(outdir, f"{region}_{var}.{fmt}")
            c.SaveAs(out)
            c.Close()
            n += 1
    log(f"{n} plots written to {outdir}")
    return n
