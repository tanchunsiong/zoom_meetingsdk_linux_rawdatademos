cmake_minimum_required(VERSION 3.16)

project(SendRawVideoAndAudioWithRTMSDemo CXX)
set(TARGET_NAME SendRawVideoAndAudioWithRTMSDemo)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/ZoomMeetingSdk.cmake)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

zoom_meeting_sdk_add_demo(
  TARGET ${TARGET_NAME}
  USE_FFMPEG
  COPY_ENV_FILE
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
    src/utils.cpp
    src/media_ws.cpp
    src/signaling_ws.cpp
    src/oauth.cpp
  EXTRA_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/external/websocketpp"
  EXTRA_LIBRARIES
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
)
