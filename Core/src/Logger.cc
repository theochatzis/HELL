#include "Core/interface/Logger.h"
#include <TError.h>
#include <algorithm>
#include <cctype>
#include <cstring>

namespace hell {
namespace {
LogLevel& levelRef() { static LogLevel l = LogLevel::Info; return l; }
}
LogLevel Logger::level() { return levelRef(); }
void Logger::setLevel(LogLevel l) { levelRef() = l; }
void Logger::setLevel(const std::string& name) {
  std::string n = name;
  std::transform(n.begin(), n.end(), n.begin(), [](unsigned char c) { return std::toupper(c); });
  if (n == "DEBUG") setLevel(LogLevel::Debug);
  else if (n == "INFO") setLevel(LogLevel::Info);
  else if (n == "WARNING" || n == "WARN") setLevel(LogLevel::Warning);
  else if (n == "ERROR") setLevel(LogLevel::Error);
  else if (n == "SILENT") setLevel(LogLevel::Silent);
  else HELL_WARN("logger") << "unknown log level '" << name << "', keeping " << levelName(level());
}
std::string Logger::levelName(LogLevel l) {
  switch (l) {
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warning: return "WARNING";
    case LogLevel::Error: return "ERROR";
    default: return "SILENT";
  }
}
namespace {
void filteringHandler(int level, Bool_t abort, const char* location, const char* msg) {
  if (level == kWarning && location && std::strcmp(location, "TClass::Init") == 0 && msg &&
      (std::strstr(msg, "no dictionary for class edm::") || std::strstr(msg, "no dictionary for class pair<edm::")))
    return;
  DefaultErrorHandler(level, abort, location, msg);
}
}  // namespace

void installRootMessageFilter() { SetErrorHandler(filteringHandler); }

}  // namespace hell
