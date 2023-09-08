==============================================================

This is tested on WSL centos 9 (works better on centos 8 or ubuntu)

In case you wish to run this on Windows Subsystem for Linux, here are some additional dependencies to get it to run
on centos 9 for WSL2. Some of these commands might be redundant, and has not been optimized.


#install files for compiling
sudo yum install cmake
sudo yum install gcc gcc-c++

#First, enable the CodeReady Linux Builder repository. You already have access to it; you just need to enable it.
sudo dnf config-manager --set-enabled crb
#Next, install the EPEL RPM.
sudo dnf install epel-release epel-next-release

#install GTK related packages
sudo yum install gtk3-devel 
sudo yum install gtkmm30
sudo yum install gtkmm30-devel

#If you encounter: Fatal error: SDL2/SDL.h: No such file or directory
sudo yum -y install SDL2-devel

#If you encounter these error Messages
#/usr/bin/ld: warning: libxcb-image.so.0, needed by /root/release_demo/demo/libmeetingsdk.so, not found (try using -rpath or -rpath-link)
#/usr/bin/ld: warning: libxcb-keysyms.so.1, needed by /root/release_demo/demo/libmeetingsdk.so, not found (try using -rpath or -rpath-link)

sudo yum install libxcb-devel
sudo yum install xcb-util-devel #might be useless
sudo yum install xcb-util-image
sudo yum install xcb-util-keysyms


#If you encounter these runtime runtime error
#libGL error: MESA-LOADER: failed to open swrast: /usr/lib64/dri/swrast_dri.so: cannot open shared object file: No such file or directory (search paths /usr/lib64/dri, suffix _dri)
sudo yum install mesa-libGL
sudo yum install mesa-libGL-devel
sudo yum install mesa-dri-drivers

# for curl related calls (this does not work at runtime for centos 9 due to dependencies on openssl 1.1.1)
  yum install -y openssl-devel 
  yum install -y libcurl-devel 

==============================================================

For Ubuntu you may try the below


# Install necessary dependencies
apt-get update && \
    apt-get install -y build-essential cmake

apt-get update && apt-get install -y --no-install-recommends --no-install-suggests \
    libx11-xcb1 \
    libxcb-xfixes0 \
    libxcb-shape0 \
    libxcb-shm0 \
    libxcb-randr0 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-xtest0 
 
 # optional
 apt-get install -y --no-install-recommends --no-install-suggests \
    libdbus-1-3 \
    libglib2.0-0 \
    libgbm1 \
    libxfixes3 \
    libgl1 \
    libdrm2 \
    libgssapi-krb5-2 \


apt-get install -y gtkmm-3.0

# if you are getting error about <SDL2/SDL.h>, not necessary anymore
# apt-get install libegl-mesa0 libsdl2-dev g++-multilib

# for curl related calls
apt-get install libcurl4-openssl-dev \
    openssl \
    ca-certificates \
    pkg-config 

  ======================================================================

Steps to start running

1 Please decompress the compressed package of zoom-meeting-sdk-linux_x86_64-5.xx.x.xxxx.tar and copy the files in the decompressed folder to these folders

  `h` to `demo/include/h`
  `qt_libs` to `demo/lib/qt_libs`
  4 of the `lib******.so` files to `demo/lib/lib******.so`
  softlink  `libmeetingsdk.so` to `libmeetingsdk.so.1` within demo/lib/
  (`ln -s libmeetingsdk.so libmeetingsdk.so.1`)

2 Execute `cmake -B build` in demo/ folder . After successful generation, the demo/build/ folder will be generated.

3 Execute `make` in the /demo/build directory

Notes for using the demo:
1 The auth token, password, meeting number etc.. needs to be written in the config.json file. remote_url calls the endpoint to get a JWT Token. This endpoint is based on this https://github.com/zoom/meetingsdk-auth-endpoint-sample
2 You need to fill in withoutloginParam.userName = "";  in meeting_sdk_demo.cpp
3 Only after init sdk and auth are executed, then you can joinmeeting and leavemeeting
4 login needs auth to succeed first, then click gen_token, fill in the token in the input box and click login to start the meeting
5 subscribe needs to join meeting first and getuserid and fill userId into the input box and then click the subscribe button

# additional cpp and .h files might need to be included in this section of CMakeLists.txt

add_executable(meetingSDKDemo 
              ${CMAKE_SOURCE_DIR}/meeting_sdk_demo.cpp
              ${CMAKE_SOURCE_DIR}/init_auth_sdk_workflow.cpp
              ${CMAKE_SOURCE_DIR}/meeting_sdk_util.cpp
              ${CMAKE_SOURCE_DIR}/init_auth_sdk_workflow.h
              ${CMAKE_SOURCE_DIR}/meeting_sdk_util.h
              ${CMAKE_SOURCE_DIR}/RegressionTestRawdataRender.cpp
              ${CMAKE_SOURCE_DIR}/RegressionTestRawdataRender.h
              ${CMAKE_SOURCE_DIR}/custom_ui_eventSink.h
              ${CMAKE_SOURCE_DIR}/custom_ui_eventSink.cpp
              ${CMAKE_SOURCE_DIR}/MeetingReminderEventListener.h
              ${CMAKE_SOURCE_DIR}/MeetingReminderEventListener.cpp
              )
# additional libraries which you are going to add, might need to be included in this section of CMakeLists.txt

target_link_libraries(meetingSDKDemo gcc_s gcc)
target_link_libraries(meetingSDKDemo meetingsdk)
target_link_libraries(meetingSDKDemo glib-2.0)
target_link_libraries(meetingSDKDemo ${GTKMM_LIBRARIES})
target_link_libraries(meetingSDKDemo curl)
target_link_libraries(meetingSDKDemo pthread)

## Recording

This needs recording token to work, without recording token, even when given recording permissions, you will not be able to record

## Get Audio Raw Data

might need to install pulse audio and these
sudo apt-get install pulseaudio jackd2 alsa-utils dbus-x11


# To install PulseAudio on Ubuntu WSL as root and configure it to emulate a virtual output device, you can follow these steps:
Install PulseAudio (if not already installed):

apt-get install pulseaudio
apt-get install pulseaudio-utils

yum install pulseaudio
yum install pulseaudio-utils


You can check if PulseAudio is installed by running:


pulseaudio --version



Load Module-Null-Sink (Virtual Sink):

PulseAudio provides a module called module-null-sink, which allows you to create virtual sinks (output devices). To load this module and create a virtual speaker:

pactl load-module module-null-sink sink_name=Virtual_Speaker


To make the virtual speaker the default audio sink, use the following command:


pacmd set-default-sink Virtual_Speaker


Similarly, you can create a virtual microphone by loading the module-null-sink module with a different name:



To set the virtual microphone as the default audio source (input device), run:

#audiotype does not have aa name a type

add this into the include #include "meeting_audio_interface.h"