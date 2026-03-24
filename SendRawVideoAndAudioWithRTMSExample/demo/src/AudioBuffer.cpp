#include "AudioBuffer.h"

void AudioBuffer::push(const uint8_t* data, size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.emplace(data, data + size);
    cv_.notify_one();
}

bool AudioBuffer::pop(std::vector<uint8_t>& frame) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this]() { return !queue_.empty() || stopped_; });

    if (stopped_ && queue_.empty()) return false;

    frame = std::move(queue_.front());
    queue_.pop();
    return true;
}

void AudioBuffer::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    stopped_ = true;
    cv_.notify_all();
}
