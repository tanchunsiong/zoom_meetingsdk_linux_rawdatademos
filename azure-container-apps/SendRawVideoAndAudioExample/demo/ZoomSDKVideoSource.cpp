//SendVideoRawData

#include "ZoomSDKVideoSource.h"

#include <thread>
#include <iostream>
#include <string>
#include <cstdio>
#include <chrono>
#include <vector>
#include <cstring>


//using namespace cv;
using namespace std;

int video_play_flag = -1;
int width = WIDTH;
int height = HEIGHT;

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

static void CopyPlaneToI420(const uint8_t* src, int src_stride, char* dst, int width, int height) {
    for (int row = 0; row < height; ++row) {
        memcpy(dst + row * width, src + row * src_stride, width);
    }
}

static bool CopyFrameToContiguousI420(const AVFrame* frame, int frame_width, int frame_height, std::vector<char>& output) {
    if (!frame || !frame->data[0] || !frame->data[1] || !frame->data[2]) {
        return false;
    }

    const int y_size = frame_width * frame_height;
    const int uv_width = frame_width / 2;
    const int uv_height = frame_height / 2;
    const int uv_size = uv_width * uv_height;
    output.resize(y_size + (uv_size * 2));

    char* y_plane = output.data();
    char* u_plane = y_plane + y_size;
    char* v_plane = u_plane + uv_size;

    CopyPlaneToI420(frame->data[0], frame->linesize[0], y_plane, frame_width, frame_height);
    CopyPlaneToI420(frame->data[1], frame->linesize[1], u_plane, uv_width, uv_height);
    CopyPlaneToI420(frame->data[2], frame->linesize[2], v_plane, uv_width, uv_height);
    return true;
}

void PlayVideoFileToVirtualCamera(IZoomSDKVideoSender* video_sender, const std::string& video_source) {
    while (video_play_flag > 0) {
        // Initialize FFmpeg.
        av_register_all();
        avcodec_register_all();

        // Open the input file (MP4).
        AVFormatContext* formatContext = nullptr;
        if (avformat_open_input(&formatContext, video_source.c_str(), nullptr, nullptr) < 0) {
            std::cerr << "Error: Could not open file :" << video_source << std::endl;
            return;
        }

        // Find the video stream information.
        if (avformat_find_stream_info(formatContext, nullptr) < 0) {
            std::cerr << "Error: Could not find stream information." << std::endl;
            avformat_close_input(&formatContext);
            continue; // Retry opening the file.
        }

        AVCodec* codec = nullptr;
        int videoStreamIndex = -1;

        // Find the video codec and stream.
        for (int i = 0; i < formatContext->nb_streams; i++) {
            if (formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoStreamIndex = i;
                codec = avcodec_find_decoder(formatContext->streams[i]->codec->codec_id);
                if (!codec) {
                    std::cerr << "Error: Codec not found." << std::endl;
                    avformat_close_input(&formatContext);
                    return;
                }
                break;
            }
        }

        // Open the codec.
        AVCodecContext* codecContext = formatContext->streams[videoStreamIndex]->codec;
        if (avcodec_open2(codecContext, codec, nullptr) < 0) {
            std::cerr << "Error: Could not open codec." << std::endl;
            avformat_close_input(&formatContext);
            return;
        }

        // Set the pixel format explicitly to YUV420I (also known as YV12).
        codecContext->pix_fmt = AV_PIX_FMT_YUV420P; // Use YUV420P for decoding

        // Read and send frames in a loop.
        AVPacket packet;
        std::vector<char> i420_frame;
        while (video_play_flag > 0 && av_read_frame(formatContext, &packet) >= 0) {
            if (packet.stream_index == videoStreamIndex) {
                AVFrame* frame = av_frame_alloc();
                if (!frame) {
                    std::cerr << "Error: Could not allocate frame." << std::endl;
                    avformat_close_input(&formatContext);
                    return;
                }

                int frameFinished;
                avcodec_decode_video2(codecContext, frame, &frameFinished, &packet);

                if (frameFinished && CopyFrameToContiguousI420(frame, codecContext->width, codecContext->height, i420_frame)) {
                    SDKError err = video_sender->sendVideoFrame(
                        i420_frame.data(),
                        codecContext->width,
                        codecContext->height,
                        static_cast<int>(i420_frame.size()),
                        0,
                        FrameDataFormat_I420_LIMITED
                    );
                    if (err != SDKERR_SUCCESS) {
                        std::cerr << "Error: Failed to send video frame. Error code: " << err << std::endl;
                        av_frame_free(&frame);
                        avformat_close_input(&formatContext);
                        return;
                    }
                }

                av_frame_free(&frame);
            }

            av_packet_unref(&packet);

            // Add a delay to control the frame rate (adjust as needed).
            std::this_thread::sleep_for(std::chrono::milliseconds(33));  // Approximately 30 frames per second.
        }

        // Clean up.
        avcodec_close(codecContext);
        avformat_close_input(&formatContext);
    }
}
void ZoomSDKVideoSource::onInitialize(IZoomSDKVideoSender* sender, IList<VideoSourceCapability>* support_cap_list, VideoSourceCapability& suggest_cap)
{
    std::cout << "ZoomSDKVideoSource onInitialize" << endl;
    video_sender_ = sender;
}

void ZoomSDKVideoSource::onPropertyChange(IList<VideoSourceCapability>* support_cap_list, VideoSourceCapability suggest_cap)
{
    std::cout << "onPropertyChange" << endl;
    std::cout << "suggest frame: " << suggest_cap.frame << endl;
    std::cout << "suggest size: " << suggest_cap.width << "x" << suggest_cap.height << endl;
    width = suggest_cap.width;
    height = suggest_cap.height;
    std::cout << "calculated frameLen: " << height / 2 * 3 * width << endl;
}

void ZoomSDKVideoSource::onStartSend()
{
    std::cout << "onStartSend" << endl;
    if (video_sender_ && video_play_flag != 1) {
        video_play_flag = 1;
        thread(PlayVideoFileToVirtualCamera, video_sender_, video_source_).detach();
    }
    else {
        std::cout << "video_sender_ is null" << endl;
    }
}

void ZoomSDKVideoSource::onStopSend()
{
    std::cout << "onCameraStopSend" << endl;
    video_play_flag = 0;
}

void ZoomSDKVideoSource::onUninitialized()
{
    std::cout << "onUninitialized" << endl;
    video_sender_ = nullptr;
}

ZoomSDKVideoSource::ZoomSDKVideoSource(string video_source)
{
    video_source_ = video_source;
}
