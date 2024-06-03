//GetVideoRawData
#include "ZoomSDKRawDataPipeDelegate.h"
#include <wchar.h>
#include <rawdata/zoom_rawdata_api.h>
#include <rawdata/rawdata_renderer_interface.h>
#include <string>


//GetAudioRawData
#include "rawdata/rawdata_audio_helper_interface.h"
#include "zoom_sdk_def.h" 
#include <iostream>
#include <fstream>


#include <stdexcept>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>

//try catch
#include <stdexcept>
#include <thread>

USING_ZOOM_SDK_NAMESPACE;





const AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };

std::vector<ZoomSDKRawDataPipeDelegate*> ZoomSDKRawDataPipeDelegate::list_;
int ZoomSDKRawDataPipeDelegate::instance_count = 0;

std::queue<YUVRawDataI420*> videoQueue;
std::queue<AudioRawData*> audioQueue;
std::mutex videoMutex;
std::mutex audioMutex;
std::condition_variable videoCV;
std::condition_variable audioCV;
bool recording = true;

//constructor
ZoomSDKRawDataPipeDelegate::ZoomSDKRawDataPipeDelegate()
{

}

void ZoomSDKRawDataPipeDelegate::SubScribeUser(IUserInfo* user, IZoomSDKRenderer* renderer)
{
	instance_id_ = instance_count++;
	user_ = user;
	renderer->subscribe(user->GetUserID(), RAW_DATA_TYPE_VIDEO);
	list_.push_back(this);

	ffmpeg_start();
}

void ZoomSDKRawDataPipeDelegate::SubScribeShareScreen(IUserInfo* user, bool isShareScreen, IZoomSDKRenderer* renderer)

{
	instance_id_ = instance_count++;
	user_ = user;
	renderer->subscribe(user->GetUserID(), RAW_DATA_TYPE_SHARE);
	list_.push_back(this);
}


//destructor
ZoomSDKRawDataPipeDelegate::~ZoomSDKRawDataPipeDelegate()
{

	// finish ffmpeg encoding
	log(L"********** [%d] Finishing encoding, user: %s, %dx%d.\n", instance_id_, user_->GetUserName(), in_width, in_height);
	
	ffmpeg_stop();
	
	log(L"********** [%d] UnSubscribe, user: %s.\n", instance_id_, user_->GetUserName());
	instance_count--;
	user_ = nullptr;
}

//look for user in LIST
ZoomSDKRawDataPipeDelegate* ZoomSDKRawDataPipeDelegate::find_instance(IUserInfo* user)
{
	for (auto iter = list_.begin(); iter != list_.end(); iter++)
	{
		ZoomSDKRawDataPipeDelegate* item = *iter;
		if (item->user_ == user)
		{
			return item;
		}
	}
	return nullptr;
}

void ZoomSDKRawDataPipeDelegate::stop_encoding_for(IUserInfo* user)
{
	ZoomSDKRawDataPipeDelegate* encoder = ZoomSDKRawDataPipeDelegate::find_instance(user);
	if (encoder)
	{
		encoder->~ZoomSDKRawDataPipeDelegate();
	}
}


void ZoomSDKRawDataPipeDelegate::stop_encoding_for(IUserInfo* user, bool isShareScreen)
{
	ZoomSDKRawDataPipeDelegate* encoder = ZoomSDKRawDataPipeDelegate::find_instance(user);
	if (encoder)
	{
		encoder->~ZoomSDKRawDataPipeDelegate();
	}
}


void ZoomSDKRawDataPipeDelegate::onRendererBeDestroyed()
{
}

void ZoomSDKRawDataPipeDelegate::onRawDataFrameReceived(YUVRawDataI420* data)
{
	{
		std::lock_guard<std::mutex> lock(videoMutex);
		data->AddRef();
		videoQueue.push(data);
	}
	videoCV.notify_one();
}

void ZoomSDKRawDataPipeDelegate::onRawDataStatusChanged(RawDataStatus status)
{
	log(L"********** [%d] onRawDataStatusChanged, user: %s, %d.\n", instance_id_, user_->GetUserName(), status);
	if (status == RawData_On)
	{
	}
	else
	{
	}
}

void ZoomSDKRawDataPipeDelegate::err_msg(int code)
{
	char errbuf[100];
	av_strerror(code, errbuf, 100);
	printf("%s\n", errbuf);
}



void ZoomSDKRawDataPipeDelegate::onMixedAudioRawDataReceived(AudioRawData* audioRawData)
{

	{
		std::lock_guard<std::mutex> lock(audioMutex);
		audioQueue.push(audioRawData);
	}
	audioCV.notify_one();
}

