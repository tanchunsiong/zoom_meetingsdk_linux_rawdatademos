# Preparing development / docker environment on different distributions

==============================================================

## Centos

This is tested on WSL centos 9 (works better on centos 8 or ubuntu)

In case you wish to run this on Windows Subsystem for Linux, here are some additional dependencies to get it to run
on centos 9 for WSL2. Some of these commands / packages might be redundant, and has not been optimized.


### install files for compiling
sudo yum install cmake
sudo yum install gcc gcc-c++

### First, enable the CodeReady Linux Builder repository. You already have access to it; you just need to enable it.
sudo dnf config-manager --set-enabled crb
#Next, install the EPEL RPM.
sudo dnf install epel-release epel-next-release

### install GTK related packages (no longer needed for headless demo)
#sudo yum install gtk3-devel 
#sudo yum install gtkmm30
#sudo yum install gtkmm30-devel

### If you encounter: Fatal error: SDL2/SDL.h: No such file or directory. This is no longer in used at code level, but leaving this here for legacy support purposes
sudo yum -y install SDL2-devel

### If you encounter these error Messages
#/usr/bin/ld: warning: libxcb-image.so.0, needed by /root/release_demo/demo/libmeetingsdk.so, not found (try using -rpath or -rpath-link)
#/usr/bin/ld: warning: libxcb-keysyms.so.1, needed by /root/release_demo/demo/libmeetingsdk.so, not found (try using -rpath or -rpath-link)

sudo yum install libxcb-devel
sudo yum install xcb-util-devel #might be useless
sudo yum install xcb-util-image
sudo yum install xcb-util-keysyms


### If you encounter these runtime runtime error
#libGL error: MESA-LOADER: failed to open swrast: /usr/lib64/dri/swrast_dri.so: cannot open shared object file: No such file or directory (search paths /usr/lib64/dri, suffix _dri)
sudo yum install mesa-libGL
sudo yum install mesa-libGL-devel
sudo yum install mesa-dri-drivers

### for curl related calls (this does not work at runtime for centos 9 due to dependencies on openssl 1.1.1)
  yum install -y openssl-devel 
  yum install -y libcurl-devel 

# Install Pulseaudio
yum install -y  pulseaudio pulseaudio-utils 

# Install ffmpeg
yum install -y libavformat-dev libavfilter-dev libavdevice-dev ffmpeg




==============================================================

## Ubuntu 22


### Install necessary dependencies
apt-get update 

apt-get install -y build-essential cmake

apt-get install -y --no-install-recommends --no-install-suggests \
    libx11-xcb1 \
    libxcb-xfixes0 \
    libxcb-shape0 \
    libxcb-shm0 \
    libxcb-randr0 \
    libxcb-randr0 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-xtest0 
 
 ### optional libraries
 apt-get install -y --no-install-recommends --no-install-suggests \
    libdbus-1-3 \
    libglib2.0-0 \
    libgbm1 \
    libxfixes3 \
    libgl1 \
    libdrm2 \
    libgssapi-krb5-2 


# No longer needed for headless demo
apt-get install -y gtkmm-3.0

### if you are getting error about <SDL2/SDL.h>
 apt-get install libegl-mesa0 libsdl2-dev g++-multilib

### for curl related calls
apt-get install libcurl4-openssl-dev \
    openssl \
    ca-certificates \
    pkg-config 

### for pulseaudio related
# Install ALSA
apt-get install -y libasound2 libasound2-plugins alsa alsa-utils alsa-oss

# Install Pulseaudio
apt-get install -y  pulseaudio pulseaudio-utils 

# Install ffmpeg
apt-get install -y libavformat-dev libavfilter-dev libavdevice-dev ffmpeg




  ======================================================================

# Getting Started
## Steps to start running

1 Please decompress the compressed package of zoom-meeting-sdk-linux_x86_64-5.xx.x.xxxx.tar and copy the files in the decompressed folder to these folders

  `h` to `demo/include/h`
  `qt_libs` to `demo/lib/qt_libs`
  4 of the `lib******.so` files to `demo/lib/lib******.so`
  softlink  `libmeetingsdk.so` to `libmeetingsdk.so.1` within demo/lib/
  (`ln -s libmeetingsdk.so libmeetingsdk.so.1`)

2 Execute `cmake -B build` in demo/ folder . After successful generation, the demo/build/ folder will be generated.

3 Execute `make` in the /demo/build directory

