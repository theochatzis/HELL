// CMS TDR style, ported from tdrstyle_mod22.C (setTDRStyle + CMS_lumi and the
// tdrCanvas / tdrDiCanvas / tdrHist / tdrLeg / tdrDraw helpers by Mikko
// Voutilainen), through the HARP port. Used by the C++ plotters and, through
// PyROOT (python/hell/tdr.py), by `hell plot` and `hell validate`.
// Returned canvases, histograms and legends are owned by the caller.
#pragma once
#include <TCanvas.h>
#include <TGraph.h>
#include <TH1.h>
#include <TH1D.h>
#include <TLegend.h>
#include <TPad.h>
#include <string>

namespace hell {
namespace tdr {

constexpr bool kSquare = true;
constexpr bool kRectangular = false;

// Text written by cmsLumi(): "CMS" (bold), the extra line(s) (italic) and the right-hand text.
struct LumiText {
  std::string cms = "CMS";
  std::string extra = "Preliminary";   // "Preliminary", "Simulation", "Simulation Preliminary", "Work in progress"
  std::string extra2 = "";
  std::string right = "(13.6 TeV)";    // e.g. "7.98 fb^{-1} (13.6 TeV)"
  bool writeExtra = true;
};

// Right-hand text from a luminosity in pb^-1 and a centre-of-mass energy label.
std::string lumiLabel(double lumiPb, const std::string& energy = "13.6 TeV");
// ROOT colour from "#rrggbb", "kRed+1" style names or a plain number.
int colorFromName(const std::string& name);

void setTDRStyle();
// iPosX: 11 top-left in frame, 33 top-right, 22 centre, 0 out of frame (CMS left, extra text next to it)
void cmsLumi(TPad* pad, const LumiText& text, int iPosX = 11);

// Empty axis histogram (not attached to any directory).
TH1D* tdrHist(const std::string& name, const std::string& ylabel, double y1, double y2, const std::string& xlabel, double x1, double x2);
// Canvas with the axis histogram drawn ("AXIS") and the CMS text.
TCanvas* tdrCanvas(const std::string& name, TH1D* axis, const LumiText& text, int iPos = 11, bool square = kSquare, double rightMargin = -1);
// Two pads sharing the x axis (main + ratio). Pad 1 is the upper, pad 2 the lower.
TCanvas* tdrDiCanvas(const std::string& name, TH1D* axisUp, TH1D* axisDown, const LumiText& text, int iPos = 11);
TLegend* tdrLeg(double x1, double y1, double x2, double y2, double textSize = 0.045, int nColumns = 1);
void tdrDraw(TH1* h, const std::string& opt, int marker = 20, int mcolor = 1, int lstyle = 1, int lcolor = -1,
             int fstyle = 1001, int fcolor = 401, double msize = 1, double lwidth = 1, double falpha = 1);
void tdrDraw(TGraph* g, const std::string& opt, int marker = 20, int mcolor = 1, int lstyle = 1, int lcolor = -1,
             int fstyle = 1001, int fcolor = 401, double msize = 1, double lwidth = 1, double falpha = 1);

}  // namespace tdr
}  // namespace hell
