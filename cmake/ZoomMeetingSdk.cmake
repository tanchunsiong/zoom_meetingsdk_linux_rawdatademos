include(CMakeParseArguments)

get_filename_component(_zoom_meeting_sdk_repo_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(_zoom_meeting_sdk_cmake_dir "${CMAKE_CURRENT_LIST_DIR}")

set(
  ZOOM_MEETING_SDK_DIR
  "${_zoom_meeting_sdk_repo_root}/sdk"
  CACHE PATH
  "Path to repo-local sdk/ or an extracted Zoom Meeting SDK directory"
)

function(_zoom_meeting_sdk_try_layout candidate out_var)
  if(
    EXISTS "${candidate}/h/zoom_sdk.h"
    AND EXISTS "${candidate}/libmeetingsdk.so"
    AND EXISTS "${candidate}/libcml.so"
    AND EXISTS "${candidate}/libmpg123.so"
    AND EXISTS "${candidate}/json/translations.json"
    AND EXISTS "${candidate}/qt_libs"
  )
    set(${out_var} "${candidate}" PARENT_SCOPE)
  else()
    set(${out_var} "" PARENT_SCOPE)
  endif()
endfunction()

function(zoom_meeting_sdk_resolve_root)
  set(_resolved_sdk_root "")
  _zoom_meeting_sdk_try_layout("${ZOOM_MEETING_SDK_DIR}" _resolved_sdk_root)

  if(NOT _resolved_sdk_root AND EXISTS "${ZOOM_MEETING_SDK_DIR}")
    file(GLOB _sdk_children LIST_DIRECTORIES true "${ZOOM_MEETING_SDK_DIR}/*")
    foreach(_sdk_child IN LISTS _sdk_children)
      if(IS_DIRECTORY "${_sdk_child}")
        _zoom_meeting_sdk_try_layout("${_sdk_child}" _resolved_sdk_root)
        if(_resolved_sdk_root)
          break()
        endif()
      endif()
    endforeach()
  endif()

  if(NOT _resolved_sdk_root)
    message(
      FATAL_ERROR
      "Unable to find a valid Zoom Meeting SDK under '${ZOOM_MEETING_SDK_DIR}'.\n"
      "Expected either:\n"
      "  1. repo/sdk containing the extracted SDK files directly, or\n"
      "  2. repo/sdk containing a single extracted SDK subdirectory.\n"
      "Override with -DZOOM_MEETING_SDK_DIR=/path/to/sdk if needed."
    )
  endif()

  set(ZOOM_MEETING_SDK_ROOT "${_resolved_sdk_root}" PARENT_SCOPE)
  set(ZOOM_MEETING_SDK_INCLUDE_DIR "${_resolved_sdk_root}/h" PARENT_SCOPE)
  set(ZOOM_MEETING_SDK_LIBRARY "${_resolved_sdk_root}/libmeetingsdk.so" PARENT_SCOPE)
  set(ZOOM_MEETING_SDK_CML_LIBRARY "${_resolved_sdk_root}/libcml.so" PARENT_SCOPE)
  set(ZOOM_MEETING_SDK_MPG123_LIBRARY "${_resolved_sdk_root}/libmpg123.so" PARENT_SCOPE)
  set(ZOOM_MEETING_SDK_QT_LIB_DIR "${_resolved_sdk_root}/qt_libs/Qt/lib" PARENT_SCOPE)
endfunction()

function(zoom_meeting_sdk_configure_target target_name)
  zoom_meeting_sdk_resolve_root()

  get_target_property(_zoom_meeting_sdk_runtime_dir "${target_name}" RUNTIME_OUTPUT_DIRECTORY)
  if(NOT _zoom_meeting_sdk_runtime_dir)
    set(_zoom_meeting_sdk_runtime_dir "${CMAKE_CURRENT_BINARY_DIR}")
  endif()

  set(_zoom_meeting_sdk_env_script "${_zoom_meeting_sdk_runtime_dir}/env.sh")
  set(_zoom_meeting_sdk_run_script "${_zoom_meeting_sdk_runtime_dir}/run_${target_name}.sh")
  set(_zoom_meeting_sdk_cpthost_script "${_zoom_meeting_sdk_runtime_dir}/run_cpthost.sh")

  set(ZOOM_MEETING_SDK_LAUNCH_TARGET "${target_name}")
  configure_file(
    "${_zoom_meeting_sdk_cmake_dir}/ZoomMeetingSdkEnv.sh.in"
    "${_zoom_meeting_sdk_env_script}"
    @ONLY
  )
  configure_file(
    "${_zoom_meeting_sdk_cmake_dir}/ZoomMeetingSdkRun.sh.in"
    "${_zoom_meeting_sdk_run_script}"
    @ONLY
  )
  set(ZOOM_MEETING_SDK_LAUNCH_TARGET "cpthost")
  configure_file(
    "${_zoom_meeting_sdk_cmake_dir}/ZoomMeetingSdkRun.sh.in"
    "${_zoom_meeting_sdk_cpthost_script}"
    @ONLY
  )

  target_include_directories(${target_name} PRIVATE "${ZOOM_MEETING_SDK_INCLUDE_DIR}")
  target_link_directories(
    ${target_name}
    PRIVATE
      "${ZOOM_MEETING_SDK_ROOT}"
      "${ZOOM_MEETING_SDK_ROOT}/qt_libs"
      "${ZOOM_MEETING_SDK_QT_LIB_DIR}"
  )
  target_link_libraries(
    ${target_name}
    PRIVATE
      "${ZOOM_MEETING_SDK_LIBRARY}"
  )

  set_target_properties(
    ${target_name}
    PROPERTIES
      BUILD_RPATH "$ORIGIN;$ORIGIN/qt_libs;$ORIGIN/qt_libs/Qt/lib"
  )

  add_custom_command(
    TARGET ${target_name}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${ZOOM_MEETING_SDK_LIBRARY}"
      "$<TARGET_FILE_DIR:${target_name}>/libmeetingsdk.so"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${ZOOM_MEETING_SDK_CML_LIBRARY}"
      "$<TARGET_FILE_DIR:${target_name}>/libcml.so"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${ZOOM_MEETING_SDK_MPG123_LIBRARY}"
      "$<TARGET_FILE_DIR:${target_name}>/libmpg123.so"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${ZOOM_MEETING_SDK_ROOT}/qt_libs"
      "$<TARGET_FILE_DIR:${target_name}>/qt_libs"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${ZOOM_MEETING_SDK_ROOT}/json"
      "$<TARGET_FILE_DIR:${target_name}>/json"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${ZOOM_MEETING_SDK_ROOT}/images"
      "$<TARGET_FILE_DIR:${target_name}>/images"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${ZOOM_MEETING_SDK_ROOT}/cpthost"
      "$<TARGET_FILE_DIR:${target_name}>/cpthost"
    COMMAND ${CMAKE_COMMAND} -E create_symlink
      libmeetingsdk.so
      "$<TARGET_FILE_DIR:${target_name}>/libmeetingsdk.so.1"
    COMMAND /bin/chmod +x
      "${_zoom_meeting_sdk_env_script}"
      "${_zoom_meeting_sdk_run_script}"
      "${_zoom_meeting_sdk_cpthost_script}"
  )
endfunction()

function(zoom_meeting_sdk_add_demo)
  set(options USE_FFMPEG COPY_ENV_FILE)
  set(oneValueArgs TARGET)
  set(multiValueArgs SOURCES EXTRA_INCLUDE_DIRS EXTRA_LIBRARIES COPY_FILES)
  cmake_parse_arguments(ZMSDK "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT ZMSDK_TARGET)
    message(FATAL_ERROR "zoom_meeting_sdk_add_demo requires TARGET")
  endif()

  if(NOT ZMSDK_SOURCES)
    message(FATAL_ERROR "zoom_meeting_sdk_add_demo requires SOURCES for target '${ZMSDK_TARGET}'")
  endif()

  find_package(PkgConfig REQUIRED)
  find_package(CURL REQUIRED)
  find_package(Threads REQUIRED)
  pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
  pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)

  if(ZMSDK_USE_FFMPEG)
    pkg_check_modules(
      FFMPEG REQUIRED IMPORTED_TARGET
      libavcodec
      libavformat
      libavfilter
      libavdevice
      libavutil
      libswresample
      libswscale
    )
  endif()

  add_executable(${ZMSDK_TARGET} ${ZMSDK_SOURCES})

  set_target_properties(
    ${ZMSDK_TARGET}
    PROPERTIES
      CXX_STANDARD 11
      CXX_STANDARD_REQUIRED YES
      CXX_EXTENSIONS NO
      ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
  )

  target_include_directories(
    ${ZMSDK_TARGET}
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/include"
      ${ZMSDK_EXTRA_INCLUDE_DIRS}
  )

  target_link_libraries(
    ${ZMSDK_TARGET}
    PRIVATE
      gcc_s
      gcc
      CURL::libcurl
      Threads::Threads
      PkgConfig::GLIB
      PkgConfig::GIO
      ${ZMSDK_EXTRA_LIBRARIES}
  )

  if(ZMSDK_USE_FFMPEG)
    target_link_libraries(${ZMSDK_TARGET} PRIVATE PkgConfig::FFMPEG)
  endif()

  zoom_meeting_sdk_configure_target(${ZMSDK_TARGET})

  set(_zoom_meeting_sdk_config_source "${CMAKE_CURRENT_SOURCE_DIR}/config.json")
  if(
    NOT EXISTS "${_zoom_meeting_sdk_config_source}"
    AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config.json.example"
  )
    set(_zoom_meeting_sdk_config_source "${CMAKE_CURRENT_SOURCE_DIR}/config.json.example")
  endif()
  if(EXISTS "${_zoom_meeting_sdk_config_source}")
    configure_file(
      "${_zoom_meeting_sdk_config_source}"
      "${CMAKE_CURRENT_SOURCE_DIR}/bin/config.json"
      COPYONLY
    )
  endif()

  if(ZMSDK_COPY_ENV_FILE)
    set(_zoom_meeting_sdk_env_source "${CMAKE_CURRENT_SOURCE_DIR}/.env")
    if(
      NOT EXISTS "${_zoom_meeting_sdk_env_source}"
      AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.env.example"
    )
      set(_zoom_meeting_sdk_env_source "${CMAKE_CURRENT_SOURCE_DIR}/.env.example")
    endif()
    if(EXISTS "${_zoom_meeting_sdk_env_source}")
      add_custom_command(
        TARGET ${ZMSDK_TARGET}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${_zoom_meeting_sdk_env_source}"
          "$<TARGET_FILE_DIR:${ZMSDK_TARGET}>/.env"
      )
    endif()
  endif()

  foreach(_zoom_meeting_sdk_copy_file IN LISTS ZMSDK_COPY_FILES)
    add_custom_command(
      TARGET ${ZMSDK_TARGET}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/${_zoom_meeting_sdk_copy_file}"
        "$<TARGET_FILE_DIR:${ZMSDK_TARGET}>/${_zoom_meeting_sdk_copy_file}"
    )
  endforeach()
endfunction()
