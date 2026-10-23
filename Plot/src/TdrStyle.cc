#include "Plot/interface/TdrStyle.h"
#include <TColor.h>
#include <TFrame.h>
#include <TLatex.h>
#include <TROOT.h>
#include <TStyle.h>
#include <cmath>
#include <cstdio>
#include <map>

namespace hell {
namespace tdr {

std::string lumiLabel(double lumiPb, const std::string& energy) {
  char buf[64];
  if (lumiPb <= 0) return "(" + energy + ")";
  if (lumiPb >= 1000) std::snprintf(buf, sizeof(buf), "%.1f fb^{-1}", lumiPb / 1000.0);
  else std::snprintf(buf, sizeof(buf), "%.1f pb^{-1}", lumiPb);
  return std::string(buf) + " (" + energy + ")";
}

int colorFromName(const std::string& name) {
  if (name.empty()) return kBlack;
  if (name[0] == '#') return TColor::GetColor(name.c_str());
  static const std::map<std::string, int> base = {{"kWhite", kWhite}, {"kBlack", kBlack}, {"kGray", kGray}, {"kRed", kRed}, {"kGreen", kGreen},
      {"kBlue", kBlue}, {"kYellow", kYellow}, {"kMagenta", kMagenta}, {"kCyan", kCyan}, {"kOrange", kOrange}, {"kSpring", kSpring},
      {"kTeal", kTeal}, {"kAzure", kAzure}, {"kViolet", kViolet}, {"kPink", kPink}};
  if (name[0] == 'k') {
    size_t i = name.find_first_of("+-");
    const std::string stem = name.substr(0, i);
    auto it = base.find(stem);
    if (it == base.end()) return kBlack;
    return it->second + (i == std::string::npos ? 0 : std::stoi(name.substr(i)));
  }
  try { return std::stoi(name); } catch (...) { return kBlack; }
}

void setTDRStyle() {
  static TStyle* tdrStyle = nullptr;
  if (!tdrStyle) {
    tdrStyle = new TStyle("tdrStyle", "Style for P-TDR");
    tdrStyle->SetCanvasBorderMode(0); tdrStyle->SetCanvasColor(kWhite);
    tdrStyle->SetCanvasDefH(600); tdrStyle->SetCanvasDefW(600); tdrStyle->SetCanvasDefX(0); tdrStyle->SetCanvasDefY(0);
    tdrStyle->SetPadBorderMode(0); tdrStyle->SetPadColor(kWhite);
    tdrStyle->SetPadGridX(false); tdrStyle->SetPadGridY(false); tdrStyle->SetGridColor(0); tdrStyle->SetGridStyle(3); tdrStyle->SetGridWidth(1);
    tdrStyle->SetFrameBorderMode(0); tdrStyle->SetFrameBorderSize(1); tdrStyle->SetFrameFillColor(0); tdrStyle->SetFrameFillStyle(0);
    tdrStyle->SetFrameLineColor(1); tdrStyle->SetFrameLineStyle(1); tdrStyle->SetFrameLineWidth(1);
    tdrStyle->SetHistLineColor(1); tdrStyle->SetHistLineStyle(0); tdrStyle->SetHistLineWidth(1);
    tdrStyle->SetEndErrorSize(2); tdrStyle->SetMarkerStyle(20);
    tdrStyle->SetOptFit(1); tdrStyle->SetFitFormat("5.4g"); tdrStyle->SetFuncColor(2); tdrStyle->SetFuncStyle(1); tdrStyle->SetFuncWidth(1);
    tdrStyle->SetOptDate(0);
    tdrStyle->SetOptFile(0); tdrStyle->SetOptStat(0); tdrStyle->SetStatColor(kWhite); tdrStyle->SetStatFont(42); tdrStyle->SetStatFontSize(0.025);
    tdrStyle->SetStatTextColor(1); tdrStyle->SetStatFormat("6.4g"); tdrStyle->SetStatBorderSize(1); tdrStyle->SetStatH(0.1); tdrStyle->SetStatW(0.15);
    tdrStyle->SetPadTopMargin(0.05); tdrStyle->SetPadBottomMargin(0.13); tdrStyle->SetPadLeftMargin(0.16); tdrStyle->SetPadRightMargin(0.02);
    tdrStyle->SetOptTitle(0); tdrStyle->SetTitleFont(42); tdrStyle->SetTitleColor(1); tdrStyle->SetTitleTextColor(1); tdrStyle->SetTitleFillColor(10); tdrStyle->SetTitleFontSize(0.05);
    tdrStyle->SetTitleColor(1, "XYZ"); tdrStyle->SetTitleFont(42, "XYZ"); tdrStyle->SetTitleSize(0.06, "XYZ");
    tdrStyle->SetTitleXOffset(0.9); tdrStyle->SetTitleYOffset(1.25);
    tdrStyle->SetLabelColor(1, "XYZ"); tdrStyle->SetLabelFont(42, "XYZ"); tdrStyle->SetLabelOffset(0.007, "XYZ"); tdrStyle->SetLabelSize(0.05, "XYZ");
    tdrStyle->SetAxisColor(1, "XYZ"); tdrStyle->SetStripDecimals(kTRUE); tdrStyle->SetTickLength(0.03, "XYZ"); tdrStyle->SetNdivisions(510, "XYZ");
    tdrStyle->SetPadTickX(1); tdrStyle->SetPadTickY(1);
    tdrStyle->SetOptLogx(0); tdrStyle->SetOptLogy(0); tdrStyle->SetOptLogz(0);
    tdrStyle->SetPaperSize(20., 20.);
    tdrStyle->SetHatchesLineWidth(5); tdrStyle->SetHatchesSpacing(0.05);
    tdrStyle->SetPalette(kViridis); tdrStyle->SetNumberContours(100);
    tdrStyle->SetLegendFont(42);
  }
  tdrStyle->cd();
  gROOT->ForceStyle();
}

void cmsLumi(TPad* pad, const LumiText& text, int iPosX) {
  const float cmsTextFont = 61, extraTextFont = 52;
  const float lumiTextSize = 0.6, lumiTextOffset = 0.2, cmsTextSize = 0.75;
  const float relPosY = 0.035, relExtraDY = 1.2, extraOverCmsTextSize = 0.76;
  float relPosX = 0.045;
  const bool outOfFrame = (iPosX / 10 == 0);
  int alignY_ = 3, alignX_ = 2;
  if (iPosX / 10 == 0) alignX_ = 1;
  if (iPosX == 0) { alignX_ = 1; alignY_ = 1; }
  if (iPosX / 10 == 1) alignX_ = 1;
  if (iPosX / 10 == 2) alignX_ = 2;
  if (iPosX / 10 == 3) alignX_ = 3;
  if (iPosX == 0) relPosX = pad->GetLeftMargin();
  const int align_ = 10 * alignX_ + alignY_;
  const float l = pad->GetLeftMargin(), t = pad->GetTopMargin(), r = pad->GetRightMargin(), b = pad->GetBottomMargin();
  pad->cd();
  TLatex latex;
  latex.SetNDC(); latex.SetTextAngle(0); latex.SetTextColor(kBlack);
  const float extraTextSize = extraOverCmsTextSize * cmsTextSize;
  latex.SetTextFont(42); latex.SetTextAlign(31); latex.SetTextSize(lumiTextSize * t);
  latex.DrawLatex(1 - r, 1 - t + lumiTextOffset * t, text.right.c_str());
  if (outOfFrame) {
    latex.SetTextFont(cmsTextFont); latex.SetTextAlign(11); latex.SetTextSize(cmsTextSize * t);
    latex.DrawLatex(l, 1 - t + lumiTextOffset * t, text.cms.c_str());
  }
  float posX_ = 0;
  if (iPosX % 10 <= 1) posX_ = l + relPosX * (1 - l - r);
  else if (iPosX % 10 == 2) posX_ = l + 0.5 * (1 - l - r);
  else if (iPosX % 10 == 3) posX_ = 1 - r - relPosX * (1 - l - r);
  float posY_ = 1 - t - relPosY * (1 - t - b);
  if (!outOfFrame) {
    latex.SetTextFont(cmsTextFont); latex.SetTextSize(cmsTextSize * t); latex.SetTextAlign(align_);
    latex.DrawLatex(posX_, posY_, text.cms.c_str());
    if (text.writeExtra) {
      latex.SetTextFont(extraTextFont); latex.SetTextAlign(align_); latex.SetTextSize(extraTextSize * t);
      latex.DrawLatex(posX_, posY_ - relExtraDY * cmsTextSize * t, text.extra.c_str());
      if (!text.extra2.empty()) latex.DrawLatex(posX_, posY_ - relExtraDY * cmsTextSize * t - relExtraDY * extraTextSize * t, text.extra2.c_str());
    }
  } else if (text.writeExtra) {
    if (iPosX == 0) { posX_ = l + relPosX * (1 - l - r); posY_ = 1 - t + lumiTextOffset * t; }
    latex.SetTextFont(extraTextFont); latex.SetTextSize(extraTextSize * t); latex.SetTextAlign(align_);
    latex.DrawLatex(posX_, posY_, text.extra.c_str());
  }
}

TH1D* tdrHist(const std::string& name, const std::string& ylabel, double y1, double y2, const std::string& xlabel, double x1, double x2) {
  auto* h = new TH1D(name.c_str(), Form(";%s;%s", xlabel.c_str(), ylabel.c_str()), std::max(1, int(std::fabs(x2 - x1))), x1, x2);
  h->SetDirectory(nullptr);
  h->GetXaxis()->SetMoreLogLabels();
  h->GetXaxis()->SetNoExponent();
  h->SetMinimum(y1);
  h->SetMaximum(y2);
  return h;
}

TCanvas* tdrCanvas(const std::string& name, TH1D* axis, const LumiText& text, int iPos, bool square, double rightMargin) {
  setTDRStyle();
  const int W = square ? 600 : 800, H = 600;
  const float T = square ? 0.07f * H : 0.08f * H, B = square ? 0.13f * H : 0.12f * H;
  const float L = square ? 0.15f * W : 0.12f * W, R = square ? 0.05f * W : 0.04f * W;
  auto* canv = new TCanvas(name.c_str(), name.c_str(), 50, 50, W, H);
  canv->SetFillColor(0); canv->SetBorderMode(0); canv->SetFrameFillStyle(0); canv->SetFrameBorderMode(0);
  canv->SetLeftMargin(L / W); canv->SetRightMargin(rightMargin > 0 ? rightMargin : R / W); canv->SetTopMargin(T / H); canv->SetBottomMargin(B / H);
  axis->GetYaxis()->SetTitleOffset(square ? 1.25 : 1);
  axis->GetXaxis()->SetTitleOffset(square ? 1.0 : 0.9);
  axis->Draw("AXIS");
  cmsLumi(canv, text, iPos);
  canv->Update(); canv->RedrawAxis(); canv->GetFrame()->Draw();
  return canv;
}

TCanvas* tdrDiCanvas(const std::string& name, TH1D* hup, TH1D* hdw, const LumiText& text, int iPos) {
  setTDRStyle();
  const int W_ref = 600, H_ref = 600;
  const float F_ref = 1. / 3., M_ref = 0.03, T_ref = 0.07, B_ref = 0.13, L = 0.15, R = 0.05;
  const int W = W_ref;
  const int H = static_cast<int>(H_ref * (1 + (1 - T_ref - B_ref) * F_ref + M_ref));
  const float Hup = H_ref * (1 - B_ref), Hdw = H - Hup;
  const float Tup = T_ref * H_ref / Hup, Tdw = M_ref * H_ref / Hdw, Bup = 0.01f, Bdw = B_ref * H_ref / Hdw;
  auto* canv = new TCanvas(name.c_str(), name.c_str(), 50, 50, W, H);
  canv->SetFillColor(0); canv->SetBorderMode(0); canv->SetFrameFillStyle(0); canv->SetFrameBorderMode(0);
  canv->SetFrameLineColor(0); canv->SetFrameLineWidth(0);
  canv->Divide(1, 2);
  canv->cd(1);
  gPad->SetPad(0, Hdw / H, 1, 1);
  gPad->SetLeftMargin(L); gPad->SetRightMargin(R); gPad->SetTopMargin(Tup); gPad->SetBottomMargin(Bup);
  hup->GetYaxis()->SetTitleOffset(1.25 * Hup / H_ref); hup->GetXaxis()->SetTitleOffset(1.0);
  hup->SetTitleSize(hup->GetTitleSize("Y") * H_ref / Hup, "Y"); hup->SetLabelSize(hup->GetLabelSize("Y") * H_ref / Hup, "Y");
  hup->SetTickLength(hup->GetTickLength("Y") * Hup / H_ref, "Y"); hup->SetTickLength(hup->GetTickLength("X") * H_ref / Hup, "X");
  hup->Draw("AXIS");
  cmsLumi(static_cast<TPad*>(gPad), text, iPos);
  canv->cd(2);
  gPad->SetPad(0, 0, 1, Hdw / H);
  gPad->SetLeftMargin(L); gPad->SetRightMargin(R); gPad->SetTopMargin(Tdw); gPad->SetBottomMargin(Bdw);
  hdw->GetYaxis()->SetTitleOffset(1.25 * Hdw / H_ref); hdw->GetXaxis()->SetTitleOffset(1.0);
  hdw->SetTitleSize(hdw->GetTitleSize("Y") * H_ref / Hdw, "Y"); hdw->SetLabelSize(hdw->GetLabelSize("Y") * H_ref / Hdw, "Y");
  hdw->SetTitleSize(hdw->GetTitleSize("X") * H_ref / Hdw, "X"); hdw->SetLabelSize(hdw->GetLabelSize("X") * H_ref / Hdw, "X");
  hdw->SetTickLength(hdw->GetTickLength("Y") * H_ref / Hup, "Y"); hdw->SetTickLength(hdw->GetTickLength("X") * H_ref / Hdw, "X");
  hdw->GetYaxis()->SetNdivisions(504);
  hdw->Draw("AXIS");
  canv->cd(0);
  canv->Update(); canv->RedrawAxis(); canv->GetFrame()->Draw();
  return canv;
}

TLegend* tdrLeg(double x1, double y1, double x2, double y2, double textSize, int nColumns) {
  auto* leg = new TLegend(x1, y1, x2, y2, "", "brNDC");
  if (nColumns > 1) leg->SetNColumns(nColumns);
  leg->SetFillStyle(kNone); leg->SetBorderSize(0); leg->SetTextSize(textSize);
  leg->Draw();
  return leg;
}

void tdrDraw(TH1* h, const std::string& opt, int marker, int mcolor, int lstyle, int lcolor, int fstyle, int fcolor, double msize, double lwidth, double falpha) {
  if (!h) return;
  h->SetMarkerStyle(marker); h->SetMarkerColor(mcolor); h->SetMarkerSize(msize);
  h->SetLineStyle(lstyle); h->SetLineColor(lcolor == -1 ? mcolor : lcolor); h->SetLineWidth(lwidth);
  h->SetFillStyle(fstyle); h->SetFillColor(fcolor);
  if (falpha != 1) h->SetFillColorAlpha(fcolor, falpha);
  h->Draw((opt + "SAME").c_str());
}

void tdrDraw(TGraph* g, const std::string& opt, int marker, int mcolor, int lstyle, int lcolor, int fstyle, int fcolor, double msize, double lwidth, double falpha) {
  if (!g) return;
  g->SetMarkerStyle(marker); g->SetMarkerColor(mcolor); g->SetMarkerSize(msize);
  g->SetLineStyle(lstyle); g->SetLineColor(lcolor == -1 ? mcolor : lcolor); g->SetLineWidth(lwidth);
  g->SetFillStyle(fstyle); g->SetFillColor(fcolor);
  if (falpha != 1) g->SetFillColorAlpha(fcolor, falpha);
  g->Draw((opt + "SAME").c_str());
}

}  // namespace tdr
}  // namespace hell
