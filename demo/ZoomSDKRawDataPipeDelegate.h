//GetVideoRawData
// ffmpeg

#include <vector>
#include <chrono>
using namespace std::chrono;

// Zoom Video SDK
#define __STDC_CONSTANT_MACROS
extern "C"
{
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixfmt.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavcodec/avcodec.h>
}

#include "zoom_sdk.h"
#include "zoom_sdk_raw_data_def.h"
#include "meeting_service_components/meeting_audio_interface.h"
#include "meeting_service_components/meeting_participants_ctrl_interface.h"
#include "rawdata/rawdata_video_source_helper_interface.h"
#include "rawdata/rawdata_renderer_interface.h"

//GetAudioRawData
#include "rawdata/rawdata_audio_helper_interface.h"
#include "zoom_sdk.h"
#include "zoom_sdk_raw_data_def.h"


USING_ZOOM_SDK_NAMESPACE;

class ZoomSDKRawDataPipeDelegate :
	public IZoomSDKRendererDelegate, 
	public IZoomSDKAudioRawDataDelegate
{
	virtual void onRendererBeDestroyed();
	virtual void onRawDataFrameReceived(YUVRawDataI420* data);
	virtual void onRawDataStatusChanged(RawDataStatus	status);

	static ZoomSDKRawDataPipeDelegate* find_instance(IUserInfo* user);



	int instance_id_;
	static int instance_count;
	static std::vector<ZoomSDKRawDataPipeDelegate*> list_;
	IUserInfo* user_;

	int ffmpeg_start(const char* userName, uint userID, int sourceID);
	int ffmpeg_flush(AVFormatContext* fmt_ctx, unsigned int stream_index);
	int ffmpeg_stop();
	int ffmpeg_filter(uint8_t* Y, uint8_t* U, uint8_t* V);
	int ffmpeg_encode();
	int ffmpeg_filter_init();

	// ffmpeg filter
	AVFrame* frame_in;
	AVFrame* frame_out;
	unsigned char* frame_buffer_in;
	unsigned char* frame_buffer_out;
	int in_width = 1280;
	int in_height = 720;
	int out_width = 1280;
	int out_height = 720;

	AVFilterContext* buffersink_ctx;
	AVFilterContext* buffersrc_ctx;
	AVFilterGraph* filter_graph;
	//static int video_stream_index = -1;
	const AVFilter* buffersrc;
	const AVFilter* buffersink;
	AVFilterInOut* outputs;
	AVFilterInOut* inputs;
	AVBufferSinkParams* buffersink_params;

	//Output YUV
	FILE* fp_yuv;
	int isOutputYUV = 0;

	// ffmpeg encoding
	AVFormatContext* pFormatCtx;
	AVOutputFormat* fmt;
	AVStream* video_st;
	AVCodecContext* pCodecCtx;
	AVCodec* pCodec;
	AVPacket pkt;
	//uint8_t* picture_buf;
	AVFrame* pFrame;
	int picture_size;
	int y_size;
	int framecnt = 0;
	int is_ffmpeg_encoding_on = 0;
	int current_sourceID = -1;
	// struct _timeb start_tstruct;
	steady_clock::time_point start_time;

	//Output video file name.
	char fn_out[120];

public:
	ZoomSDKRawDataPipeDelegate();
	~ZoomSDKRawDataPipeDelegate();


	virtual void SubScribeUser(IUserInfo* user, IZoomSDKRenderer* renderer);
	virtual void SubScribeShareScreen(IUserInfo* user, bool isShareScreen, IZoomSDKRenderer* renderer);
	static void stop_encoding_for(IUserInfo* user);
	static void stop_encoding_for(IUserInfo* user, bool isShareScreen);
	static void log(const wchar_t* format, ...);
	static void err_msg(int code);



public:
	virtual void onMixedAudioRawDataReceived(AudioRawData* data_);
	virtual void onOneWayAudioRawDataReceived(AudioRawData* data_, uint32_t node_id);
};



