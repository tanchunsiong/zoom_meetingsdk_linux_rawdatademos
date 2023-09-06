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
    libxcb-xtest0 \
    libdbus-1-3 \
    libglib2.0-0 \
    libgbm1 \
    libxfixes3 \
    libgl1 \
    libdrm2 \
    libgssapi-krb5-2 \
    openssl \
    ca-certificates \
    pkg-config \
    libegl-mesa0 \
    libsdl2-dev \
    g++-multilib 

apt-get install -y gtkmm-3.0

# if you are getting error about <SDL2/SDL.h>
apt-get install libegl-mesa0 libsdl2-dev g++-multilib

# for curl related calls
apt-get libcurl4-openssl-dev

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


To install PulseAudio on Ubuntu and configure it to emulate a virtual output device, you can follow these steps:

Install PulseAudio:

PulseAudio is usually pre-installed on Ubuntu, but you can ensure it's up-to-date by running:


sudo apt update
sudo apt install pulseaudio
Install pavucontrol (PulseAudio Volume Control):

pavucontrol is a graphical tool that makes it easier to manage PulseAudio devices. Install it using:


sudo apt install pavucontrol
Load the module-null-sink module:

PulseAudio can create a virtual audio sink (output device) using the module-null-sink. To load this module, open a terminal and run:


pactl load-module module-null-sink sink_name=VirtualOutput sink_properties=device.description="Virtual Output"
This command creates a virtual output device named "Virtual Output."

Set the default sink:

Now, you need to set the default audio sink to the virtual output device. You can do this using pavucontrol:

Launch pavucontrol by running pavucontrol in the terminal or searching for it in your application launcher.

In the "Playback" tab, you should see a list of applications that are currently producing audio. For each application, you can select the output device. Set the desired applications to use the "Virtual Output."

Additionally, in the "Configuration" tab, you can set the "Virtual Output" as the default sink for all audio.

Close pavucontrol after making these changes.