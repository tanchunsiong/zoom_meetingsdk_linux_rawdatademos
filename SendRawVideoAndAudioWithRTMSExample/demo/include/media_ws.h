#pragma once
#include <string>
#include <websocketpp/connection.hpp>

#include "../ZoomSDKVideoSource.h"
#include "../ZoomSDKVirtualAudioMicEvent.h"


void connect_to_media_server(
    const std::string& media_url,
    const std::string& meeting_uuid,
    const std::string& stream_id,
    websocketpp::connection_hdl signaling_hdl
);

void set_video_source_reference(ZoomSDKVideoSource* ptr);

void set_audio_source_reference(ZoomSDKVirtualAudioMicEvent* ptr);