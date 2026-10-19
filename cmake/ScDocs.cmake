# Doxygen API documentation target (AGENTS.md: doxygen → docs build target).
# Public headers of the engine, the psx library, and the test framework are
# documented (Doxygen is mandated on all public project-owned C++ APIs).

function(sc_add_doxygen_target)
  find_package(Doxygen QUIET)
  if(NOT Doxygen_FOUND)
    message(STATUS "Doxygen not found — sc-api-docs target not created (install doxygen to enable)")
    return()
  endif()

  set(SC_DOXYGEN_CONFIG_FILE ${CMAKE_BINARY_DIR}/Doxyfile)
  # Note: every Doxyfile tag on its own line; all INPUT paths on one line
  # (Doxygen does not support wrapped/indented continuations).
  file(WRITE ${SC_DOXYGEN_CONFIG_FILE}
"PROJECT_NAME = \"Severed Chains Native\"
PROJECT_NUMBER = \"${PROJECT_VERSION}\"
OUTPUT_DIRECTORY = ${CMAKE_BINARY_DIR}/docs
RECURSIVE = YES
INPUT = \"${CMAKE_CURRENT_SOURCE_DIR}/src/engine\" \"${CMAKE_CURRENT_SOURCE_DIR}/psx/include\" \"${CMAKE_CURRENT_SOURCE_DIR}/tests/framework\"
GENERATE_HTML = YES
HTML_OUTPUT = api
GENERATE_LATEX = NO
WARNINGS = YES
QUIET = YES
")

  add_custom_target(sc-api-docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${SC_DOXYGEN_CONFIG_FILE}
    COMMENT "Generating Doxygen API documentation (public headers) -> ${CMAKE_BINARY_DIR}/docs/api"
    VERBATIM)
endfunction()
