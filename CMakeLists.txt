# HELL - High Energy Likelihood Laboratory
# "We forge the analysis in HELL."
cmake_minimum_required(VERSION 3.20)
project(HELL LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

find_package(ROOT REQUIRED COMPONENTS Core RIO Tree TreePlayer Hist HistPainter Graf Gpad MathCore Physics GenVector RooFit RooFitCore)

# Follow the C++ standard ROOT was built with (ABI safety), never below 17.
if(DEFINED ROOT_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD ${ROOT_CXX_STANDARD})
else()
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_RPATH "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)

# nlohmann/json: packaged target if available, otherwise header-only from the include path.
find_package(nlohmann_json QUIET)
if(NOT TARGET nlohmann_json::nlohmann_json)
  find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp REQUIRED)
  add_library(nlohmann_json INTERFACE)
  target_include_directories(nlohmann_json INTERFACE ${NLOHMANN_JSON_INCLUDE_DIR})
  add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
endif()

include(GNUInstallDirs)
include(cmake/HellVersion.cmake)
include(cmake/HellMacros.cmake)

add_subdirectory(Core)
add_subdirectory(DataFormats)
add_subdirectory(Modules)
add_subdirectory(Hist)
add_subdirectory(Fit)
add_subdirectory(Plot)
add_subdirectory(bin)

message(STATUS "[HELL] Version ${HELL_VERSION} (${HELL_GIT_HASH}${HELL_GIT_DIRTY})")
message(STATUS "[HELL] ROOT ${ROOT_VERSION}, C++${CMAKE_CXX_STANDARD}, build type ${CMAKE_BUILD_TYPE}")
