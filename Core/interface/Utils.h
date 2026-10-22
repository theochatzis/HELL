// Small helpers shared by the framework.
#pragma once
#include <string>
#include <vector>

namespace hell {
// Shell-style glob match supporting '*' and '?'.
bool globMatch(const std::string& pattern, const std::string& text);
std::vector<std::string> splitString(const std::string& s, char sep, bool skipEmpty = true);
std::string trim(const std::string& s);
bool startsWith(const std::string& s, const std::string& prefix);
bool endsWith(const std::string& s, const std::string& suffix);
std::string joinStrings(const std::vector<std::string>& v, const std::string& sep);
std::string nowISO8601();
std::string hostName();
std::string userName();
std::string currentDirectory();
std::string readTextFile(const std::string& path);
bool fileExists(const std::string& path);
std::string baseName(const std::string& path);
std::string formatDuration(double seconds);
}  // namespace hell
