#include "ZoomSDKShareSource.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>
#include <utility>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace {

void CopyPlane(const uint8_t* source, int stride, char* destination, int width, int height)
{
	for (int row = 0; row < height; ++row) {
		std::memcpy(destination + row * width, source + row * stride, width);
	}
}

bool CopyToI420(const AVFrame* frame, int width, int height, std::vector<char>& output)
{
	if (!frame || frame->format != AV_PIX_FMT_YUV420P ||
		!frame->data[0] || !frame->data[1] || !frame->data[2]) {
		return false;
	}

	const int y_size = width * height;
	const int uv_width = width / 2;
	const int uv_height = height / 2;
	const int uv_size = uv_width * uv_height;
	output.resize(y_size + uv_size * 2);

	CopyPlane(frame->data[0], frame->linesize[0], output.data(), width, height);
	CopyPlane(frame->data[1], frame->linesize[1], output.data() + y_size, uv_width, uv_height);
	CopyPlane(frame->data[2], frame->linesize[2], output.data() + y_size + uv_size, uv_width, uv_height);
	return true;
}

void StreamShareVideo(
	IZoomSDKShareSender* sender,
	const std::string& video_source,
	std::atomic<bool>* sending)
{
	while (sending->load()) {
		AVFormatContext* format_context = nullptr;
		if (avformat_open_input(&format_context, video_source.c_str(), nullptr, nullptr) < 0) {
			std::cerr << "Unable to open share video: " << video_source << std::endl;
			sending->store(false);
			return;
		}

		if (avformat_find_stream_info(format_context, nullptr) < 0) {
			std::cerr << "Unable to read share video stream information" << std::endl;
			avformat_close_input(&format_context);
			sending->store(false);
			return;
		}

		int stream_index = -1;
		AVCodec* codec = nullptr;
		for (unsigned int i = 0; i < format_context->nb_streams; ++i) {
			AVCodecContext* candidate = format_context->streams[i]->codec;
			if (candidate->codec_type == AVMEDIA_TYPE_VIDEO) {
				stream_index = static_cast<int>(i);
				codec = avcodec_find_decoder(candidate->codec_id);
				break;
			}
		}

		if (stream_index < 0 || !codec) {
			std::cerr << "No supported video stream found in " << video_source << std::endl;
			avformat_close_input(&format_context);
			sending->store(false);
			return;
		}

		AVCodecContext* codec_context = format_context->streams[stream_index]->codec;
		if (avcodec_open2(codec_context, codec, nullptr) < 0) {
			std::cerr << "Unable to open share video decoder" << std::endl;
			avformat_close_input(&format_context);
			sending->store(false);
			return;
		}

		AVPacket packet;
		std::vector<char> i420_frame;
		while (sending->load() && av_read_frame(format_context, &packet) >= 0) {
			if (packet.stream_index == stream_index) {
				AVFrame* frame = av_frame_alloc();
				int frame_finished = 0;
				avcodec_decode_video2(codec_context, frame, &frame_finished, &packet);

				if (frame_finished &&
					CopyToI420(frame, codec_context->width, codec_context->height, i420_frame)) {
					SDKError error = sender->sendShareFrame(
						i420_frame.data(),
						codec_context->width,
						codec_context->height,
						static_cast<int>(i420_frame.size()),
						FrameDataFormat_I420_LIMITED);
					if (error != SDKERR_SUCCESS) {
						std::cerr << "sendShareFrame failed: " << error << std::endl;
						sending->store(false);
					}
				}

				av_frame_free(&frame);
			}

			av_packet_unref(&packet);
			std::this_thread::sleep_for(std::chrono::milliseconds(33));
		}

		avcodec_close(codec_context);
		avformat_close_input(&format_context);
	}
}

} // namespace

ZoomSDKShareSource::ZoomSDKShareSource(std::string video_source)
	: video_source_(std::move(video_source))
{
}

void ZoomSDKShareSource::onStartSend(IZoomSDKShareSender* sender)
{
	std::cout << "Share video channel started" << std::endl;
	sender_ = sender;
	if (sender_ && !sending_.exchange(true)) {
		std::thread(StreamShareVideo, sender_, video_source_, &sending_).detach();
	}
}

void ZoomSDKShareSource::onStopSend()
{
	std::cout << "Share video channel stopped" << std::endl;
	sending_.store(false);
	sender_ = nullptr;
}
