# hell_add_library(Name SOURCES ... DEPENDS ...)   -> lib/libHell<Name>.so
# hell_add_plugin(Name SOURCES ... DEPENDS ...)    -> lib/plugins/libHell<Name>Plugin.so (dlopen'ed at run time)
# hell_add_executable(name SOURCES ... DEPENDS ...)-> bin/<name>
# hell_add_dictionary(Name LINKDEF ... HEADERS ...) -> ROOT dictionary attached to library Hell<Name>

function(hell_add_library NAME)
  cmake_parse_arguments(ARG "" "" "SOURCES;DEPENDS" ${ARGN})
  add_library(Hell${NAME} SHARED ${ARG_SOURCES})
  target_include_directories(Hell${NAME} PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/generated)
  target_link_libraries(Hell${NAME} PUBLIC ROOT::Core ROOT::RIO ROOT::Tree ROOT::TreePlayer ROOT::Hist ROOT::MathCore ROOT::Physics ROOT::GenVector nlohmann_json::nlohmann_json ${ARG_DEPENDS})
  set_target_properties(Hell${NAME} PROPERTIES OUTPUT_NAME "Hell${NAME}")
endfunction()

function(hell_add_plugin NAME)
  cmake_parse_arguments(ARG "" "" "SOURCES;DEPENDS" ${ARGN})
  add_library(Hell${NAME}Plugin MODULE ${ARG_SOURCES})
  target_include_directories(Hell${NAME}Plugin PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/generated)
  target_link_libraries(Hell${NAME}Plugin PUBLIC HellCore HellDataFormats ${ARG_DEPENDS})
  set_target_properties(Hell${NAME}Plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/plugins"
    PREFIX "lib")
endfunction()

function(hell_add_executable NAME)
  cmake_parse_arguments(ARG "" "" "SOURCES;DEPENDS" ${ARGN})
  add_executable(${NAME} ${ARG_SOURCES})
  target_include_directories(${NAME} PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/generated)
  target_link_libraries(${NAME} PUBLIC HellCore ${CMAKE_DL_LIBS} ${ARG_DEPENDS})
endfunction()

function(hell_add_dictionary NAME)
  cmake_parse_arguments(ARG "" "" "HEADERS;LINKDEF" ${ARGN})
  root_generate_dictionary(G__Hell${NAME} ${ARG_HEADERS} MODULE Hell${NAME} LINKDEF ${CMAKE_SOURCE_DIR}/${ARG_LINKDEF} NOINSTALL)
endfunction()