void ZoomSDKRawDataPipeDelegate::onOneWayAudioRawDataReceived(AudioRawData* audioRawData, uint32_t node_id)
{


}

void ZoomSDKRawDataPipeDelegate::onShareAudioRawDataReceived(AudioRawData* data_) {}

void ZoomSDKRawDataPipeDelegate::log(const wchar_t* format, ...)
{
	va_list args;
	va_start(args, format);
	wprintf(format, args);
	va_end(args);
}


int ZoomSDKRawDataPipeDelegate::ffmpeg_start()
{
	int ret = 0;

	encode_and_mux();


	return ret;
}


AVFrame* ZoomSDKRawDataPipeDelegate::create_black_video_frame(AVCodecContext* codecCtx) {
	AVFrame* frame = av_frame_alloc();
	frame->format = codecCtx->pix_fmt;
	frame->width = codecCtx->width;
	frame->height = codecCtx->height;
	av_frame_get_buffer(frame, 0);
	av_frame_make_writable(frame);
	memset(frame->data[0], 0, frame->linesize[0] * codecCtx->height); // Y
	memset(frame->data[1], 128, frame->linesize[1] * codecCtx->height / 2); // U
	memset(frame->data[2], 128, frame->linesize[2] * codecCtx->height / 2); // V
	return frame;
}

AVFrame* ZoomSDKRawDataPipeDelegate::create_silent_audio_frame(AVCodecContext * codecCtx) {
	AVFrame* frame = av_frame_alloc();
	frame->nb_samples = codecCtx->frame_size;
	frame->format = codecCtx->sample_fmt;
	frame->channel_layout = codecCtx->channel_layout;
	frame->channels = codecCtx->channels;
	frame->sample_rate = codecCtx->sample_rate;
	av_frame_get_buffer(frame, 0);
	av_frame_make_writable(frame);
	memset(frame->data[0], 0, frame->linesize[0]);
	return frame;
}


bool validate_audio_buffer(const uint8_t* buffer, int buffer_len) {
	for (int i = 0; i < buffer_len; ++i) {
		if (isnan(buffer[i]) || isinf(buffer[i])) {
			return false;
		}
	}
	return true;
}


