cmake_minimum_required(VERSION 3.14)

project(meetingSDKDemo CXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
add_definitions(-std=c++14)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)


include_directories(${GTKMM_INCLUDE_DIRS})
link_directories(${GTKMM_LIBRARY_DIRS})
add_definitions(${GTKMM_CFLAGS_OTHER})


include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/h)
include_directories(/usr/include/glib-2.0/)
include_directories(/usr/include/glib-2.0/glib)
include_directories(/usr/lib/x86_64-linux-gnu/glib-2.0/include/)

link_directories(${CMAKE_SOURCE_DIR}/lib/zoom_meeting_sdk)
link_directories(${CMAKE_SOURCE_DIR}/lib/zoom_meeting_sdk/qt_libs)
link_directories(${CMAKE_SOURCE_DIR}/lib/zoom_meeting_sdk/qt_libs/Qt/lib)

add_executable(meetingSDKDemo 
              ${CMAKE_SOURCE_DIR}/meeting_sdk_demo.cpp
              ${CMAKE_SOURCE_DIR}/MeetingReminderEventListener.h
              ${CMAKE_SOURCE_DIR}/MeetingReminderEventListener.cpp
              ${CMAKE_SOURCE_DIR}/MeetingServiceEventListener.h
              ${CMAKE_SOURCE_DIR}/MeetingServiceEventListener.cpp
              ${CMAKE_SOURCE_DIR}/AuthServiceEventListener.h
              ${CMAKE_SOURCE_DIR}/AuthServiceEventListener.cpp
              ${CMAKE_SOURCE_DIR}/ZoomSDKRenderer.h
              ${CMAKE_SOURCE_DIR}/ZoomSDKRenderer.cpp
              )

target_link_libraries(meetingSDKDemo gcc_s gcc)
target_link_libraries(meetingSDKDemo meetingsdk)
target_link_libraries(meetingSDKDemo glib-2.0)
target_link_libraries(meetingSDKDemo ${GTKMM_LIBRARIES})
target_link_libraries(meetingSDKDemo curl)
target_link_libraries(meetingSDKDemo pthread)

# Create a symbolic link
execute_process(COMMAND ln -s libmeetingsdk.so libmeetingsdk.so.1
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/lib/zoom_meeting_sdk
)


configure_file(${CMAKE_SOURCE_DIR}/config.json ${CMAKE_SOURCE_DIR}/bin/config.json COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/cacert.pem ${CMAKE_SOURCE_DIR}/bin/cacert.pem COPYONLY)
file(COPY ${CMAKE_SOURCE_DIR}/lib/zoom_meeting_sdk/ DESTINATION ${CMAKE_SOURCE_DIR}/bin)
