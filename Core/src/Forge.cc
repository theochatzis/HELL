#include "Core/interface/Forge.h"
#include "Core/interface/Logger.h"
#include "Core/interface/ModuleFactory.h"
#include "Core/interface/Utils.h"
#include "Core/interface/Version.h"
#include <TObjArray.h>
#include <TStopwatch.h>
#include <set>
#include <iomanip>
#include <sstream>

namespace hell {

Forge::Forge(const ParameterSet& config) : config_(config) {}
Forge::~Forge() = default;

void Forge::initialize() {
  if (initialized_) return;
  const ParameterSet options = config_.getPSet("options", ParameterSet());
  Logger::setLevel(options.get("logLevel", "INFO"));
  reportEvery_ = options.get<long long>("reportEvery", 10000);
  HELL_INFO("forge") << "Welcome to HELL. Version " << HELL_VERSION << " (" << HELL_GIT_HASH << ")";

  // Plugins
  std::vector<std::string> pluginDirs = options.get<std::vector<std::string>>("pluginDirs", {});
  const auto loaded = PluginLoader::loadAll(pluginDirs);
  HELL_INFO("forge") << "loaded " << loaded.size() << " plugin libraries, " << ModuleFactory::instance().available().size() << " module types";

  // Job context
  job_.processName = config_.get("process", "HELL");
  const ParameterSet jobPs = config_.getPSet("job", ParameterSet());
  job_.isData = jobPs.get<bool>("isData", false);
  job_.era = jobPs.get("era", "");
  job_.sample = jobPs.get("sample", "");
  job_.variation = variationOverride_.empty() ? jobPs.get("variation", "nominal") : variationOverride_;
  job_.crossSection = jobPs.get<double>("crossSection", -1.0);
  job_.options = jobPs;

  // Input
  const ParameterSet inPs = config_.getPSet("input");
  std::vector<std::string> files = inputOverride_.empty() ? inPs.get<std::vector<std::string>>("files") : inputOverride_;
  files = InputSource::expandInputFiles(files);
  const std::string treeName = inPs.get("treeName", "Events");
  if (maxEvents_ < 0) maxEvents_ = inPs.get<long long>("maxEvents", -1);
  if (skipEvents_ <= 0) skipEvents_ = inPs.get<long long>("skipEvents", 0);
  input_ = std::make_unique<InputSource>(files, treeName, inPs.get<bool>("probeFiles", true));
  HELL_INFO("forge") << input_->goodFiles().size() << " input files, " << input_->entries() << " entries in tree " << treeName
                     << (input_->badFiles().empty() ? "" : " (" + std::to_string(input_->badFiles().size()) + " files skipped)");
  checkNanoVersion();
  input_->chain().SetBranchStatus("*", 0);

  // Output
  const ParameterSet outPs = config_.getPSet("output", ParameterSet());
  outputFile_ = outputOverride_.empty() ? outPs.get("file", "hell_output.root") : outputOverride_;
  hists_ = std::make_unique<HistService>(outputFile_);
  TDirectory* hellDir = hists_->dir("hell").dir();
  bookkeeper_ = std::make_unique<Bookkeeper>(hellDir, hists_->file(), outPs.get<bool>("copyRuns", true), outPs.get<bool>("copyLumis", true));

  // Provenance
  provenance_.setConfig(config_.raw());
  provenance_.setInputFiles(input_->goodFiles());
  provenance_.setBadInputFiles(input_->badFiles());
  provenance_.set("job", {{"process", job_.processName}, {"isData", job_.isData}, {"era", job_.era}, {"sample", job_.sample},
                          {"variation", job_.variation}, {"crossSection", job_.crossSection}, {"output", outputFile_},
                          {"nanoVersion", jobPs.get("nanoVersion", "")}});

  // Modules
  buildModules();
  validateDependencies();
  std::vector<std::string> labels;
  for (const auto& p : path_) labels.push_back(p.label);
  bookkeeper_->bookCutflow(labels);

  // Framework-level branches (all optional: data has no genWeight, toy trees may lack run/lumi).
  runH_ = branches_.value<unsigned int>("run", true);
  lumiH_ = branches_.value<unsigned int>("luminosityBlock", true);
  eventH_ = branches_.value<unsigned long long>("event", true);
  genWeightH_ = branches_.value<float>("genWeight", true);

  // Tree-change wiring: bind branches first, then everything that depends on them.
  input_->onTreeChange([this](TTree* tree, int) {
    branches_.bind(tree, input_->currentTreeNumber());
    for (auto& f : treeListeners_) f(tree);
    TFile* file = input_->currentFileHandle();
    const std::string path = input_->currentFile();
    bookkeeper_->newInputFile(file, path);
    provenance_.addParent(file, path);
    for (auto& p : path_) p.module->beginFile(path);
  });
  initialized_ = true;
}

// The declared NanoAOD version (job.nanoVersion, settings in job.nano) must match the branch fingerprint of
// the input: branches that must be present and branches that must be absent (samples/nanoaod.yaml).
void Forge::checkNanoVersion() {
  const ParameterSet jobPs = config_.getPSet("job", ParameterSet());
  const std::string version = jobPs.get("nanoVersion", "");
  if (version.empty()) { HELL_WARN("forge") << "no NanoAOD version declared (nanoVersion=): the selections cannot be aligned to the input"; return; }
  if (!jobPs.has("nano")) return;
  const ParameterSet nano = jobPs.getPSet("nano");
  const ParameterSet fp = nano.getPSet("fingerprint", ParameterSet());
  TObjArray* branches = input_->chain().GetListOfBranches();
  std::set<std::string> names;
  if (branches) for (int i = 0; i < branches->GetEntries(); ++i) names.insert(branches->At(i)->GetName());
  std::vector<std::string> missing, unexpected;
  for (const auto& b : fp.get<std::vector<std::string>>("present", {})) if (!names.count(b)) missing.push_back(b);
  for (const auto& b : fp.get<std::vector<std::string>>("absent", {})) if (names.count(b)) unexpected.push_back(b);
  if (missing.empty() && unexpected.empty()) { HELL_INFO("forge") << "input matches NanoAOD v" << version; return; }
  const std::string msg = "the input does not look like NanoAOD v" + version + ": missing branches [" + joinStrings(missing, ", ") +
                          "], unexpected branches [" + joinStrings(unexpected, ", ") + "]. Declare the right version (nanoVersion=) or allowNanoVersionMismatch=True";
  if (jobPs.get<bool>("allowNanoVersionMismatch", false)) HELL_WARN("forge") << msg;
  else throw std::runtime_error(msg);
}

void Forge::buildModules() {
  const ParameterSet modules = config_.getPSet("modules");
  const std::vector<std::string> path = config_.get<std::vector<std::string>>("path");
  if (path.empty()) throw std::runtime_error("the path is empty: nothing to forge");
  for (const auto& label : path) {
    for (const auto& p : path_) if (p.label == label) throw std::runtime_error("module '" + label + "' appears twice in the path");
    if (!modules.has(label)) throw std::runtime_error("module '" + label + "' is in the path but not configured");
    PathEntry entry;
    entry.label = label;
    entry.params = modules.getPSet(label);
    entry.params.setContext("module '" + label + "'");
    const std::string type = entry.params.get<std::string>("@type");
    entry.context = std::make_unique<ModuleContext>(label, type, branches_, products_, *hists_, job_);
    entry.context->setInput(&input_->chain(), &treeListeners_);
    try {
      entry.module = ModuleFactory::instance().create(type, entry.params, *entry.context);
    } catch (const std::exception& e) {
      throw std::runtime_error("cannot build module '" + label + "' of type " + type + ": " + e.what());
    }
    // Warn about parameters in the config that the module never read (typos).
    for (const auto& k : entry.params.keys()) {
      if (k == "@type" || k == "@doc") continue;
      if (!entry.params.accessed().count(k)) HELL_WARN(label) << "parameter '" << k << "' is not used by " << type;
    }
    path_.push_back(std::move(entry));
  }
  HELL_INFO("forge") << "path: " << path.size() << " modules, " << products_.products().size() << " products, " << branches_.size() << " branches";
}

void Forge::validateDependencies() const {
  std::vector<std::string> available;
  for (const auto& p : path_) {
    for (const auto& c : p.context->consumedProducts()) {
      bool ok = false;
      for (const auto& a : available) if (a == c || startsWith(a, c + ":")) ok = true;
      if (!ok) HELL_WARN(p.label) << "consumes product '" << c << "' which is not produced earlier in the path";
    }
    for (const auto& prod : p.context->producedProducts()) available.push_back(prod);
  }
}

int Forge::run() {
  initialize();
  TStopwatch sw; sw.Start();
  for (auto& p : path_) p.module->beginJob();

  Event event;
  event.isData = job_.isData;
  const long long total = input_->entries();
  long long first = skipEvents_;
  long long last = (maxEvents_ < 0) ? total : std::min(total, first + maxEvents_);
  HELL_INFO("forge") << "forging entries [" << first << ", " << last << ") of " << total;
  long long processed = 0;
  for (long long entry = first; entry < last; ++entry) {
    const long long local = input_->load(entry);
    if (local < 0) { HELL_ERROR("forge") << "cannot load entry " << entry; break; }
    branches_.setEntry(local);
    event.clear();
    event.entry = entry;
    event.weight = 1.0;
    event.id.run = runH_.valid() ? *runH_ : 0;
    event.id.lumi = lumiH_.valid() ? *lumiH_ : 0;
    event.id.event = eventH_.valid() ? *eventH_ : 0;
    double genW = 1.0;
    if (!job_.isData && genWeightH_.valid()) { genW = *genWeightH_; bookkeeper_->countGenWeight(genW); }
    bookkeeper_->countEvent(genW);
    ++processed;
    if (reportEvery_ > 0 && processed % reportEvery_ == 0)
      HELL_INFO("forge") << "entry " << entry << " / " << last << "  (run " << event.id.run << ", event " << event.id.event << ")";

    bool passed = true;
    for (size_t i = 0; i < path_.size(); ++i) {
      auto& p = path_[i];
      switch (p.module->kind()) {
        case ModuleKind::Producer: static_cast<Producer*>(p.module.get())->produce(event); break;
        case ModuleKind::Filter: passed = static_cast<Filter*>(p.module.get())->filter(event); break;
        case ModuleKind::Analyzer: static_cast<Analyzer*>(p.module.get())->analyze(event); break;
      }
      if (!passed) break;
      bookkeeper_->countPassed(i, event.weight);
    }
    if (passed) bookkeeper_->countOutput(event.weight);
  }
  for (auto& p : path_) p.module->endJob();
  bookkeeper_->write();
  provenance_.set("bookkeeping", {{"eventsIn", bookkeeper_->eventsIn()}, {"eventsOut", bookkeeper_->eventsOut()},
                                  {"genEventSumw", bookkeeper_->genEventSumw()}, {"genEventCount", bookkeeper_->genEventCount()}});
  provenance_.setProducts(describe());
  provenance_.write(hists_->dir("hell").dir());
  hists_->close();
  sw.Stop();
  HELL_INFO("forge") << "cutflow:\n" << bookkeeper_->summaryTable();
  HELL_INFO("forge") << "forged " << processed << " events in " << formatDuration(sw.RealTime())
                     << (processed > 0 ? " (" + std::to_string(static_cast<long long>(processed / std::max(sw.RealTime(), 1e-9))) + " ev/s)" : "")
                     << ". Output: " << outputFile_;
  return 0;
}

nlohmann::json Forge::describe() const {
  nlohmann::json j;
  j["process"] = job_.processName;
  j["path"] = nlohmann::json::array();
  for (const auto& p : path_) {
    nlohmann::json m;
    m["label"] = p.label;
    m["type"] = p.module->type();
    m["kind"] = moduleKindName(p.module->kind());
    m["parameters"] = p.params.accessedAsJson();
    m["consumesBranches"] = p.context->consumedBranches();
    m["consumesProducts"] = p.context->consumedProducts();
    nlohmann::json prods = nlohmann::json::array();
    for (const auto* pi : products_.byModule(p.label)) prods.push_back({{"label", pi->label}, {"type", pi->typeName}});
    m["produces"] = prods;
    j["path"].push_back(m);
  }
  j["branches"] = branches_.names();
  return j;
}

std::string Forge::summary() const {
  std::ostringstream os;
  os << "process " << job_.processName << (job_.isData ? " [data]" : " [MC]") << " era=" << job_.era << " variation=" << job_.variation << "\n";
  os << std::left << std::setw(24) << "label" << std::setw(28) << "type" << std::setw(10) << "kind" << "produces\n";
  for (const auto& p : path_) {
    std::string prods;
    for (const auto* pi : products_.byModule(p.label)) prods += (prods.empty() ? "" : ", ") + pi->label + " <" + pi->typeName + ">";
    os << std::left << std::setw(24) << p.label << std::setw(28) << p.module->type() << std::setw(10) << moduleKindName(p.module->kind()) << prods << "\n";
  }
  os << branches_.size() << " input branches registered: " << joinStrings(branches_.names(), " ") << "\n";
  return os.str();
}

}  // namespace hell