void ZoomSDKRawDataPipeDelegate::encode_and_mux() {
	AVFormatContext* formatCtx = nullptr;
	AVCodecContext* videoCodecCtx = nullptr;
	AVCodecContext* audioCodecCtx = nullptr;
	AVStream* videoStream = nullptr;
	AVStream* audioStream = nullptr;

	avformat_alloc_output_context2(&formatCtx, nullptr, "matroska", "output.mkv");

	// Video codec initialization
	AVCodec* videoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
	videoCodecCtx = avcodec_alloc_context3(videoCodec);
	videoCodecCtx->width = 1280;  // Replace with actual width
	videoCodecCtx->height = 720;  // Replace with actual height
	videoCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	videoCodecCtx->time_base = (AVRational){ 1, 30 };
	avcodec_open2(videoCodecCtx, videoCodec, nullptr);
	videoStream = avformat_new_stream(formatCtx, videoCodec);
	avcodec_parameters_from_context(videoStream->codecpar, videoCodecCtx);

	// Audio codec initialization
	AVCodec* audioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	audioCodecCtx = avcodec_alloc_context3(audioCodec);
	audioCodecCtx->sample_rate = 44100;  // Replace with actual sample rate
	audioCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
	audioCodecCtx->channels = 2;
	audioCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
	audioCodecCtx->time_base = (AVRational){ 1, audioCodecCtx->sample_rate };
	avcodec_open2(audioCodecCtx, audioCodec, nullptr);
	audioStream = avformat_new_stream(formatCtx, audioCodec);
	avcodec_parameters_from_context(audioStream->codecpar, audioCodecCtx);

	avio_open(&formatCtx->pb, "output.mkv", AVIO_FLAG_WRITE);
	avformat_write_header(formatCtx, nullptr);


	int64_t video_pts = 0;
	int64_t audio_pts = 0;

	while (recording) {
		std::unique_lock<std::mutex> videoLock(videoMutex);
		std::unique_lock<std::mutex> audioLock(audioMutex);

		if (videoQueue.empty()) {
			videoCV.wait_for(videoLock, std::chrono::milliseconds(30));
			if (videoQueue.empty()) {
				AVFrame* blackFrame = create_black_video_frame(videoCodecCtx);
				blackFrame->pts = video_pts++;
				avcodec_send_frame(videoCodecCtx, blackFrame);
				AVPacket pkt;
				av_init_packet(&pkt);
				pkt.data = nullptr;
				pkt.size = 0;
				if (avcodec_receive_packet(videoCodecCtx, &pkt) == 0) {
					pkt.stream_index = videoStream->index;
					pkt.pts = av_rescale_q(pkt.pts, videoCodecCtx->time_base, videoStream->time_base);
					pkt.dts = av_rescale_q(pkt.dts, videoCodecCtx->time_base, videoStream->time_base);
					pkt.duration = av_rescale_q(pkt.duration, videoCodecCtx->time_base, videoStream->time_base);
					av_interleaved_write_frame(formatCtx, &pkt);
					av_packet_unref(&pkt);
				}
				av_frame_free(&blackFrame);
			}
		}

		if (audioQueue.empty()) {
			audioCV.wait_for(audioLock, std::chrono::milliseconds(30));
			if (audioQueue.empty()) {
				AVFrame* silentFrame = create_silent_audio_frame(audioCodecCtx);
				silentFrame->pts = audio_pts;
				audio_pts += silentFrame->nb_samples;
				avcodec_send_frame(audioCodecCtx, silentFrame);
				AVPacket pkt;
				av_init_packet(&pkt);
				pkt.data = nullptr;
				pkt.size = 0;
				if (avcodec_receive_packet(audioCodecCtx, &pkt) == 0) {
					pkt.stream_index = audioStream->index;
					pkt.pts = av_rescale_q(pkt.pts, audioCodecCtx->time_base, audioStream->time_base);
					pkt.dts = av_rescale_q(pkt.dts, audioCodecCtx->time_base, audioStream->time_base);
					pkt.duration = av_rescale_q(pkt.duration, audioCodecCtx->time_base, audioStream->time_base);
					av_interleaved_write_frame(formatCtx, &pkt);
					av_packet_unref(&pkt);
				}
				av_frame_free(&silentFrame);
			}
		}

		if (!videoQueue.empty()) {
			try {
				YUVRawDataI420* videoData = videoQueue.front();
				videoQueue.pop();
				videoLock.unlock();

				if (videoData == nullptr) {
					std::cerr << "Error: videoData is null" << std::endl;
					continue;
				}

				AVFrame* frame = av_frame_alloc();
				if (!frame) {
					std::cerr << "Error: Could not allocate AVFrame" << std::endl;
					continue;  // Skip to the next iteration if frame allocation fails
				}

				frame->format = videoCodecCtx->pix_fmt;
				frame->width = videoCodecCtx->width;
				frame->height = videoCodecCtx->height;

				if (av_frame_get_buffer(frame, 0) < 0) {
					std::cerr << "Error: Could not allocate frame buffer" << std::endl;
					av_frame_free(&frame);
					continue;  // Skip to the next iteration if frame buffer allocation fails
				}

				if (av_frame_make_writable(frame) < 0) {
					std::cerr << "Error: Could not make frame writable" << std::endl;
					av_frame_free(&frame);
					continue;  // Skip to the next iteration if making frame writable fails
				}

				// Check the buffer sizes to ensure they are valid
				int streamWidth = videoData->GetStreamWidth();
				int streamHeight = videoData->GetStreamHeight();

				if (streamWidth > 0 && streamHeight > 0) {
					int ySize = streamWidth * streamHeight;
					int uSize = ySize / 4;
					int vSize = ySize / 4;

					std::cout << "Y Size: " << ySize << ", U Size: " << uSize << ", V Size: " << vSize << std::endl;



					if (videoData->GetYBuffer() != nullptr && videoData->GetUBuffer() != nullptr && videoData->GetVBuffer() != nullptr) {
						memcpy(frame->data[0], videoData->GetYBuffer(), ySize);
						memcpy(frame->data[1], videoData->GetUBuffer(), uSize);
						memcpy(frame->data[2], videoData->GetVBuffer(), vSize);

						frame->pts = video_pts++;

						avcodec_send_frame(videoCodecCtx, frame);
						AVPacket pkt;
						av_init_packet(&pkt);
						pkt.data = nullptr;
						pkt.size = 0;
						if (avcodec_receive_packet(videoCodecCtx, &pkt) == 0) {
							pkt.stream_index = videoStream->index;
							pkt.pts = av_rescale_q(pkt.pts, videoCodecCtx->time_base, videoStream->time_base);
							pkt.dts = av_rescale_q(pkt.dts, videoCodecCtx->time_base, videoStream->time_base);
							pkt.duration = av_rescale_q(pkt.duration, videoCodecCtx->time_base, videoStream->time_base);
							av_interleaved_write_frame(formatCtx, &pkt);
							av_packet_unref(&pkt);
						}
					}
					else {
						std::cerr << "Error: Frame data is null" << std::endl;
					}
				}
				else {

				}
				videoData->Release();
				av_frame_free(&frame);
			}
			catch (const std::exception& e) {
			

			}
		
		}

		if (!audioQueue.empty()) {
			AudioRawData* audioData = audioQueue.front();
			audioQueue.pop();
			audioLock.unlock();

			if (audioData == nullptr) {
				std::cerr << "Error: audioData is null" << std::endl;
				continue;
			}

			AVFrame* audioFrame = av_frame_alloc();
			audioFrame->nb_samples = audioCodecCtx->frame_size;
			audioFrame->format = audioCodecCtx->sample_fmt;
			audioFrame->channel_layout = audioCodecCtx->channel_layout;
			audioFrame->channels = audioCodecCtx->channels;
			audioFrame->sample_rate = audioCodecCtx->sample_rate;
			av_frame_get_buffer(audioFrame, 0);
			av_frame_make_writable(audioFrame);

			// Fill the frame with data from audioData
			if (audioFrame->data[0] != nullptr) {
				if (validate_audio_buffer(reinterpret_cast<const uint8_t*>(audioData->GetBuffer()), audioData->GetBufferLen())) {
					memcpy(audioFrame->data[0], audioData->GetBuffer(), audioData->GetBufferLen());
				}
				else {
					std::cerr << "Error: Audio data contains invalid values" << std::endl;
					av_frame_free(&audioFrame);
					continue;
				}
			}
			else {
				std::cerr << "Error: Audio frame data is null" << std::endl;
			}

			audioFrame->pts = audio_pts;
			audio_pts += audioFrame->nb_samples;

			avcodec_send_frame(audioCodecCtx, audioFrame);
			AVPacket pkt;
			av_init_packet(&pkt);
			pkt.data = nullptr;
			pkt.size = 0;
			if (avcodec_receive_packet(audioCodecCtx, &pkt) == 0) {
				pkt.stream_index = audioStream->index;
				pkt.pts = av_rescale_q(pkt.pts, audioCodecCtx->time_base, audioStream->time_base);
				pkt.dts = av_rescale_q(pkt.dts, audioCodecCtx->time_base, audioStream->time_base);
				pkt.duration = av_rescale_q(pkt.duration, audioCodecCtx->time_base, audioStream->time_base);
				av_interleaved_write_frame(formatCtx, &pkt);
				av_packet_unref(&pkt);
			}
			av_frame_free(&audioFrame);
		}
	}

	av_write_trailer(formatCtx);
	avcodec_close(videoCodecCtx);
	avcodec_close(audioCodecCtx);
	avformat_free_context(formatCtx);
}


