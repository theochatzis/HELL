// Registry of module types and loader of plugin libraries.
// Register a module at the end of its .cc file:
//   HELL_REGISTER_MODULE(hell::JetSelector, "Selects jets by kinematics and id");
// Plugins (lib/plugins/libHell*Plugin.so) are dlopen'ed at start-up; static
// initialisation of the macro registers the creator.
#pragma once
#include "Core/interface/Module.h"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace hell {

struct ModuleTypeInfo {
  std::string type;
  std::string doc;
  std::string library;  // filled when known
};

class ModuleFactory {
 public:
  using Creator = std::function<std::unique_ptr<ModuleBase>(const ParameterSet&, ModuleContext&)>;
  static ModuleFactory& instance();
  void registerModule(const std::string& type, Creator creator, const std::string& doc);
  std::unique_ptr<ModuleBase> create(const std::string& type, const ParameterSet& ps, ModuleContext& ctx) const;
  bool has(const std::string& type) const { return creators_.count(type) > 0; }
  std::vector<ModuleTypeInfo> available() const;

 private:
  std::map<std::string, Creator> creators_;
  std::map<std::string, ModuleTypeInfo> info_;
};

class PluginLoader {
 public:
  // Loads every libHell*Plugin.so from: explicit dirs, $HELL_PLUGIN_PATH, <exe>/../lib/plugins.
  static std::vector<std::string> loadAll(const std::vector<std::string>& dirs = {}, const std::string& pattern = "libHell*Plugin*.so");
  static bool load(const std::string& path);
};

template <class T>
struct ModuleRegistrar {
  ModuleRegistrar(const char* fullName, const char* doc) {
    std::string name(fullName);
    const auto pos = name.rfind("::");
    if (pos != std::string::npos) name = name.substr(pos + 2);
    ModuleFactory::instance().registerModule(
        name, [](const ParameterSet& ps, ModuleContext& ctx) { return std::unique_ptr<ModuleBase>(new T(ps, ctx)); }, doc);
  }
};

}  // namespace hell

#define HELL_CONCAT_INNER(a, b) a##b
#define HELL_CONCAT(a, b) HELL_CONCAT_INNER(a, b)
#define HELL_REGISTER_MODULE(TYPE, DOC) \
  static hell::ModuleRegistrar<TYPE> HELL_CONCAT(hellModuleRegistrar_, __COUNTER__)(#TYPE, DOC)
