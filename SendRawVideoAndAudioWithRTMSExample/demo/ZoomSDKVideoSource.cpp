#include "ZoomSDKVideoSource.h"
#include <iostream>
#include <chrono>
#include <fstream>


// FrameBuffer Implementation
void FrameBuffer::push(const uint8_t* data, size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    //std::cout << "📥 FrameBuffer::push() — Pushing frame of size: " << size << " bytes\n";
    queue_.emplace(data, data + size);
    cv_.notify_one();
}

bool FrameBuffer::pop(std::vector<uint8_t>& frame) {
    std::unique_lock<std::mutex> lock(mutex_);
    //std::cout << "⏳ FrameBuffer::pop() — Waiting for frame...\n";
    cv_.wait(lock, [this] { return !queue_.empty() || stopped_; });

    if (stopped_ && queue_.empty()) {
        std::cout << "⚠️ FrameBuffer::pop() — Buffer stopped, exiting\n";
        return false;
    }

    frame = std::move(queue_.front());
    queue_.pop();

    //std::cout << "📦 FrameBuffer::pop() — Got frame of size: " << frame.size() << " bytes\n";

    // // 💾 Append frame to shared output file
    // static std::ofstream outfile("output_debug.yuv", std::ios::binary | std::ios::app);
    // if (outfile.is_open()) {
    //     outfile.write(reinterpret_cast<char*>(frame.data()), frame.size());
    //     std::cout << "💾 Appended frame to output_debug.yuv (total bytes written: " << frame.size() << ")\n";
    // } else {
    //     std::cerr << "❌ Failed to open output_debug.yuv for writing\n";
    // }

    return true;
}

void FrameBuffer::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << "🛑 FrameBuffer::stop() — Stopping frame loop\n";
    stopped_ = true;
    cv_.notify_all();
}

// ZoomSDKVideoSource Implementation
ZoomSDKVideoSource::ZoomSDKVideoSource() {
    std::cout << "📦 ZoomSDKVideoSource constructed\n";
}

void ZoomSDKVideoSource::onInitialize(IZoomSDKVideoSender* sender, IList<VideoSourceCapability>* support_cap_list, VideoSourceCapability& suggest_cap) {
    std::cout << "🟢 ZoomSDKVideoSource::onInitialize — Sender initialized\n";
    video_sender_ = sender;
}

void ZoomSDKVideoSource::onPropertyChange(IList<VideoSourceCapability>* support_cap_list, VideoSourceCapability suggest_cap) {
    std::cout << "📥 onPropertyChange called\n";

    if (support_cap_list) {
        int count = support_cap_list->GetCount();
        std::cout << "📋 Zoom supports " << count << " video capability(ies):\n";

        for (int i = 0; i < count; ++i) {
            VideoSourceCapability cap = support_cap_list->GetItem(i);
            std::cout << "  [" << i << "] "
                      << cap.width << "x" << cap.height
                      << " @ " << cap.frame << " fps\n";
        }
    } else {
        std::cout << "⚠️ No support_cap_list provided\n";
    }

    // ✅ You can still override HD:
    width_ = 1280;
    height_ = 720;
    std::cout << "✅ Forcing 1280x720 resolution\n";
}

void ZoomSDKVideoSource::onStartSend() {
    std::cout << "🟢 ZoomSDKVideoSource::onStartSend\n";

    if (!video_sender_) {
        std::cerr << "❌ ZoomSDKVideoSource::onStartSend — video_sender_ is null\n";
        return;
    }

    if (running_) {
        std::cout << "⚠️ ZoomSDKVideoSource::onStartSend — Already running\n";
        return;
    }

    running_ = true;
    std::cout << "🚀 Starting sender thread...\n";
    sender_thread_ = std::thread(&ZoomSDKVideoSource::sendFrames, this);
}

void ZoomSDKVideoSource::onStopSend() {
    std::cout << "🛑 ZoomSDKVideoSource::onStopSend\n";
    running_ = false;
    frame_buffer_.stop();

    if (sender_thread_.joinable()) {
        std::cout << "🧵 Joining sender thread...\n";
        sender_thread_.join();
    }
}

void ZoomSDKVideoSource::onUninitialized() {
    std::cout << "❎ ZoomSDKVideoSource::onUninitialized — Releasing sender\n";
    video_sender_ = nullptr;
}

FrameBuffer& ZoomSDKVideoSource::getFrameBuffer() {
    return frame_buffer_;
}

void ZoomSDKVideoSource::sendFrames() {
    std::cout << "🚀 ZoomSDKVideoSource::sendFrames — Thread started\n";
    const int frame_len = width_ * height_ * 3 / 2;
    std::vector<uint8_t> frame;

    while (running_) {
        if (!frame_buffer_.pop(frame)) {
            std::cout << "🛑 ZoomSDKVideoSource::sendFrames — Exiting (pop returned false)\n";
            break;
        }

 
// std::cout << "📤 Sending frame of size: " << frame.size()
//           << " | Resolution: " << width_ << "x" << height_
//           << " | Expected size: " << width_ * height_ * 3 / 2 << "\n";
        SDKError err = video_sender_->sendVideoFrame(
            reinterpret_cast<char*>(frame.data()),
            width_,
            height_,
            frame_len,
            0,
            FrameDataFormat_I420_FULL
        );

        if (err == SDKERR_SUCCESS) {
            //std::cout << "✅ Frame sent successfully\n";
        } else {
            std::cerr << "❌ Failed to send frame — SDK error: " << err << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }

    std::cout << "📴 ZoomSDKVideoSource::sendFrames — Thread finished\n";
}
