#include "Core/interface/ModuleFactory.h"
#include "Core/interface/Logger.h"
#include "Core/interface/Utils.h"
#include <dlfcn.h>
#include <unistd.h>
#include <TSystemDirectory.h>
#include <TList.h>
#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace hell {

ModuleFactory& ModuleFactory::instance() { static ModuleFactory f; return f; }

void ModuleFactory::registerModule(const std::string& type, Creator creator, const std::string& doc) {
  if (creators_.count(type)) { HELL_WARN("factory") << "module type '" << type << "' registered twice, keeping the first"; return; }
  creators_[type] = std::move(creator);
  info_[type] = ModuleTypeInfo{type, doc, ""};
}

std::unique_ptr<ModuleBase> ModuleFactory::create(const std::string& type, const ParameterSet& ps, ModuleContext& ctx) const {
  auto it = creators_.find(type);
  if (it == creators_.end()) {
    std::string known;
    for (const auto& kv : creators_) known += (known.empty() ? "" : ", ") + kv.first;
    throw std::runtime_error("unknown module type '" + type + "'. Known types: " + known);
  }
  return it->second(ps, ctx);
}

std::vector<ModuleTypeInfo> ModuleFactory::available() const {
  std::vector<ModuleTypeInfo> v;
  for (const auto& kv : info_) v.push_back(kv.second);
  return v;
}

bool PluginLoader::load(const std::string& path) {
  void* h = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL);
  if (!h) { HELL_WARN("plugins") << "cannot load " << path << ": " << dlerror(); return false; }
  HELL_DEBUG("plugins") << "loaded " << path;
  return true;
}

std::vector<std::string> PluginLoader::loadAll(const std::vector<std::string>& dirs, const std::string& pattern) {
  std::vector<std::string> search = dirs;
  if (const char* env = std::getenv("HELL_PLUGIN_PATH")) for (const auto& d : splitString(env, ':')) search.push_back(d);
  char exe[4096];
  const ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
  if (n > 0) {
    exe[n] = 0;
    std::string e(exe);
    const auto pos = e.find_last_of('/');
    if (pos != std::string::npos) search.push_back(e.substr(0, pos) + "/../lib/plugins");
  }
  std::vector<std::string> loaded;
  std::vector<std::string> seen;
  for (const auto& d : search) {
    if (d.empty()) continue;
    TSystemDirectory dir(d.c_str(), d.c_str());
    TList* files = dir.GetListOfFiles();
    if (!files) continue;
    std::vector<std::string> names;
    for (TObject* o : *files) { const std::string nme = o->GetName(); if (globMatch(pattern, nme)) names.push_back(nme); }
    delete files;
    std::sort(names.begin(), names.end());
    for (const auto& nme : names) {
      if (std::find(seen.begin(), seen.end(), nme) != seen.end()) continue;
      seen.push_back(nme);
      if (load(d + "/" + nme)) loaded.push_back(d + "/" + nme);
    }
  }
  return loaded;
}

}  // namespace hell
