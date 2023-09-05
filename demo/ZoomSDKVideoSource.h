#pragma once
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <glib.h>
#include <gtkmm.h>
#include <sstream>
#include <thread>
#include <sys/syscall.h>
#include <fstream>
#include <iosfwd>
#include <iostream>
#include <pthread.h>
#include <SDL2/SDL.h>
#include "rawdata/rawdata_video_source_helper_interface.h"
#include "rawdata/rawdata_audio_helper_interface.h"

constexpr auto WIDTH = 640;
constexpr auto HEIGHT = 480;

using namespace std;
using namespace ZOOMSDK;

class ZoomSDKVideoSource :
	public IZoomSDKVideoSource
{
private:
	IZoomSDKVideoSender* video_sender_;
	std::string video_source_;
protected:
	virtual	void onInitialize(IZoomSDKVideoSender* sender, IList<VideoSourceCapability >* support_cap_list, VideoSourceCapability& suggest_cap);
	virtual void onPropertyChange(IList<VideoSourceCapability >* support_cap_list, VideoSourceCapability suggest_cap);
	virtual void onStartSend();
	virtual void onStopSend();
	virtual void onUninitialized();
public:
	ZoomSDKVideoSource(std::string video_source);
};

