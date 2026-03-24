// SendAudioRawData (from memory, not file)
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <queue>

#include "ZoomSDKVirtualAudioMicEvent.h"
#include "rawdata/rawdata_audio_helper_interface.h"
#include "zoom_sdk_def.h"

using namespace std;
using namespace ZOOM_SDK_NAMESPACE;

int audio_play_flag = -1;

// ========== AudioBuffer Implementation ==========
void AudioBuffer::push(const uint8_t* data, size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.emplace(data, data + size);
    cv_.notify_one();
}

bool AudioBuffer::pop(std::vector<uint8_t>& out) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !queue_.empty() || stopped_; });

    if (stopped_ && queue_.empty())
        return false;

    out = std::move(queue_.front());
    queue_.pop();
    return true;
}

void AudioBuffer::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    stopped_ = true;
    cv_.notify_all();
}

AudioBuffer& ZoomSDKVirtualAudioMicEvent::getAudioBuffer() {
    return audio_buffer_;
}

// ========== Audio Sending Thread ==========
void ZoomSDKVirtualAudioMicEvent::StreamPCMToVirtualMic(IZoomSDKAudioRawDataSender * audio_sender) {
    std::vector<uint8_t> frame;

    while (audio_play_flag > 0 && audio_sender) {
        if (!audio_buffer_.pop(frame)) {
            break;
        }

        SDKError err = audio_sender->send(reinterpret_cast<char*>(frame.data()), frame.size(), 16000);
        if (err != SDKERR_SUCCESS) {
            std::cerr << "❌ Failed to send audio: " << err << "\n";
        } else {
            std::cout << "🔊 Audio frame sent (" << frame.size() << " bytes)\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(20)); // ~50 FPS @ 16000hz mono 16-bit
    }
}

// ========== SDK Callbacks ==========
void ZoomSDKVirtualAudioMicEvent::onMicInitialize(IZoomSDKAudioRawDataSender* pSender) {
    pSender_ = pSender;
    std::cout << "🟢 ZoomSDKVirtualAudioMicEvent::onMicInitialize\n";
}

void ZoomSDKVirtualAudioMicEvent::onMicStartSend() {
    std::cout << "🟢 ZoomSDKVirtualAudioMicEvent::onMicStartSend\n";
    if (pSender_ && audio_play_flag != 1) {
        audio_play_flag = 1;
        std::thread(&ZoomSDKVirtualAudioMicEvent::StreamPCMToVirtualMic, this, pSender_).detach();
    }
}

void ZoomSDKVirtualAudioMicEvent::onMicStopSend() {
    std::cout << "🛑 ZoomSDKVirtualAudioMicEvent::onMicStopSend\n";
    audio_play_flag = 0;
    audio_buffer_.stop();
}

void ZoomSDKVirtualAudioMicEvent::onMicUninitialized() {
    std::cout << "❎ ZoomSDKVirtualAudioMicEvent::onMicUninitialized\n";
    pSender_ = nullptr;
}

ZoomSDKVirtualAudioMicEvent::ZoomSDKVirtualAudioMicEvent() {}
   
