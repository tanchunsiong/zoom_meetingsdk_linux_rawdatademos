if(NOT DEFINED CPTHOST OR NOT EXISTS "${CPTHOST}")
  message(FATAL_ERROR "CPTHOST must identify the copied cpthost executable")
endif()

execute_process(
  COMMAND readelf -d "${CPTHOST}"
  OUTPUT_VARIABLE _dynamic_section
  RESULT_VARIABLE _readelf_result
)
if(NOT _readelf_result EQUAL 0)
  message(FATAL_ERROR "Unable to inspect cpthost RUNPATH")
endif()

string(
  REGEX MATCH
  "\\(RUNPATH\\)[^\n]*\\[([^]]*)\\]"
  _runpath_match
  "${_dynamic_section}"
)
if(NOT _runpath_match)
  message(FATAL_ERROR "Unable to find cpthost RUNPATH")
endif()

set(_old_runpath "${CMAKE_MATCH_1}")
file(
  RPATH_CHANGE
  FILE "${CPTHOST}"
  OLD_RPATH "${_old_runpath}"
  NEW_RPATH "\$ORIGIN:\$ORIGIN/qt_libs:\$ORIGIN/qt_libs/Qt/lib"
)
