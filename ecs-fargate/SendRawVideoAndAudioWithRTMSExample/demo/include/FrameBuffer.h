#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>

class FrameBuffer {
public:
    void push(const uint8_t* data, size_t size);
    bool pop(std::vector<uint8_t>& frame);
    void stop();

private:
    std::queue<std::vector<uint8_t>> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stopped_ = false;
};
