# Every subdirectory with a CMakeLists.txt is a plugin package.
file(GLOB _pkgs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
foreach(_p ${_pkgs})
  if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${_p} AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${_p}/CMakeLists.txt)
    add_subdirectory(${_p})
  endif()
endforeach()
