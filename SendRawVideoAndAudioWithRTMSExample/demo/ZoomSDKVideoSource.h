//SendVideoRawData


#pragma once

#include "ZoomSDKVideoSource.h"
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>

#include <string>
#include "rawdata/rawdata_video_source_helper_interface.h"

using namespace std;
using namespace ZOOMSDK;


#include "FrameBuffer.h"



class ZoomSDKVideoSource : public IZoomSDKVideoSource {
public:
    ZoomSDKVideoSource();


    void onInitialize(IZoomSDKVideoSender* sender, IList<VideoSourceCapability>* support_cap_list, VideoSourceCapability& suggest_cap) override;
    void onPropertyChange(IList<VideoSourceCapability>* support_cap_list, VideoSourceCapability suggest_cap) override;
    void onStartSend() override;
    void onStopSend() override;
    void onUninitialized() override;

    FrameBuffer& getFrameBuffer();

private:
    FrameBuffer frame_buffer_;

    void sendFrames();
    std::thread sender_thread_;
    IZoomSDKVideoSender* video_sender_ = nullptr;
    int width_ = 1280;
    int height_ = 720;
    bool running_ = false;


};

