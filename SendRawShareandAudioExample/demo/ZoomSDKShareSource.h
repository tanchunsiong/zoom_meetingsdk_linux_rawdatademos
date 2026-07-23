#pragma once

#include <atomic>
#include <string>

#include "rawdata/rawdata_share_source_helper_interface.h"

using namespace ZOOMSDK;

class ZoomSDKShareSource : public IZoomSDKShareSource
{
public:
	explicit ZoomSDKShareSource(std::string video_source);

	void onStartSend(IZoomSDKShareSender* sender) override;
	void onStopSend() override;

private:
	IZoomSDKShareSender* sender_ = nullptr;
	std::string video_source_;
	std::atomic<bool> sending_{false};
};
