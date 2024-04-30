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

//try catch
#include <stdexcept>

USING_ZOOM_SDK_NAMESPACE;

const AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
std::vector<ZoomSDKRawDataPipeDelegate*> ZoomSDKRawDataPipeDelegate::list_;
int ZoomSDKRawDataPipeDelegate::instance_count = 0;

//buffer for raw audio data
bool audioDataAvailable = false;
std::vector<uint8_t> audioBuffer;



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
	if (is_ffmpeg_encoding_on)
	{
		ffmpeg_stop();
		is_ffmpeg_encoding_on = 0;
	}

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
	const zchar_t* userName = user_->GetUserName();
	const uint userID = user_->GetUserID();
	const int width = data->GetStreamWidth();
	const int height = data->GetStreamHeight();
	const int bufLen = data->GetBufferLen();
	const int rotation = data->GetRotation();
	const int sourceID = data->GetSourceID();

	// to skip frames when sourceID comes in but userID is not ready, otherwise create another seperate file for this moment.
	if ((sourceID != current_sourceID) && (sourceID == 0 || userID != 0))
	{
		log(L"********** [%d] Start encoding, user: %s, %dx%d, sourceID: %d.\n", instance_id_, user_->GetUserName(), width, height, sourceID);
		if (is_ffmpeg_encoding_on)
		{
			ffmpeg_stop();
		};
		current_sourceID = sourceID;
		in_width = width;
		in_height = height;
		ffmpeg_start(userName, userID, sourceID);
		is_ffmpeg_encoding_on = 1;

		gstreamer_start();
	}
	else
	{
		//if resolution changes
		if (is_ffmpeg_encoding_on == 1 && (width != in_width || height != in_height))
		{
			is_ffmpeg_encoding_on = 0;
			log(L"********** [%d] Update scale, user: %s, %dx%d -> %dx%d sourceID: %d.\n", instance_id_, user_->GetUserName(), in_width, in_height, width, height, sourceID);
			in_width = width;
			in_height = height;
			// the video source reslution changed, update the ffmpeg filter for scale.
			ffmpeg_filter_init();

			is_ffmpeg_encoding_on = 1;
		}
	}
	if (is_ffmpeg_encoding_on)
	{
		ffmpeg_filter(
			reinterpret_cast<unsigned char*>(data->GetYBuffer()),
			reinterpret_cast<unsigned char*>(data->GetUBuffer()),
			reinterpret_cast<unsigned char*>(data->GetVBuffer()));
		ffmpeg_encode();
	}
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



