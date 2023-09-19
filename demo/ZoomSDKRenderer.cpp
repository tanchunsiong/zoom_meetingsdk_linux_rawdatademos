#include "rawdata/rawdata_video_source_helper_interface.h"
#include "ZoomSDKRenderer.h"
#include "zoom_sdk_def.h" 
#include <iostream>


#include <fstream>
#include <cstring>
#include <string>
#include <cstdlib>
#include <cstdint>
#include <cstdio>


extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}


void ZoomSDKRenderer::onRawDataFrameReceived(YUVRawDataI420* data)
{
	std::cout << "onRawDataFrameReceived." << std::endl;

	std::cout << "width." << data->GetStreamWidth() << std::endl;
	std::cout << "height." << data->GetStreamHeight() << std::endl;
	//std::cout << "sourceID." << data->GetSourceID() << std::endl;
	//SaveToRawYUVFile(data);
    SaveYUVRawDataToMP4(data);

}
void ZoomSDKRenderer::onRawDataStatusChanged(RawDataStatus status)
{
	std::cout << "onRawDataStatusChanged." << std::endl;
}

void ZoomSDKRenderer::onRendererBeDestroyed()
{
	std::cout << "onRendererBeDestroyed ." << std::endl;
}

//void ZoomSDKRenderer::SaveToRawYUVFile(YUVRawDataI420* data) {
//
//	// Open the file for writing
//	ofstream outputFile("output.yuv", ios::out | ios::binary | ios::app);
//	if (!outputFile.is_open())
//	{
//		cout << "Error opening file." << endl;
//		return;
//	}
//
//	char* _data = new char[data->GetStreamHeight() * data->GetStreamWidth() * 3 / 2];
//
//	memset(_data, 0, data->GetStreamHeight() * data->GetStreamWidth() * 3 / 2);
//
//	// Copy Y buffer
//	memcpy(_data, data->GetYBuffer(), data->GetStreamHeight() * data->GetStreamWidth());
//
//	// Copy U buffer
//	size_t loc = data->GetStreamHeight() * data->GetStreamWidth();
//	memcpy(&_data[loc], data->GetUBuffer(), data->GetStreamHeight() * data->GetStreamWidth() / 4);
//
//
//	// Copy V buffer
//	loc = (data->GetStreamHeight() * data->GetStreamWidth()) + (data->GetStreamHeight() * data->GetStreamWidth() / 4);
//	memcpy(&_data[loc], data->GetVBuffer(), data->GetStreamHeight() * data->GetStreamWidth() / 4);
//
//
//
//	//outputFile.write((char*)data->GetBuffer(), data->GetBufferLen());
//	// Write the Y plane
//	outputFile.write(_data, data->GetStreamHeight() * data->GetStreamWidth() * 3 / 2);
//
//
//
//	// Close the file
//	outputFile.close();
//	outputFile.flush();
//	//cout << "YUV420 buffer saved to file." << endl;
//}
void ZoomSDKRenderer::SaveToRawYUVFile(YUVRawDataI420* data) {

	// Open the file for writing
	std::ofstream outputFile("output.yuv", std::ios::out | std::ios::binary | std::ios::app);
	if (!outputFile.is_open())
	{
		std::cout << "Error opening file." << std::endl;
		return;
	}
	// Calculate the sizes for Y, U, and V components
	size_t ySize = data->GetStreamWidth() * data->GetStreamHeight();
	size_t uvSize = ySize / 4;

	// Write Y, U, and V components to the output file
	outputFile.write(data->GetYBuffer(), ySize);
	outputFile.write(data->GetUBuffer(), uvSize);
	outputFile.write(data->GetVBuffer(), uvSize);


	// Close the file
	outputFile.close();
	outputFile.flush();
	//cout << "YUV420 buffer saved to file." << endl;
}




    // Function to save YUVRawDataI420 to an MP4 file using the MPEG4 codec
    void ZoomSDKRenderer::SaveYUVRawDataToMP4( YUVRawDataI420 * data) {
        // Initialize FFmpeg libraries
        av_register_all();
        avcodec_register_all();
        avformat_network_init();

        // Create a new format context for the output file
        AVFormatContext* formatContext = nullptr;
        if (avformat_alloc_output_context2(&formatContext, nullptr, nullptr, "output.mp4") < 0) {
            std::cerr << "Error creating format context." << std::endl;
            return;
        }

        // Find the codec for MPEG4
        AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
        if (!codec) {
            std::cerr << "Codec not found." << std::endl;
            return;
        }

        // Create a new codec context and set its parameters
        AVCodecContext* codecContext = avcodec_alloc_context3(codec);
        if (!codecContext) {
            std::cerr << "Error creating codec context." << std::endl;
            return;
        }
        
        // Set codec parameters
        codecContext->bit_rate = 400000;
        codecContext->width = data->GetStreamWidth();
        codecContext->height = data->GetStreamHeight();
        codecContext->time_base.num = 1;
        codecContext->time_base.den = 15; // Assuming 30 frames per second
        codecContext->gop_size = 10;     // Group of pictures size
        codecContext->max_b_frames = 1;
        codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
        codecContext->codec_type = AVMEDIA_TYPE_VIDEO;

        // Open codec
        if (avcodec_open2(codecContext, codec, nullptr) < 0) {
            std::cerr << "Error opening codec." << std::endl;
            return;
        }

        // Create a new video stream in the format context
        AVStream* stream = avformat_new_stream(formatContext, codec);
        if (!stream) {
            std::cerr << "Error creating stream." << std::endl;
            return;
        }

        // Write the stream header to the output file
        if (avformat_write_header(formatContext, nullptr) < 0) {
            std::cerr << "Error writing header." << std::endl;
            return;
        }

        // Encode and write frames to the output file
        // Note: You may need to adapt this part to properly handle your YUV data and frame
        AVFrame* frame = av_frame_alloc();
        if (!frame) {
            std::cerr << "Error allocating frame." << std::endl;
            return;
        }

        // Fill the frame with your YUV data (from YUVRawDataI420)

        // Initialize the packet
        AVPacket packet;
        av_init_packet(&packet);
        packet.data = nullptr;
        packet.size = 0;

        if (avcodec_encode_video2(codecContext, &packet, frame, nullptr) < 0) {
            std::cerr << "Error encoding frame." << std::endl;
            return;
        }

        if (packet.size > 0) {
            // Write the packet to the output file
            if (av_write_frame(formatContext, &packet) < 0) {
                std::cerr << "Error writing frame." << std::endl;
            }
        }

        // Write the trailer to the output file
        av_write_trailer(formatContext);

        // Clean up resources
        avcodec_close(codecContext);
        avformat_close_input(&formatContext);
        av_frame_free(&frame);
    }