# Captures the code version at configure time so that it can be written
# into every output file (provenance). Re-run cmake to refresh it.
find_package(Git QUIET)
set(HELL_GIT_HASH "unknown")
set(HELL_GIT_DIRTY "")
set(HELL_GIT_BRANCH "unknown")
set(HELL_VERSION "0.1.0")
if(GIT_FOUND)
  execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short=12 HEAD
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  OUTPUT_VARIABLE _hash OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE _rc)
  if(_rc EQUAL 0 AND NOT "${_hash}" STREQUAL "")
    set(HELL_GIT_HASH "${_hash}")
  endif()
  execute_process(COMMAND ${GIT_EXECUTABLE} status --porcelain --untracked-files=no
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  OUTPUT_VARIABLE _dirty OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  if(NOT "${_dirty}" STREQUAL "")
    set(HELL_GIT_DIRTY "-dirty")
  endif()
  execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  OUTPUT_VARIABLE _branch OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE _rc2)
  if(_rc2 EQUAL 0 AND NOT "${_branch}" STREQUAL "")
    set(HELL_GIT_BRANCH "${_branch}")
  endif()
  execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --always
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  OUTPUT_VARIABLE _desc OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE _rc3)
  if(_rc3 EQUAL 0 AND NOT "${_desc}" STREQUAL "")
    set(HELL_VERSION "${_desc}")
  endif()
endif()
string(TIMESTAMP HELL_BUILD_TIME "%Y-%m-%dT%H:%M:%S")
configure_file(${CMAKE_SOURCE_DIR}/Core/interface/Version.h.in ${CMAKE_BINARY_DIR}/generated/Core/interface/Version.h @ONLY)
