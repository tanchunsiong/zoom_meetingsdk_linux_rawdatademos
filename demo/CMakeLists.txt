cmake_minimum_required(VERSION 3.16)

project(meetingSDKDemo CXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
add_definitions(-std=c++14)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

find_package(PkgConfig REQUIRED)
#no longer needed for headless demo
#pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
#include_directories(${GTKMM_INCLUDE_DIRS})
#link_directories(${GTKMM_LIBRARY_DIRS})
#add_definitions(${GTKMM_CFLAGS_OTHER})
find_package(ZLIB REQUIRED)



# Find FFmpeg

pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavfilter
    libavdevice
    libavutil
    libswresample
    libswscale

    # Add other FFmpeg components as needed
)


include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/h)
#include_directories(${CMAKE_SOURCE_DIR}/include/ffmpeg)

include_directories(/usr/include/glib-2.0/)
include_directories(/usr/include/glib-2.0/glib)
include_directories(/usr/lib/x86_64-linux-gnu/glib-2.0/include/)
include_directories(/usr/lib64/glib-2.0/include/)
# Add FFmpeg include directories
# include_directories(${FFMPEG_INCLUDE_DIRS})


link_directories(${CMAKE_SOURCE_DIR}/lib/zoom_meeting_sdk)
link_directories(${CMAKE_SOURCE_DIR}/lib/zoom_meeting_sdk/qt_libs)
link_directories(${CMAKE_SOURCE_DIR}/lib/zoom_meeting_sdk/qt_libs/Qt/lib)


add_executable(meetingSDKDemo 
              ${CMAKE_SOURCE_DIR}/meeting_sdk_demo.cpp
              ${CMAKE_SOURCE_DIR}/MeetingChatEventListener.h 
              ${CMAKE_SOURCE_DIR}/MeetingChatEventListener.cpp
              ${CMAKE_SOURCE_DIR}/MeetingReminderEventListener.h
              ${CMAKE_SOURCE_DIR}/MeetingReminderEventListener.cpp
              ${CMAKE_SOURCE_DIR}/MeetingServiceEventListener.h
              ${CMAKE_SOURCE_DIR}/MeetingServiceEventListener.cpp
              ${CMAKE_SOURCE_DIR}/AuthServiceEventListener.h
              ${CMAKE_SOURCE_DIR}/AuthServiceEventListener.cpp
              ${CMAKE_SOURCE_DIR}/MeetingParticipantsCtrlEventListener.h
              ${CMAKE_SOURCE_DIR}/MeetingParticipantsCtrlEventListener.cpp
              ${CMAKE_SOURCE_DIR}/MeetingRecordingCtrlEventListener.h
              ${CMAKE_SOURCE_DIR}/MeetingRecordingCtrlEventListener.cpp
              ${CMAKE_SOURCE_DIR}/ZoomSDKRenderer.h
              ${CMAKE_SOURCE_DIR}/ZoomSDKRenderer.cpp
              ${CMAKE_SOURCE_DIR}/ZoomSDKAudioRawData.h
              ${CMAKE_SOURCE_DIR}/ZoomSDKAudioRawData.cpp
              ${CMAKE_SOURCE_DIR}/ZoomSDKRawDataPipeDelegate.h
              ${CMAKE_SOURCE_DIR}/ZoomSDKRawDataPipeDelegate.cpp
              ${CMAKE_SOURCE_DIR}/ZoomSDKVideoSource.h 
              ${CMAKE_SOURCE_DIR}/ZoomSDKVideoSource.cpp
              ${CMAKE_SOURCE_DIR}/ZoomSDKVirtualAudioMicEvent.h 
              ${CMAKE_SOURCE_DIR}/ZoomSDKVirtualAudioMicEvent.cpp
              )

target_link_libraries(meetingSDKDemo gcc_s gcc)
target_link_libraries(meetingSDKDemo meetingsdk)
target_link_libraries(meetingSDKDemo glib-2.0)
#no longer needed for headless demo
#target_link_libraries(meetingSDKDemo ${GTKMM_LIBRARIES})
target_link_libraries(meetingSDKDemo curl)
target_link_libraries(meetingSDKDemo pthread)

#target_link_libraries(meetingSDKDemo z avcodec avformat avutil)
#target_link_libraries(meetingSDKDemo lzma swresample swscale avfilter)
#link_directories(${CMAKE_SOURCE_DIR}/lib/ffmpeg)
target_link_libraries(meetingSDKDemo PkgConfig::FFMPEG)

#target_link_libraries(meetingSDKDemo ZLIB::ZLIB)

target_include_directories(meetingSDKDemo PRIVATE ${FFMPEG_INCLUDE_DIRS})

# Create a symbolic link
execute_process(COMMAND ln -s libmeetingsdk.so libmeetingsdk.so.1
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/lib/zoom_meeting_sdk
)

configure_file(${CMAKE_SOURCE_DIR}/config.json ${CMAKE_SOURCE_DIR}/bin/config.json COPYONLY)

file(COPY ${CMAKE_SOURCE_DIR}/lib/zoom_meeting_sdk/ DESTINATION ${CMAKE_SOURCE_DIR}/bin)
# //SendVideoRawData
file(COPY ${CMAKE_SOURCE_DIR}/Big_Buck_Bunny_720_10s_10MB.mp4 DESTINATION ${CMAKE_SOURCE_DIR}/bin/)

# //SendAudioRawData
file(COPY ${CMAKE_SOURCE_DIR}/Big_Buck_Bunny.wav DESTINATION ${CMAKE_SOURCE_DIR}/bin/)