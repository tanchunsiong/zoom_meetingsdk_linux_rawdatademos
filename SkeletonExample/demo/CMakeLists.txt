cmake_minimum_required(VERSION 3.16)

project(SkeletonDemo CXX)
set(TARGET_NAME SkeletonDemo)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
add_definitions(-std=c++11)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

find_package(PkgConfig REQUIRED)
#no longer needed for headless demo
#pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
#include_directories(${GTKMM_INCLUDE_DIRS})
#link_directories(${GTKMM_LIBRARY_DIRS})
#add_definitions(${GTKMM_CFLAGS_OTHER})


pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GIO REQUIRED gio-2.0)

find_package(ZLIB REQUIRED)





include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/h)


include_directories(/usr/include/glib-2.0/)
include_directories(/usr/include/glib-2.0/glib)
include_directories(/usr/lib/x86_64-linux-gnu/glib-2.0/include/)
include_directories(/usr/lib64/glib-2.0/include/)


include_directories(${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS})


link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/zoom_meeting_sdk)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/zoom_meeting_sdk/qt_libs)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/zoom_meeting_sdk/qt_libs/Qt/lib)
link_directories(${GLIB_LIBRARY_DIRS} ${GIO_LIBRARY_DIRS})


add_definitions(${GLIB_CFLAGS_OTHER} ${GIO_CFLAGS_OTHER})

add_executable(${TARGET_NAME} 
             ${CMAKE_CURRENT_SOURCE_DIR}/meeting_sdk_demo.cpp
              ${CMAKE_CURRENT_SOURCE_DIR}/MeetingReminderEventListener.h
              ${CMAKE_CURRENT_SOURCE_DIR}/MeetingReminderEventListener.cpp
              ${CMAKE_CURRENT_SOURCE_DIR}/MeetingServiceEventListener.h
              ${CMAKE_CURRENT_SOURCE_DIR}/MeetingServiceEventListener.cpp
              ${CMAKE_CURRENT_SOURCE_DIR}/AuthServiceEventListener.h
              ${CMAKE_CURRENT_SOURCE_DIR}/AuthServiceEventListener.cpp
              ${CMAKE_CURRENT_SOURCE_DIR}/MeetingParticipantsCtrlEventListener.h
              ${CMAKE_CURRENT_SOURCE_DIR}/MeetingParticipantsCtrlEventListener.cpp
              )




target_link_libraries(${TARGET_NAME} gcc_s gcc)
target_link_libraries(${TARGET_NAME} meetingsdk)
target_link_libraries(${TARGET_NAME} glib-2.0)


target_link_libraries(${TARGET_NAME} curl)
target_link_libraries(${TARGET_NAME} pthread)



target_link_libraries(${TARGET_NAME}  ${GLIB_LIBRARIES} ${GIO_LIBRARIES})



# Create a symbolic link
execute_process(COMMAND ln -s libmeetingsdk.so libmeetingsdk.so.1
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/zoom_meeting_sdk
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.json ${CMAKE_CURRENT_SOURCE_DIR}/bin/config.json COPYONLY)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/zoom_meeting_sdk/ DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# //SendVideoRawData
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Big_Buck_Bunny_720_10s_10MB.mp4 DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/)

# //SendAudioRawData
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Big_Buck_Bunny.wav DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/)