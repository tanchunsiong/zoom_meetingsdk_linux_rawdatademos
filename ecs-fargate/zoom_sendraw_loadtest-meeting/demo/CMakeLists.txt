cmake_minimum_required(VERSION 3.16)

project(ZoomSendRawLoadTestMeetingDemo CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/ZoomMeetingSdk.cmake)

set(COMMON_SOURCES
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
)

zoom_meeting_sdk_add_demo(
  TARGET ZoomSendRawLoadTestJoinMeetingDemo
  USE_FFMPEG
  SOURCES
    meeting_sdk_demo.cpp
    ${COMMON_SOURCES}
)

zoom_meeting_sdk_add_demo(
  TARGET ZoomSendRawLoadTestStartMeetingDemo
  USE_FFMPEG
  SOURCES
    start_meeting_sdk_demo.cpp
    ${COMMON_SOURCES}
)

# Both targets share demo/bin as their runtime directory. Build them in order so
# the SDK support directories are not copied concurrently during parallel builds.
add_dependencies(ZoomSendRawLoadTestStartMeetingDemo ZoomSendRawLoadTestJoinMeetingDemo)
