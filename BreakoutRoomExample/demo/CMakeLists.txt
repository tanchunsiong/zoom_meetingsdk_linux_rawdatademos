cmake_minimum_required(VERSION 3.16)
project(BreakoutRoomDemo LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/ZoomMeetingSdk.cmake)

zoom_meeting_sdk_add_demo(
  TARGET BreakoutRoomDemo
  SOURCES src/main.cpp
)