void ZoomSDKRawDataPipeDelegate::encode_audio_to_wav(uint8_t* audioDataPtr, int audioDataSize, int64_t pts) {
	// Initialize an AVPacket to hold the encoded audio data
	AVPacket audioPkt;
	av_init_packet(&audioPkt);

	// Encode the PCM audio data to WAV
	AVCodec* wavCodec = avcodec_find_encoder_by_name("pcm_s16le");
	if (!wavCodec) {
		std::cerr << "PCM encoder not found!" << std::endl;
		return;
	}

	// Allocate an AVCodecContext for the WAV encoder
	AVCodecContext* wavCodecContext = avcodec_alloc_context3(wavCodec);
	if (!wavCodecContext) {
		std::cerr << "Failed to allocate WAV codec context!" << std::endl;
		return;
	}

	// Set codec parameters for the WAV encoder
	wavCodecContext->sample_fmt = AV_SAMPLE_FMT_S16;
	wavCodecContext->bit_rate = 128000; // Adjust as needed
	wavCodecContext->sample_rate = 32000; // Adjust as needed
	wavCodecContext->channels = 1; // Adjust as needed
	wavCodecContext->channel_layout = AV_CH_LAYOUT_MONO;
	wavCodecContext->time_base = { 1, wavCodecContext->sample_rate };

	// Open the WAV encoder
	if (avcodec_open2(wavCodecContext, wavCodec, nullptr) < 0) {
		std::cerr << "Failed to open WAV encoder!" << std::endl;
		avcodec_free_context(&wavCodecContext);
		return;
	}

	// Encode the audio data
	AVFrame* audioFrame = av_frame_alloc();
	audioFrame->nb_samples = audioDataSize / (wavCodecContext->channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16));
	audioFrame->format = AV_SAMPLE_FMT_S16;
	audioFrame->channel_layout = av_get_default_channel_layout(wavCodecContext->channels);

	int ret = avcodec_fill_audio_frame(audioFrame, wavCodecContext->channels, AV_SAMPLE_FMT_S16, audioDataPtr, audioDataSize, 0);
	if (ret < 0) {
		//std::cerr << "Error filling audio frame: " << av_err2str(ret) << std::endl;
		av_frame_free(&audioFrame);
		avcodec_free_context(&wavCodecContext);
		return;
	}

	ret = avcodec_send_frame(wavCodecContext, audioFrame);
	if (ret < 0) {
		//std::cerr << "Error sending audio frame for encoding: " << av_err2str(ret) << std::endl;
		av_frame_free(&audioFrame);
		avcodec_free_context(&wavCodecContext);
		return;
	}

	while (ret >= 0) {
		//ret = avcodec_receive_packet(wavCodecContext, &audioPkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			break;
		}
		else if (ret < 0) {
			//std::cerr << "Error encoding audio frame to WAV: " << av_err2str(ret) << std::endl;
			av_packet_unref(&audioPkt);
			av_frame_free(&audioFrame);
			avcodec_free_context(&wavCodecContext);
			return;
		}

		// Set packet PTS and stream index
		audioPkt.pts = pts;
		audioPkt.dts = pts;
		audioPkt.stream_index = audioStream->index;

		// Write the encoded audio packet to the output file
		ret = av_write_frame(formatContext, &audioPkt);
		if (ret < 0) {
			//std::cerr << "Error writing encoded audio packet: " << av_err2str(ret) << std::endl;
			av_packet_unref(&audioPkt);
			av_frame_free(&audioFrame);
			avcodec_free_context(&wavCodecContext);
			return;
		}

		av_packet_unref(&audioPkt);
	}

	// Free resources
	//av_frame_free(&audioFrame);
	//avcodec_free_context(&wavCodecContext);
}

void ZoomSDKRawDataPipeDelegate::save_audio_to_file(AudioRawData* audioRawData, uint32_t node_id)
{
	char fileName[100];
	sprintf(fileName, "%d", node_id);
	char pcmFileName[110];

	sprintf(pcmFileName, "../%s.pcm", fileName);
	static std::ofstream pcmFile;
	pcmFile.open(pcmFileName, std::ios::out | std::ios::binary | std::ios::app);

	if (!pcmFile.is_open()) {
		std::cout << "Failed to open wave file" << std::endl;
		return;
	}

	// Write the audio data to the file
	pcmFile.write((char*)audioRawData->GetBuffer(), audioRawData->GetBufferLen());
	//std::cout << "buffer length: " << audioRawData->GetBufferLen() << std::endl;
	std::cout << "audio buffer : " << audioRawData->GetBufferLen() << " node_id: " << node_id << " source_id: " << current_sourceID << std::endl;

	// Close the wave file
	pcmFile.close();
	pcmFile.flush();




}


int flush_encoder(AVFormatContext* fmt_ctx, unsigned int stream_index) {
	int ret;
	int got_frame;
	AVPacket enc_pkt;
	if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities &
		AV_CODEC_CAP_DELAY))
		return 0;
	while (1) {
		enc_pkt.data = NULL;
		enc_pkt.size = 0;
		av_init_packet(&enc_pkt);
		ret = avcodec_encode_audio2(fmt_ctx->streams[stream_index]->codec, &enc_pkt,
			NULL, &got_frame);
		av_frame_free(NULL);
		if (ret < 0)
			break;
		if (!got_frame) {
			ret = 0;
			break;
		}
		printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", enc_pkt.size);
		/* mux encoded frame */
		ret = av_write_frame(fmt_ctx, &enc_pkt);
		if (ret < 0)
			break;
	}
	return ret;
}


//gstreamer
void ZoomSDKRawDataPipeDelegate::save_audio_to_wav(uint8_t* audioDataPtr, int audioDataSize, uint32_t node_id) {
	
	// Push audio data into the GStreamer pipeline
	GstBuffer* buffer = gst_buffer_new_allocate(NULL, audioDataSize, NULL);
	GstMapInfo map;
	gst_buffer_map(buffer, &map, GST_MAP_WRITE);
	memcpy(map.data, audioDataPtr, audioDataSize);
	gst_buffer_unmap(buffer, &map);

	GstFlowReturn ret;
	g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
	gst_buffer_unref(buffer);


}



