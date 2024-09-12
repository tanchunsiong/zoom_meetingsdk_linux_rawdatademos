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


// Socket related headers
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h> // For non-blocking socket
#include <netinet/tcp.h>

USING_ZOOM_SDK_NAMESPACE;

// Global socket variable (optional, if socket needs to be accessed globally)
int global_socket;



// Connect to the local server at the specified IP and port
int connectToLocalServer(const std::string& server_ip, int port) {
	// Create the socket
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		std::cerr << "Socket creation error!" << std::endl;
		return -1;
	}

	// Server address setup
	struct sockaddr_in serv_addr {};
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);

	// Convert IPv4 address from text to binary form
	if (inet_pton(AF_INET, server_ip.c_str(), &serv_addr.sin_addr) <= 0) {
		std::cerr << "Invalid address!" << std::endl;
		return -1;
	}

	// Attempt to connect to the server
	if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		std::cerr << "Connection failed! Error: " << strerror(errno) << std::endl;
		return -1;
	}

	std::cout << "Successfully connected to server on " << server_ip << ":" << port << std::endl;
	return sock;
}


const AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
std::vector<ZoomSDKRawDataPipeDelegate*> ZoomSDKRawDataPipeDelegate::list_;
int ZoomSDKRawDataPipeDelegate::instance_count = 0;

//buffer for raw audio data
bool audioDataAvailable = false;
std::vector<uint8_t> audioBuffer;



//constructor
ZoomSDKRawDataPipeDelegate::ZoomSDKRawDataPipeDelegate()
{
	
	// Connect to the server
	global_socket = connectToLocalServer("127.0.0.1", 8080);
	if (global_socket < 0) {
		std::cerr << "Failed to connect to the server." << std::endl;
	}
	else {
		std::cout << "Connected to the server." << std::endl;
	}
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

void ZoomSDKRawDataPipeDelegate::onMixedAudioRawDataReceived(AudioRawData* audioRawData)
{
	if (global_socket <= 0) {
		std::cerr << "Invalid socket, cannot send data." << std::endl;
		return;
	}


	// Fire and forget: just send the data without caring about how much was sent
	const char* buffer = audioRawData->GetBuffer();
	size_t buffer_len = audioRawData->GetBufferLen();

	// Send data without worrying about how much is actually sent
	ssize_t sent_bytes = send(global_socket, buffer, buffer_len, 0);

	if (sent_bytes == -1) {
		// Error occurred
		std::cerr << "Error sending data: " << strerror(errno) << std::endl;
	}
	else {
		// Just log that data was sent (fire and forget)
		// std::cout << "Sent " << sent_bytes << " bytes to the server (fire and forget)." << std::endl;
	}


}

void ZoomSDKRawDataPipeDelegate::onOneWayAudioRawDataReceived(AudioRawData* audioRawData, uint32_t node_id)
{


	//char fileName[100];
	//sprintf(fileName, "%d", node_id);
	//char pcmFileName[110];

	//sprintf(pcmFileName, "../%s.pcm", fileName);
	//static std::ofstream pcmFile;
	//pcmFile.open(pcmFileName, std::ios::out | std::ios::binary | std::ios::app);

	//if (!pcmFile.is_open()) {
	//	std::cout << "Failed to open wave file" << std::endl;
	//	return;
	//}

	//// Write the audio data to the file
	//pcmFile.write((char*)audioRawData->GetBuffer(), audioRawData->GetBufferLen());
	////std::cout << "buffer length: " << audioRawData->GetBufferLen() << std::endl;
	//std::cout << "audio buffer : " << audioRawData->GetBufferLen() << " node_id: " << node_id << " source_id: " << current_sourceID << std::endl;

	//// Close the wave file
	//pcmFile.close();
	//pcmFile.flush();


}

void ZoomSDKRawDataPipeDelegate::onShareAudioRawDataReceived(AudioRawData* data_)
{
}

void ZoomSDKRawDataPipeDelegate::onOneWayInterpreterAudioRawDataReceived(AudioRawData* data_, const zchar_t* pLanguageName)
{
}

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
  

    // Write File Header
	avformat_write_header(formatContext, NULL);

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
		printf("Succeed to encode frame: %5d\tsize:%5d\source_id:%d\n", framecnt, pkt.size,current_sourceID);
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