int ZoomSDKRawDataPipeDelegate::ffmpeg_filter(uint8_t* Y, uint8_t* U, uint8_t* V)
{
	// input Y,U,V
	frame_in->data[0] = Y;
	frame_in->data[1] = U;
	frame_in->data[2] = V;

	// output Y,U,V
	if (isOutputYUV && frame_in->format == AV_PIX_FMT_YUV420P)
	{
		for (int i = 0; i < frame_in->height; i++)
		{
			fwrite(frame_in->data[0] + frame_in->linesize[0] * i, 1, frame_in->width, fp_yuv);
		}
		for (int i = 0; i < frame_in->height / 2; i++)
		{
			fwrite(frame_in->data[1] + frame_in->linesize[1] * i, 1, frame_in->width / 2, fp_yuv);
		}
		for (int i = 0; i < frame_in->height / 2; i++)
		{
			fwrite(frame_in->data[2] + frame_in->linesize[2] * i, 1, frame_in->width / 2, fp_yuv);
		}
	}

	// apply filter
	if (av_buffersrc_add_frame(buffersrc_ctx, frame_in) < 0)
	{
		printf("Error while add frame.\n");
		return -1;
	}

	/* pull filtered pictures from the filtergraph */
	if ((av_buffersink_get_frame(buffersink_ctx, frame_out)) < 0)
		return -1;

	return 0;
}



int ZoomSDKRawDataPipeDelegate::ffmpeg_stop()
{

	recording = false;

}