## Notes for using the demo:
1. The auth token, password, meeting number etc.. needs to be written in the config.json file. There is an advanced option to set `useJWTTokenFromWebService = true;`. This will call the remote_url (found on config.json) endpoint to get a JWT Token. This endpoint is based on this https://github.com/zoom/meetingsdk-auth-endpoint-sample
2. You need to fill in `withoutloginParam.userName = "";`  in `meeting_sdk_demo.cpp`
3. Only after init sdk and auth are successfully executed, then you can joinmeeting and leavemeeting
4. Subscribing to raw audio and raw video requires successful join meeting.
5. You might need a file named `~\.config.us\zoomus.conf` before you can access raw audio. The sample zoomus.conf file is provided in this project.

## additional cpp and .h files might need to be included in this section of CMakeLists.txt

```
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
```
## additional libraries which you are going to add, might need to be included in this section of CMakeLists.txt
```
target_link_libraries(meetingSDKDemo gcc_s gcc)
target_link_libraries(meetingSDKDemo meetingsdk)
target_link_libraries(meetingSDKDemo glibcd -2.0)
target_link_libraries(meetingSDKDemo ${GTKMM_LIBRARIES})
target_link_libraries(meetingSDKDemo curl)
target_link_libraries(meetingSDKDemo pthread)
```

# Get Raw Video Data

# Get Raw Audio Data

# Send Raw Video Data

Send Raw Video data sample breaks down an mp4 file using ffmpeg, and sends them to the meeting as a webcam stream.
You will need ffmpeg installed for this. We have included a creative commons video Big_Buck_Bunny_720_10s_1MB.mp4 for this purpose

Ensure cmake is refering this ffmpeg too

It uses the file ZoomSDKVideoSource.cpp and h

# Send Audio Video Data

this reads from whitenoise.wav 

It uses the file ZoomSDKVirtualAudioMicEvent.cpp and h

## addition requirement for raw audio data in docker
need to have a file in ~/.config/zoomus.conf

[General]
system.audio.type=default

# Docker Containers

## Dockerfiles

There is a seperate `readme for docker.md`
Dockerfile targetting different distros are found in their own folder (Dockerfile-centos, Dockerfile-Ubuntu .....)
Currently this is tested on 
- Centos 9 (functionality where custom function of fetching JWT Token from Web Service does not work, main SDK function works fully)
- Centos 8
- Ubuntu 22
- dorowu/ubuntu-desktop-lxde-vnc:focal

## pulse audio is configured using scripts

The setup is done via `setup-pulseaudio.sh` and `setup-pulssaudio-centos.sh`, this need to be run prior to running this project in a docker environment.
The script starts the pulseaudio service, creates a virtual speaker, a virtual microphone, and a zoomus.conf file in the docker environment.

## Segmentation fault error
In the scenario where you encounter segmentation error right after calling authsdk, it might be possible this is caused by newer version of OPENSSL. 
I've not investigated this in details, but here are some ways to fix it (warning, not a secure fix) in development



cd /tmp



get dependencies
sudo apt install libssl-dev liblzo2-dev libpam0g-dev
way, instead of trying to solve this new issue, I simply downloaded the Impish binary instead of trying to build it from scratch:


download binary openssl packages from Impish builds
wget http://security.ubuntu.com/ubuntu/pool/main/o/openssl/openssl_1.1.1f-1ubuntu2.20_amd64.deb |
wget http://security.ubuntu.com/ubuntu/pool/main/o/openssl/libssl-dev_1.1.1f-1ubuntu2.20_amd64.deb |
wget http://security.ubuntu.com/ubuntu/pool/main/o/openssl/libssl1.1_1.1.1f-1ubuntu2.20_amd64.deb


install downloaded binary packages
 dpkg -i libssl1.1_1.1.1f-1ubuntu2.20_amd64.deb |
 dpkg -i libssl-dev_1.1.1f-1ubuntu2.20_amd64.deb |
 dpkg -i openssl_1.1.1f-1ubuntu2.20_amd64.deb

## References : Additional libraries which might be used


send video and send audio requires manual chat to turn it on









sudo apt-get install  -y dbus \
 fontconfig               \
 libglib2.0               \
 libdrm-dev                   \
 libsm-dev                \
 libx11-dev               \
 libxcb1-dev              \
 libxcomposite-dev        \
 libxcursor-dev           \
 libxfixes-dev            \
 libxi-dev                \
 libxkbcommon-x11-dev     \
 libxrandr-dev            \
 libxrender-dev           \
 libxshmfence-dev         \
 libxslt1-dev             \
 libxtst-dev              \
 mesa-utils               \
 libnss3-dev              \
 ibus                     \
 picom               \
 libqt5webengine5     \
 xcompmgr




WIP

do i need libav series??
sudo apt update
sudo apt-get install libavformat-dev libavfilter-dev libavdevice-dev ffmpeg


# gstreamer

apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio