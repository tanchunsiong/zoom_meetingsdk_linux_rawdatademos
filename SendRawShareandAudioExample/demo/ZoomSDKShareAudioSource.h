#pragma once

#include <atomic>
#include <string>

#include "rawdata/rawdata_share_source_helper_interface.h"

using namespace ZOOMSDK;

class ZoomSDKShareAudioSource : public IZoomSDKShareAudioSource
{
public:
	explicit ZoomSDKShareAudioSource(std::string audio_source);

	void onStartSendAudio(IZoomSDKShareAudioSender* sender) override;
	void onStopSendAudio() override;

private:
	IZoomSDKShareAudioSender* sender_ = nullptr;
	std::string audio_source_;
	std::atomic<bool> sending_{false};
};
