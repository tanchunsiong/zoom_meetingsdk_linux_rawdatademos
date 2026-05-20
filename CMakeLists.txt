cmake_minimum_required(VERSION 3.16)

project(MasterProject LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

add_subdirectory(AllInOneExample/demo)
add_subdirectory(ChatExample/demo)
add_subdirectory(GetRawVideoAndAudioExample/demo)
add_subdirectory(GetRawVideoAndAudioAPIExample/demo)
add_subdirectory(SendRawVideoAndAudioExample/demo)
add_subdirectory(SendRawVideoAndAudioWithRTMSExample/demo)
add_subdirectory(SkeletonExample/demo)
add_subdirectory(zoom_sendraw_loadtest/demo)
