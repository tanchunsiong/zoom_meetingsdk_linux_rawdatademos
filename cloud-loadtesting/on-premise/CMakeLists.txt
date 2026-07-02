cmake_minimum_required(VERSION 3.16)

project(ZoomLoadTestingOnPremise LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

get_filename_component(_repo_root "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(
  ZOOM_MEETING_SDK_DIR
  "${_repo_root}/sdk"
  CACHE PATH
  "Path to repo-local sdk/ or an extracted Zoom Meeting SDK directory"
)

add_subdirectory(zoom_sendraw_loadtest-meeting/demo)
