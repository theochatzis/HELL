hell_add_library(Core
  SOURCES
    src/Logger.cc src/ParameterSet.cc src/Event.cc src/ProductRegistry.cc src/Utils.cc
    src/BranchRegistry.cc src/InputSource.cc src/HistService.cc src/FlatTable.cc src/Persistence.cc
    src/OutputTree.cc src/Bookkeeper.cc src/Provenance.cc src/Module.cc src/ModuleFactory.cc src/Forge.cc
  DEPENDS ${CMAKE_DL_LIBS})
