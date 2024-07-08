//SendVideoRawData

#include "ZoomSDKVideoSource.h"
#include <iostream>
#include <thread> 
#include <iostream>
#include <string>
#include <cstdio>
#include <chrono>


//using namespace cv;
using namespace std;

int video_play_flag = -1;
int width = WIDTH;
int height = HEIGHT;

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

void PlayVideoFileToVirtualCamera(IZoomSDKVideoSender* video_sender, const std::string& video_source) {
    while (true) {
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
        while (av_read_frame(formatContext, &packet) >= 0) {
            if (packet.stream_index == videoStreamIndex) {
                AVFrame* frame = av_frame_alloc();
                if (!frame) {
                    std::cerr << "Error: Could not allocate frame." << std::endl;
                    avformat_close_input(&formatContext);
                    return;
                }

                int frameFinished;
                avcodec_decode_video2(codecContext, frame, &frameFinished, &packet);

                if (frameFinished) {
                    // Now, `frame` contains the YUV420P frame.

                    // Send the frame to the virtual camera.
                    SDKError err = ((IZoomSDKVideoSender*)video_sender)->sendVideoFrame(
                        reinterpret_cast<char*>(frame->data[0]), // Cast to char* for the frame buffer
                        codecContext->width,
                        codecContext->height, 
                        codecContext->width * codecContext->height * 3 / 2, // Frame length for YUV420I
                        0, // Rotation (modify as needed)
                        FrameDataFormat_I420_FULL
                    );
                    if (err != SDKERR_SUCCESS) {
                        std::cerr << "Error: Failed to send video frame." << std::endl;
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
    } // End of the infinite loop
}
void ZoomSDKVideoSource::onInitialize(IZoomSDKVideoSender* sender, IList<VideoSourceCapability>* support_cap_list, VideoSourceCapability& suggest_cap)
{
    std::cout << "ZoomSDKVideoSource onInitialize waiting for turnOn chat command" << endl;
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
        while (video_play_flag > -1) {}
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

