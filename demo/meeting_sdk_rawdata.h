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
#include <SDL2/SDL.h>

#include "zoom_sdk.h"
#include "auth_service_interface.h"
#include "meeting_sdk_util.h"
#include "init_auth_sdk_workflow.h"
#include "RegressionTestRawdataRender.h"
#include "RegressionTestRawdataDemo.h"
#include "rawdata/rawdata_audio_helper_interface.h"
#include "rawdata/rawdata_video_source_helper_interface.h"

USING_ZOOM_SDK_NAMESPACE

#pragma once


class MeetingsdkRawDataUI:
	                                     public ZOOM_SDK_NAMESPACE::IZoomSDKAudioRawDataDelegate,
	                                     public ZOOM_SDK_NAMESPACE::IZoomSDKVideoSource,
	                                     public ZOOM_SDK_NAMESPACE::IZoomSDKVirtualAudioMicEvent
{

public:
	MeetingsdkRawDataUI();
	~MeetingsdkRawDataUI();

public:
Gtk::TextView* text_view;

static std::string ConvertResolutionToString(const ZOOM_SDK_NAMESPACE::ZoomSDKResolution& resolution)
{
	std::string sResolution;

	switch (resolution)
	{
	case ZOOM_SDK_NAMESPACE::ZoomSDKResolution_90P:
		sResolution = "ZoomSDKResolution_90P";
		break;
	case ZOOM_SDK_NAMESPACE::ZoomSDKResolution_180P:
		sResolution = "ZoomSDKResolution_180P";
		break;
	case ZOOM_SDK_NAMESPACE::ZoomSDKResolution_360P:
		sResolution = "ZoomSDKResolution_360P";
		break;
	case ZOOM_SDK_NAMESPACE::ZoomSDKResolution_720P:
		sResolution = "ZoomSDKResolution_720P";
		break;
	case ZOOM_SDK_NAMESPACE::ZoomSDKResolution_1080P:
		sResolution = "ZoomSDKResolution_1080P";
		break;
	case ZOOM_SDK_NAMESPACE::ZoomSDKResolution_NoUse:
		sResolution = "ZoomSDKResolution_NoUse";
		break;
	default:
		break;
	}

	return sResolution;
}
static std::string ConvertRawDataTypeToString(const ZOOM_SDK_NAMESPACE::ZoomSDKRawDataType& type)
{
	std::string sRawDataType;

	switch (type)
	{
	case ZOOM_SDK_NAMESPACE::RAW_DATA_TYPE_VIDEO:
		sRawDataType = "raw_data_type_video";
		break;
	case ZOOM_SDK_NAMESPACE::RAW_DATA_TYPE_SHARE:
		sRawDataType = "raw_data_type_share";
		break;
	default:
		break;
	}

	return sRawDataType;
}
};