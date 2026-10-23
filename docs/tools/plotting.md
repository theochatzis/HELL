# Plotting

All ROOT plots of HELL use the CMS TDR style: `Plot/interface/TdrStyle.h` (namespace `hell::tdr`) is a port of
`tdrstyle_mod22.C` (setTDRStyle, cmsLumi, tdrCanvas, tdrDiCanvas, tdrHist, tdrLeg, tdrDraw) taken from HARP.
It is a C++ library (`libHellPlot`) usable by C++ plotters and, through `python/hell/tdr.py`, by PyROOT:

```python
from hell import tdr
T = tdr.load()                                       # hell::tdr namespace, TDR style applied
text = tdr.lumi_text(7980.4, extra="Preliminary")    # "8.0 fb^{-1} (13.6 TeV)", CMS Preliminary
c, up, down = tdr.ratio_canvas("c", "m_{l#nub} [GeV]", 100, 300, "Events / bin", 1e4, text=text)
c.cd(1); h.Draw("hist same"); leg = tdr.legend(); ...
```
`hell.plot.stack_with_ratio(...)` is the generic data/MC stack with the MC statistical band and a ratio panel,
used by `hell plot` (histogram files) and `hell validate` (object validation). Labels: "CMS Preliminary" with
data, "CMS Simulation Preliminary" without; the right text is built from the luminosity of the era. Change
the extra text with `--extra` / `extraText` (e.g. "Work in progress", "" for final).

The matplotlib plots of the ML validation use the mplhep CMS style when mplhep is available (LCG view).
