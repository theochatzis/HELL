#include "Core/interface/Utils.h"
#include <unistd.h>
#include <pwd.h>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>

namespace hell {

bool globMatch(const std::string& p, const std::string& t) {
  size_t pi = 0, ti = 0, star = std::string::npos, match = 0;
  while (ti < t.size()) {
    if (pi < p.size() && (p[pi] == '?' || p[pi] == t[ti])) { ++pi; ++ti; }
    else if (pi < p.size() && p[pi] == '*') { star = pi++; match = ti; }
    else if (star != std::string::npos) { pi = star + 1; ti = ++match; }
    else return false;
  }
  while (pi < p.size() && p[pi] == '*') ++pi;
  return pi == p.size();
}

std::vector<std::string> splitString(const std::string& s, char sep, bool skipEmpty) {
  std::vector<std::string> out;
  std::string cur;
  std::istringstream in(s);
  while (std::getline(in, cur, sep)) if (!(skipEmpty && cur.empty())) out.push_back(cur);
  return out;
}

std::string trim(const std::string& s) {
  const auto a = s.find_first_not_of(" \t\r\n");
  if (a == std::string::npos) return "";
  const auto b = s.find_last_not_of(" \t\r\n");
  return s.substr(a, b - a + 1);
}
bool startsWith(const std::string& s, const std::string& p) { return s.compare(0, p.size(), p) == 0; }
bool endsWith(const std::string& s, const std::string& p) { return s.size() >= p.size() && s.compare(s.size() - p.size(), p.size(), p) == 0; }
std::string joinStrings(const std::vector<std::string>& v, const std::string& sep) {
  std::string out;
  for (size_t i = 0; i < v.size(); ++i) out += (i ? sep : "") + v[i];
  return out;
}
std::string nowISO8601() {
  auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::ostringstream os;
  os << std::put_time(std::localtime(&t), "%Y-%m-%dT%H:%M:%S");
  return os.str();
}
std::string hostName() { char buf[256]; if (gethostname(buf, sizeof(buf)) == 0) return buf; return "unknown"; }
std::string userName() {
  if (const char* u = std::getenv("USER")) return u;
  if (passwd* pw = getpwuid(getuid())) return pw->pw_name;
  return "unknown";
}
std::string currentDirectory() { char buf[4096]; if (getcwd(buf, sizeof(buf))) return buf; return ""; }
std::string readTextFile(const std::string& path) {
  std::ifstream in(path);
  std::stringstream ss; ss << in.rdbuf();
  return ss.str();
}
bool fileExists(const std::string& path) { struct stat st; return stat(path.c_str(), &st) == 0; }
std::string baseName(const std::string& path) { const auto p = path.find_last_of('/'); return p == std::string::npos ? path : path.substr(p + 1); }
std::string formatDuration(double s) {
  std::ostringstream os;
  if (s < 60) os << std::fixed << std::setprecision(1) << s << " s";
  else if (s < 3600) os << int(s / 60) << " min " << int(s) % 60 << " s";
  else os << int(s / 3600) << " h " << (int(s) % 3600) / 60 << " min";
  return os.str();
}
}  // namespace hell
