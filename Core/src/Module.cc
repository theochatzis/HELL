#include "Core/interface/Module.h"
namespace hell {
std::string moduleKindName(ModuleKind k) {
  switch (k) { case ModuleKind::Producer: return "Producer"; case ModuleKind::Filter: return "Filter"; default: return "Analyzer"; }
}
}