void ZoomSDKRawDataPipeDelegate::onMixedAudioRawDataReceived(AudioRawData* audioRawData)
{


}

void ZoomSDKRawDataPipeDelegate::onOneWayAudioRawDataReceived(AudioRawData* audioRawData, uint32_t node_id)
{
	try {
		steady_clock::time_point current_time = steady_clock::now();

		// Get the audio buffer pointer
		char* audioBufferPtr = audioRawData->GetBuffer();

		// Cast the char* pointer to uint8_t*
		uint8_t* audioDataPtr = reinterpret_cast<uint8_t*>(audioBufferPtr);

		//long temppts = duration_cast<std::chrono::milliseconds>(current_time - start_time).count() * (audioStream->time_base.den) / (audioStream->time_base.num * 1000);

		// Fetch audio data from the provided AudioRawData object
		char* audioBuffer = audioRawData->GetBuffer();
		unsigned int audioBufferSize = audioRawData->GetBufferLen();

		// Check if audio data is valid
		if (!audioBuffer || audioBufferSize == 0) {
			std::cerr << "Invalid audio data!" << std::endl;
			return;
		}

		// Call save_audio_to_wav to process the audio data
		save_audio_to_wav(reinterpret_cast<uint8_t*>(audioBuffer), audioBufferSize, node_id);
		save_audio_to_wav(audioDataPtr, audioRawData->GetBufferLen(), node_id);
		//save_audio_to_file(audioRawData, node_id);
		std::cout << "encoding audio, pts:  " << audioPkt.pts << " node ID:" << node_id << std::endl;


		// If no exception is thrown, continue with normal execution
		std::cout << "Operation completed successfully." << std::endl;
	}
	catch (const std::exception& e) {
		// Catch any exceptions thrown by the risky operation
		std::cerr << "Exception caught: " << e.what() << std::endl;

		// Handle the exception gracefully, such as logging the error or recovering from it
	}

}

void ZoomSDKRawDataPipeDelegate::onShareAudioRawDataReceived(AudioRawData* data_) {}

void ZoomSDKRawDataPipeDelegate::log(const wchar_t* format, ...)
{
	va_list args;
	va_start(args, format);
	wprintf(format, args);
	va_end(args);
}


int ZoomSDKRawDataPipeDelegate::ffmpeg_start(const char* userName, uint userID, int sourceID)
{
	int ret = 0;

	// Timestamp
	start_time = steady_clock::now();

	// Init files
	if (userID == 0)
		userID = 0;
	char fileName[100];
	sprintf(fileName, "%d_%d_%s_%dx%d_to_%dx%d", userID, sourceID, userName, in_width, in_height, out_width, out_height);
	char yuvFileName[110];
	sprintf(yuvFileName, "../%s.yuv", fileName);
	if (isOutputYUV)
	{
		fp_yuv = fopen(yuvFileName, "wb+");
		if (fp_yuv == NULL)
		{
			printf("Error opening output file.\n");
			return -1;
		}
	}

	char outFileName[110];
	sprintf(outFileName, "%s.avi", fileName);
	sprintf(fn_out, "../%s", outFileName);

	// FFmpeg init
	// Init filters
	avfilter_register_all();

	ffmpeg_filter_init();

	// Init encoder
	av_register_all();
	formatContext = avformat_alloc_context();

	// Method 1: Guess Format
	fmt = av_guess_format("avi", fn_out, NULL);
	formatContext->oformat = fmt;

	// Open output file
	avio_open(&formatContext->pb, fn_out, AVIO_FLAG_WRITE);

	// Init streams & codec
	videoStream = avformat_new_stream(formatContext, 0);

	// Param that must be set
	codecContext = videoStream->codec;
	codecContext->codec_id = AV_CODEC_ID_MPEG4;
	codecContext->codec_type = AVMEDIA_TYPE_VIDEO;
	codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
	codecContext->width = out_width;
	codecContext->height = out_height;

	// Set your desired bitrate and other codec parameters here
	codecContext->bit_rate = 400000;
	codecContext->gop_size = 10; // Set your desired GOP size
	codecContext->time_base.num = 1;
	codecContext->time_base.den = 25;
	codecContext->qmin = 10;
	codecContext->qmax = 51;
	codecContext->max_b_frames = 3;

	// Specify MPEG-4 codec explicitly
	videoCodec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);

	// Open MPEG-4 encoder
	avcodec_open2(codecContext, videoCodec, NULL);


	// Create audio stream
	audioStream = avformat_new_stream(formatContext, 0);

	audioCodecContext = audioStream->codec;
	audioCodecContext->codec_id = AV_CODEC_ID_AAC;
	audioCodecContext->codec_type = AVMEDIA_TYPE_AUDIO;
	audioCodecContext->sample_fmt = AV_SAMPLE_FMT_FLT;
	audioCodecContext->sample_rate = 32000;
	audioCodecContext->channels = 1;
	audioCodecContext->channel_layout = AV_CH_LAYOUT_MONO;


	audioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);

	// Open AAC encoder
	avcodec_open2(audioCodecContext, audioCodec, nullptr);

	// Write File Header
	avformat_write_header(formatContext, NULL);

	initCompleted = true;


	return ret;
}


