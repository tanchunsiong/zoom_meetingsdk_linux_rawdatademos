# MeetingSDK Demo

This is a simple demo application that interacts with the Zoom SDK to perform various actions related to Zoom meetings. The application is written in C++ and uses the GTKmm library for the graphical user interface.

## Features

- **Init SDK:** Initialize the Zoom SDK for further usage.
- **Authenticate:** Authenticate the Zoom SDK using JWT token or a stored token. (Note 1)
- **Join Meeting:** Join a Zoom meeting using meeting number, token, and password. (Note 2)
- **Leave Meeting:** Leave the ongoing Zoom meeting.
- **Start Meeting:** Start a new Zoom meeting.
- **Login:** Log in to the Zoom SDK using a token. (Note 4)
- **Get User ID:** Retrieve and display the user IDs of participants in the meeting. (Note 5)
- **Mute/Unmute Video:** Toggle video mute/unmute in an ongoing meeting.
- **Mute/Unmute Audio:** Toggle audio mute/unmute for a specified user ID in an ongoing meeting.
- **Subscribe/Unsubscribe Video:** Subscribe/unsubscribe to raw video data for a specific user ID. (Note 5)

## Notes for Using the Demo

1. The authentication token needs to be pre-filled in the `param.jwt_token` field in the `void AuthMeetingSDK(Gtk::TextView* text_view)` method in `meeting_sdk_demo/meeting_sdk_demo.cpp`.
2. You need to fill in `normalParam.userName = "";` and `normalParam.psw = "";` with the meeting name and password you want to join in the code of the demo.
3. After initializing the SDK and successfully authenticating, you can use the `joinmeeting` and `leavemeeting` functionalities.
4. The `login` feature requires successful authentication first. Click `gen_token`, enter the generated token in the input box, and then click `login` to start the meeting.
5. To use the `subscribe` feature, you need to join a meeting first, retrieve the user ID using `getuserid`, and then enter the user ID into the input box before clicking the `subscribe` button.


## Dependencies

- `limits.h`, `stdio.h`, `string.h`, `stdlib.h`, `unistd.h`: Standard C libraries.
- `glib.h`: Glib library for utility functions.
- `gtkmm.h`: C++ wrapper for GTK+ graphical toolkit.
- `sstream`, `thread`, `sys/syscall.h`, `fstream`, `iosfwd`, `iostream`: Standard C++ libraries.
- `SDL2/SDL.h`: Simple DirectMedia Layer for multimedia handling.
- `zoom_sdk.h`, `auth_service_interface.h`, `meeting_sdk_util.h`, `init_auth_sdk_workflow.h`, `RegressionTestRawdataRender.h`, `meeting_sdk_rawdata_audio.h`: Zoom SDK header files.
- `MeetingReminderEventListener.h`: Custom event listener for meeting reminders.
- `json.hpp`: JSON library for parsing configuration settings.

## Usage

1. Compile the application with the necessary dependencies and include paths for GTKmm, Zoom SDK, and other libraries.
2. Run the compiled executable.
3. Use the graphical user interface to perform different actions related to Zoom meetings.

### Additional Dependencies for Windows Subsystem for Linux (WSL) on CentOS 9

In case you wish to run this on Windows Subsystem for Linux, here are some additional dependencies to get it to run on CentOS 9 for WSL2. Some of these commands might be redundant.

```bash
# Install files for compiling
sudo yum install cmake
sudo yum install gcc gcc-c++

# Enable the CodeReady Linux Builder repository
sudo dnf config-manager --set-enabled crb

# Install the EPEL RPM
sudo dnf install epel-release epel-next-release

# Install GTK related packages
sudo yum install gtk3-devel
sudo yum install gtkmm30
sudo yum install gtkmm30-devel

# If you encounter: Fatal error: SDL2/SDL.h: No such file or directory
sudo yum -y install SDL2-devel

# If you encounter: libGL error: MESA-LOADER: failed to open swrast: ...
sudo yum install mesa-libGL
sudo yum install mesa-libGL-devel
sudo yum install mesa-dri-drivers

# Install xcb libraries
sudo yum install libxcb-devel
sudo yum install xcb-util-image
sudo yum install xcb-util-keysyms

### Dependencies for Windows Subsystem for Linux (WSL) on Ubuntu

# Install required libraries
apt-get update \
  && apt-get install -y --no-install-recommends --no-install-suggests \
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
  ca-certificates

# Install GTKmm library
apt-get update
apt-get install gtkmm-3.0

# Install SDL2 and related libraries
sudo apt-get install libegl-mesa0 libsdl2-dev g++-multilib


## Configuration

Before running the application, you need to provide the necessary configuration settings in the `config.json` file. The configuration includes meeting details, authentication tokens, and other settings. The application reads these settings during startup.

## Building

Steps to start running

1.  Please decompress the compressed package of zoom-meeting-sdk-linux_x86_64-5.xx.x.xxxx.tar and copy the files in the decompressed folder 
2.  From `h` to `demo\include\h`
3.  From `qt_libs` to `demo\lib\zoom_meeting_sdk\qt_libs`
4.  From `libfdkaac2.so` to `demo\lib\zoom_meeting_sdk\libfdkaac2.so`
5.  From`libmeetingsdk.so` to `demo\lib\zoom_meeting_sdk\libmeetingsdk.so`
6.  From`libmpg123.so` to `demo\lib\zoom_meeting_sdk\libmpg123.so`
7.  From `libturbojpeg.so` to `demo\lib\zoom_meeting_sdk\libturbojpeg.so`
8.  Create a softlink (linux `ln` command) from  `demo\lib\zoom_meeting_sdk\libmeetingsdk.so` to  `demo\lib\zoom_meeting_sdk\libmeetingsdk.so.1`
`ln -s libmeetingsdk.so.1 libmeetingsdk.so`

To build the application run the below commands

`cmake -B build` in the demo\ folder
`make` in the demo\build folder

## Running

`./meetingSDKDemo` in demo\bin folder

## Disclaimer

This application is intended as a simple demonstration of integrating with the Zoom SDK. It may require further enhancements and error handling to be used in a production environment.

## License

This project is licensed under the [MIT License](LICENSE).