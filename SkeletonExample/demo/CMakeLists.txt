cmake_minimum_required(VERSION 3.16)

project(SkeletonDemo CXX)
set(TARGET_NAME SkeletonDemo)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/ZoomMeetingSdk.cmake)

zoom_meeting_sdk_add_demo(
  TARGET ${TARGET_NAME}
  SOURCES
    meeting_sdk_demo.cpp
    MeetingReminderEventListener.h
    MeetingReminderEventListener.cpp
    MeetingServiceEventListener.h
    MeetingServiceEventListener.cpp
    AuthServiceEventListener.h
    AuthServiceEventListener.cpp
    MeetingParticipantsCtrlEventListener.h
    MeetingParticipantsCtrlEventListener.cpp
  COPY_FILES
    Big_Buck_Bunny_720_10s_10MB.mp4
    Big_Buck_Bunny.wav
)
