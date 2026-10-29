"""CMS TDR plotting style for PyROOT scripts: the C++ hell::tdr library
(Plot/interface/TdrStyle.h, a port of tdrstyle_mod22.C) loaded through
cling, plus small helpers.

    from hell import tdr
    T = tdr.load()                                   # the hell.tdr namespace
    text = tdr.lumi_text(lumi_pb=7980.4, extra="Preliminary")
    c = T.tdrDiCanvas("c", axis_up, axis_down, text, 11)
"""
import os

from hell import HELL_BASE, HELL_BUILD, HellError

_loaded = None


def load():
    """Load libHellPlot and declare the header; returns ROOT.hell.tdr."""
    global _loaded
    if _loaded is not None:
        return _loaded
    from hell.rootutils import quiet_root
    ROOT = quiet_root()
    lib = os.path.join(HELL_BUILD, "lib", "libHellPlot.so")
    if not os.path.exists(lib):
        raise HellError(f"{lib} not found: build HELL first")
    if ROOT.gSystem.Load(lib) < 0:
        raise HellError(f"cannot load {lib}")
    ROOT.gInterpreter.AddIncludePath(HELL_BASE)
    if not ROOT.gInterpreter.Declare('#include "Plot/interface/TdrStyle.h"'):
        raise HellError("cannot declare Plot/interface/TdrStyle.h")
    ROOT.TH1.AddDirectory(False)
    ROOT.TGaxis.SetMaxDigits(4)   # 18000 -> 18 x 10^3 keeps the y labels narrow
    _loaded = ROOT.hell.tdr
    _loaded.setTDRStyle()
    return _loaded


def lumi_text(lumi_pb=0.0, extra="Preliminary", energy="13.6 TeV", right=None):
    T = load()
    t = T.LumiText()
    t.cms = "CMS"
    t.extra = extra
    t.writeExtra = bool(extra)
    t.right = right if right is not None else T.lumiLabel(lumi_pb, energy)
    return t


def color(name):
    return load().colorFromName(str(name))


def axis(name, xlabel, x1, x2, ylabel, y1, y2):
    import ROOT
    h = load().tdrHist(name, ylabel, y1, y2, xlabel, x1, x2)
    ROOT.SetOwnership(h, True)
    return h


def legend(x1=0.55, y1=0.62, x2=0.92, y2=0.90, size=0.04, columns=1):
    import ROOT
    leg = load().tdrLeg(x1, y1, x2, y2, size, columns)
    ROOT.SetOwnership(leg, True)
    return leg


def ratio_canvas(name, xlabel, x1, x2, ylabel, ymax, ratio_label="Data / MC", ratio_range=(0.5, 1.5), text=None, logy=False):
    """Two-pad canvas with TDR axes. Returns (canvas, axis_up, axis_down); cd(1) is the main pad."""
    import ROOT
    T = load()
    up = axis(name + "_up", xlabel, x1, x2, ylabel, 0.1 if logy else 0.0, ymax)
    down = axis(name + "_down", xlabel, x1, x2, ratio_label, ratio_range[0], ratio_range[1])
    ROOT.TGaxis.SetMaxDigits(4)   # 18000 -> 18 x 10^3 keeps the y labels narrow
    c = T.tdrDiCanvas(name, up, down, text or lumi_text(), 11)
    ROOT.SetOwnership(c, True)
    up.GetYaxis().SetTitleOffset(up.GetYaxis().GetTitleOffset() * 1.15)
    up.GetYaxis().SetMaxDigits(4)
    if logy:
        c.cd(1)
        ROOT.gPad.SetLogy()
    return c, up, down


def unity_line(x1, x2):
    import ROOT
    line = ROOT.TLine(x1, 1.0, x2, 1.0)
    line.SetLineStyle(2)
    line.SetLineColor(ROOT.kGray + 2)
    line.Draw()
    return line
