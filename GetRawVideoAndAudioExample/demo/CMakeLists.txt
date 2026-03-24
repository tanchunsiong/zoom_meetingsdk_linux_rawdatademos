cmake_minimum_required(VERSION 3.16)

project(GetRawVideoAndAudioDemo CXX)
set(TARGET_NAME GetRawVideoAndAudioDemo)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/ZoomMeetingSdk.cmake)

zoom_meeting_sdk_add_demo(
  TARGET ${TARGET_NAME}
  USE_FFMPEG
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
    MeetingRecordingCtrlEventListener.h
    MeetingRecordingCtrlEventListener.cpp
    ZoomSDKRawDataPipeDelegate.h
    ZoomSDKRawDataPipeDelegate.cpp
    MeetingShareCtrlEventListener.h
    MeetingShareCtrlEventListener.cpp
)
