"""ROOT helpers shared by the python tools."""
_quiet = False


def quiet_root():
    """Import ROOT in batch mode and drop the harmless NanoAOD "no dictionary for class edm::..." warnings
    (CMSSW classes in the MetaData/ParameterSets trees that we never read). Every other ROOT message passes."""
    global _quiet
    import ROOT
    if _quiet:
        return ROOT
    ROOT.gROOT.SetBatch(True)
    ROOT.gErrorIgnoreLevel = ROOT.kWarning   # no "Info in <TCanvas::Print>" chatter; warnings and errors still show
    ROOT.gInterpreter.Declare(r"""
#include <TError.h>
#include <cstring>
namespace hellpy {
void filteringHandler(int level, Bool_t abort, const char* location, const char* msg) {
  if (level == kWarning && location && std::strcmp(location, "TClass::Init") == 0 && msg &&
      (std::strstr(msg, "no dictionary for class edm::") || std::strstr(msg, "no dictionary for class pair<edm::")))
    return;
  DefaultErrorHandler(level, abort, location, msg);
}
void install() { SetErrorHandler(filteringHandler); }
}
""")
    ROOT.hellpy.install()
    _quiet = True
    return ROOT
