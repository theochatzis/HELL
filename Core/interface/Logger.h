// HELL logging. Every line starts with the [HELL] prefix so that logs from
// batch jobs can be grepped. Usage:
//   HELL_INFO("jetSelector") << "kept " << n << " jets";
//   HELL_WARN("input") << "file skipped";
#pragma once
#include <iostream>
#include <sstream>
#include <string>

namespace hell {

enum class LogLevel { Debug = 0, Info = 1, Warning = 2, Error = 3, Silent = 4 };

class Logger {
 public:
  static LogLevel level();
  static void setLevel(LogLevel l);
  static void setLevel(const std::string& name);  // "DEBUG", "INFO", "WARNING", "ERROR", "SILENT"
  static std::string levelName(LogLevel l);
  static bool enabled(LogLevel l) { return static_cast<int>(l) >= static_cast<int>(level()); }
};

class LogStream {
 public:
  LogStream(LogLevel l, const std::string& context) : level_(l), context_(context) {}
  ~LogStream() {
    if (!Logger::enabled(level_)) return;
    std::ostream& os = (level_ >= LogLevel::Warning) ? std::cerr : std::cout;
    os << "[HELL]";
    if (level_ == LogLevel::Debug) os << "[debug]";
    if (level_ == LogLevel::Warning) os << " WARNING:";
    if (level_ == LogLevel::Error) os << " ERROR:";
    if (!context_.empty()) os << " (" << context_ << ")";
    os << " " << buffer_.str() << std::endl;
  }
  template <class T>
  LogStream& operator<<(const T& v) {
    buffer_ << v;
    return *this;
  }

 private:
  LogLevel level_;
  std::string context_;
  std::ostringstream buffer_;
};

// Installs a ROOT message handler that drops the harmless "no dictionary for class edm::..." warnings
// that ROOT prints when opening NanoAOD files (their MetaData/ParameterSets trees hold CMSSW classes we
// never read) and forwards every other ROOT message to the default handler.
void installRootMessageFilter();

}  // namespace hell

#define HELL_DEBUG(ctx) hell::LogStream(hell::LogLevel::Debug, ctx)
#define HELL_INFO(ctx) hell::LogStream(hell::LogLevel::Info, ctx)
#define HELL_WARN(ctx) hell::LogStream(hell::LogLevel::Warning, ctx)
#define HELL_ERROR(ctx) hell::LogStream(hell::LogLevel::Error, ctx)