int ZoomSDKRawDataPipeDelegate::ffmpeg_filter_init()
{
	int ret;

	buffersrc = avfilter_get_by_name("buffer");
	buffersink = avfilter_get_by_name("buffersink");
	outputs = avfilter_inout_alloc();
	inputs = avfilter_inout_alloc();
	buffersink_params = av_buffersink_params_alloc();

	filter_graph = avfilter_graph_alloc();
	///* buffer video source: the decoded frames from the decoder will be inserted here. */
	char args[512];
	snprintf(args, sizeof(args),
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		in_width, in_height, AV_PIX_FMT_YUV420P,
		1, 25, 1, 1);

	if (avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, NULL, filter_graph) < 0)
	{
		printf("Cannot create buffer source\n");
		return -1;
	}

	/* buffer video sink: to terminate the filter chain. */
	buffersink_params->pixel_fmts = pix_fmts;
	if ((ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, buffersink_params, filter_graph)) < 0)
	{
		printf("Cannot create buffer sink, code: %d, ", ret);
		err_msg(ret);
		return ret;
	}

	/* Endpoints for the filter graph. */
	outputs = avfilter_inout_alloc();
	inputs = avfilter_inout_alloc();
	outputs->name = av_strdup("in");
	outputs->filter_ctx = buffersrc_ctx;
	outputs->pad_idx = 0;
	outputs->next = NULL;

	inputs->name = av_strdup("out");
	inputs->filter_ctx = buffersink_ctx;
	inputs->pad_idx = 0;
	inputs->next = NULL;

	char filter_descr[50];
	sprintf(filter_descr, "scale=w=%d:h=%d", out_width, out_height);

	if (avfilter_graph_parse_ptr(filter_graph, filter_descr,
		&inputs, &outputs, NULL) < 0)
		return -2;

	if (avfilter_graph_config(filter_graph, NULL) < 0)
		return -3;

	frame_in = av_frame_alloc();
	frame_buffer_in = (unsigned char*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, in_width,
		in_height, 1));
	av_image_fill_arrays(frame_in->data, frame_in->linesize, frame_buffer_in,
		AV_PIX_FMT_YUV420P, in_width, in_height, 1);

	frame_out = av_frame_alloc();
	frame_buffer_out = (unsigned char*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, out_width, out_height, 1));
	av_image_fill_arrays(frame_out->data, frame_out->linesize, frame_buffer_out,
		AV_PIX_FMT_YUV420P, out_width, out_height, 1);

	frame_in->format = AV_PIX_FMT_YUV420P;
	frame_in->width = in_width;
	frame_in->height = in_height;

	return ret;
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

int ZoomSDKRawDataPipeDelegate::ffmpeg_encode()
{
	int ret;

	// timestamp
	steady_clock::time_point current_time = steady_clock::now();

	// frame_out->pts = ((tstruct.time - start_tstruct.time) * 1000 + (tstruct.millitm - start_tstruct.millitm)) * 10;
	frame_out->pts = duration_cast<std::chrono::milliseconds>(current_time - start_time).count() * (videoStream->time_base.den) / (videoStream->time_base.num * 1000);

	av_init_packet(&pkt);

	int got_picture = 0;
	if ((ret = avcodec_encode_video2(codecContext, &pkt, frame_out, &got_picture)) < 0)
	{
		printf("Failed to encode, code: %d, ", ret);
		err_msg(ret);
		return -1;
	}
	if (got_picture == 1)
	{
		printf("Succeed to encode frame: %5d\tsize:%5d\source_id:%d\n", framecnt, pkt.size, current_sourceID);
		framecnt++;
		pkt.stream_index = videoStream->index;
		av_write_frame(formatContext, &pkt);
		av_packet_unref(&pkt);
	}

	av_frame_unref(frame_out);
	printf(".");

	return 0;
}

