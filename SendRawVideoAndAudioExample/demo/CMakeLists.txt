cmake_minimum_required(VERSION 3.16)

project(SendRawVideoAndAudioDemo CXX)
set(TARGET_NAME SendRawVideoAndAudioDemo)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/ZoomMeetingSdk.cmake)

zoom_meeting_sdk_add_demo(
  TARGET ${TARGET_NAME}
  USE_FFMPEG
  SOURCES
    meeting_sdk_demo.cpp
    MeetingChatEventListener.h
    MeetingChatEventListener.cpp
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
    ZoomSDKVideoSource.h
    ZoomSDKVideoSource.cpp
    ZoomSDKVirtualAudioMicEvent.h
    ZoomSDKVirtualAudioMicEvent.cpp
  COPY_FILES
    Big_Buck_Bunny_720_10s_10MB.mp4
    Big_Buck_Bunny.wav
    Big_Buck_Bunny2.wav
    pcm1644m.wav
)
