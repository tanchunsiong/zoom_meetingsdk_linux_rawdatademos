==============================================================

This is tested on WSL centos 9 

In case you wish to run this on Windows Subsystem for Linux, here are some additional dependencies to get it to run
on centos 9 for WSL2. Some of these commands might be redundant 


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

#f you encounter : Error Messages
#/usr/bin/ld: warning: libxcb-image.so.0, needed by /root/release_demo/demo/libmeetingsdk.so, not found (try using -rpath or -rpath-link)
#/usr/bin/ld: warning: libxcb-keysyms.so.1, needed by /root/release_demo/demo/libmeetingsdk.so, not found (try using -rpath or -rpath-link)

sudo yum install libxcb-devel
sudo yum install xcb-util-devel #useless
sudo yum install xcb-util-image
sudo yum install xcb-util-keysyms


#If you encounter these runtime runtime error
#libGL error: MESA-LOADER: failed to open swrast: /usr/lib64/dri/swrast_dri.so: cannot open shared object file: No such file or directory (search paths /usr/lib64/dri, suffix _dri)
sudo yum install mesa-libGL
sudo yum install mesa-libGL-devel
sudo yum install mesa-dri-drivers

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

# missing #include <SDL2/SDL.h>
apt-get install libegl-mesa0 libsdl2-dev g++-multilib



  ======================================================================

  Steps to start running

1 Please decompress the compressed package of zoom-meeting-sdk-linux_x86_64-5.xx.x.xxxx.tar and copy the files in the decompressed folder to the same level directory as the demo/ compressed package

2 Execute ./build_linuxmeetingsdk_demo.sh . after successful generation, the demo will be generated in the same level directory of the demo/ compressed package

Notes for using the demo:
1 The auth token needs to be written in advance in param.jwt_token="" in the void AuthMeetingSDK(Gtk::TextView* text_view) method in meeting_sdk_demo/meeting_sdk_demo.cpp
2 You need to fill in normalParam.userName = ""; and normalParam.psw = ""; the meeting name and password you want to join in the cpp of the demo;
3 After init sdk and auth are executed, you can joinmeeting and leavemeeting
4 login needs auth to succeed first, then click gen_token, fill in the token in the input box and click login to start the meeting
5 subscribe needs to join meeting first and getuserid and fill userId into the input box and then click the subscribe button