int ZoomSDKRawDataPipeDelegate::ffmpeg_stop()
{

	// Flush Encoder
	if ((ZoomSDKRawDataPipeDelegate::ffmpeg_flush(formatContext, 0)) < 0)
	{
		printf("Flushing encoder failed\n");
		return -1;
	}

	// Write file trailer
	av_write_trailer(formatContext);

	// Clean
	if (videoStream)
	{
		avcodec_close(videoStream->codec);
		av_free(frame_out);
	}
	avio_close(formatContext->pb);
	avformat_free_context(formatContext);

	if (isOutputYUV)
	{
		fclose(fp_yuv);
	}

	av_free(buffersink_params);

	av_frame_free(&frame_in);
	avfilter_graph_free(&filter_graph);
	return 0;
}

int ZoomSDKRawDataPipeDelegate::ffmpeg_flush(AVFormatContext* fmt_ctx, unsigned int stream_index)
{
	int ret;
	int got_frame;
	AVPacket enc_pkt;
	if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities &
		AV_CODEC_CAP_DELAY))
		return 0;
	while (1)
	{
		enc_pkt.data = NULL;
		enc_pkt.size = 0;
		av_init_packet(&enc_pkt);
		ret = avcodec_encode_video2(fmt_ctx->streams[stream_index]->codec, &enc_pkt,
			NULL, &got_frame);
		av_frame_free(NULL);
		if (ret < 0)
			break;
		if (!got_frame)
		{
			ret = 0;
			break;
		}
		printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", enc_pkt.size);
		/* mux encoded frame */
		ret = av_write_frame(fmt_ctx, &enc_pkt);
		if (ret < 0)
			break;
	}
	return ret;
}

int ZoomSDKRawDataPipeDelegate::gstreamer_start() {

	//// Initialize GStreamer
	//gst_init(NULL, NULL);

	//// Create GStreamer elements
	//GstElement* pipeline = gst_pipeline_new("audio-encoding-pipeline");
	//GstElement* appsrc = gst_element_factory_make("appsrc", "app-source");
	//GstElement* audioconvert = gst_element_factory_make("audioconvert", "audio-convert");
	//GstElement* audioresample = gst_element_factory_make("audioresample", "audio-resample");
	//GstElement* audioencoder = gst_element_factory_make("fdkaacenc", "audio-encoder");
	//GstElement* muxer = gst_element_factory_make("mpegtsmux", "muxer");
	//GstElement* filesink = gst_element_factory_make("filesink", "file-sink");

	//if (!pipeline || !appsrc || !audioconvert || !audioresample || !audioencoder || !muxer || !filesink) {
	//	std::cerr << "Failed to create GStreamer elements!" << std::endl;
	//	return 1;
	//}

	//// Set properties of elements as needed
	//g_object_set(G_OBJECT(appsrc), "is-live", TRUE, NULL);
	//g_object_set(G_OBJECT(filesink), "location", "output.aac", NULL);


	//// Add elements to the pipeline
	//gst_bin_add_many(GST_BIN(pipeline), appsrc, audioconvert, audioresample, audioencoder, muxer, filesink, NULL);

	//// Link elements together
	//gst_element_link_many(appsrc, audioconvert, audioresample, audioencoder, muxer, filesink, NULL);

	//// Start the pipeline
	//GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
	//if (ret == GST_STATE_CHANGE_FAILURE) {
	//	std::cerr << "Failed to start the pipeline!" << std::endl;
	//	return 1;
	//}

	//// Run the GStreamer main loop
	//GstBus* bus = gst_element_get_bus(pipeline);
	//GstMessage* msg;
	//do {
	//	msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
	//	if (msg != NULL) {
	//		// Handle messages
	//		gst_message_unref(msg);
	//	}
	//} while (msg != NULL);

	//// Clean up
	//gst_element_set_state(pipeline, GST_STATE_NULL);
	//gst_object_unref(bus);
	//gst_object_unref(pipeline);


	//// Clean up GStreamer
	//gst_deinit();

	//return 0;

}
