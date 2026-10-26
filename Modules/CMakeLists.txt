# Every subdirectory with a CMakeLists.txt is a plugin package. New packages are found by the next
# `hell build` (CONFIGURE_DEPENDS re-globs at build time).
file(GLOB _pkgs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*/CMakeLists.txt)
foreach(_cm ${_pkgs})
  get_filename_component(_p ${_cm} DIRECTORY)
  add_subdirectory(${_p})
endforeach()
