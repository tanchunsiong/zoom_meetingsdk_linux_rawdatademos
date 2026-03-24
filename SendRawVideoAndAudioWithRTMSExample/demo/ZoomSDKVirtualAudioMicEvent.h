//SendAudioRawData
#pragma once
#include <iostream>
#include <cstdint>
#include "rawdata/rawdata_audio_helper_interface.h"
#include "zoom_sdk.h"
#include "zoom_sdk_raw_data_def.h"

#include "AudioBuffer.h"

using namespace std;
using namespace ZOOMSDK;

class ZoomSDKVirtualAudioMicEvent :
	public IZoomSDKVirtualAudioMicEvent
{

private:
	IZoomSDKAudioRawDataSender* pSender_;
	std::string audio_source_;
protected:

	
	virtual void onMicInitialize(IZoomSDKAudioRawDataSender* pSender);
	virtual void onMicStartSend();
	virtual void onMicStopSend();
	virtual void onMicUninitialized();

public:
	ZoomSDKVirtualAudioMicEvent();

 	AudioBuffer& getAudioBuffer();

 private:
	 void StreamPCMToVirtualMic(IZoomSDKAudioRawDataSender* audio_sender);
    AudioBuffer audio_buffer_;

};