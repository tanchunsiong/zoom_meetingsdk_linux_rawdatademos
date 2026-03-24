
// #include "../ZoomSDKVideoSource.h"

// #include "media_ws.h"
// #include <websocketpp/config/asio_client.hpp>
// #include <websocketpp/client.hpp>
// #include <boost/asio/ssl/context.hpp>

// #include <iostream>
// #include "utils.h"

// #include "nlohmann/json.hpp"
// #include <websocketpp/base64/base64.hpp>

// extern "C" {
// #include <libavcodec/avcodec.h>
// #include <libavutil/imgutils.h>
// }


// static ZoomSDKVideoSource* gVideoSource = nullptr;


// AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
// AVCodecContext* codec_ctx = nullptr;


// extern std::string CLIENT_ID;
// extern std::string CLIENT_SECRET;

// using json = nlohmann::json;
// using client = websocketpp::client<websocketpp::config::asio_tls_client>;
// using message_ptr = websocketpp::config::asio_client::message_type::ptr;


// void set_video_source_reference(ZoomSDKVideoSource* ptr) {
//     gVideoSource = ptr;
// }

// // Call this once at startup
// bool init_decoder() {
//     static bool initialized = false;
//     if (initialized) return true;

//     avcodec_register_all(); // optional for newer FFmpeg
//     AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
//     if (!codec) {
//         std::cerr << "❌ H.264 decoder not found\n";
//         return false;
//     }

//     codec_ctx = avcodec_alloc_context3(codec);
//     if (!codec_ctx) {
//         std::cerr << "❌ Failed to allocate codec context\n";
//         return false;
//     }

//     if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
//         std::cerr << "❌ Failed to open H.264 codec\n";
//         return false;
//     }

//     std::cout << "✅ H.264 decoder initialized\n";
//     initialized = true;
//     return true;
// }

// // Call this every time you receive a decoded H264 frame
// void decode_h264_and_push(const uint8_t* h264_data, size_t h264_size, int width, int height) {
//     if (!init_decoder()) {
//         std::cerr << "❌ Decoder not initialized\n";
//         return;
//     }

//     AVPacket* packet = av_packet_alloc();
//     if (!packet) {
//         std::cerr << "❌ Failed to allocate packet\n";
//         return;
//     }

//     // Set up packet with your input data
//     packet->data = const_cast<uint8_t*>(h264_data);
//     packet->size = static_cast<int>(h264_size);

//     if (avcodec_send_packet(codec_ctx, packet) < 0) {
//         std::cerr << "❌ Failed to send packet\n";
//         av_packet_free(&packet);
//         return;
//     }

//     AVFrame* frame = av_frame_alloc();
//     if (!frame) {
//         std::cerr << "❌ Failed to allocate frame\n";
//         av_packet_free(&packet);
//         return;
//     }

//     // Receive all available frames
//     while (avcodec_receive_frame(codec_ctx, frame) == 0) {
//         if (frame->format != AV_PIX_FMT_YUV420P) {
//             std::cerr << "❌ Frame is not YUV420P (got " << frame->format << ")\n";
//             continue;
//         }

//         int y_size = frame->width * frame->height;
//         int uv_size = y_size / 4;
//         size_t total_size = y_size + 2 * uv_size;

//         std::vector<uint8_t> yuv_data(total_size);

//         // Copy Y, U, V planes into contiguous buffer
//         for (int i = 0; i < frame->height; ++i)
//             memcpy(yuv_data.data() + i * frame->width, frame->data[0] + i * frame->linesize[0], frame->width);

//         for (int i = 0; i < frame->height / 2; ++i)
//             memcpy(yuv_data.data() + y_size + i * frame->width / 2,
//                    frame->data[1] + i * frame->linesize[1], frame->width / 2);

//         for (int i = 0; i < frame->height / 2; ++i)
//             memcpy(yuv_data.data() + y_size + uv_size + i * frame->width / 2,
//                    frame->data[2] + i * frame->linesize[2], frame->width / 2);


//      gVideoSource->getFrameBuffer().push(yuv_data.data(), total_size);
//     }

//     av_frame_free(&frame);
//     av_packet_free(&packet);
// }

// void connect_to_media_server(const std::string& media_url, const std::string& meeting_uuid, const std::string& stream_id, websocketpp::connection_hdl signaling_hdl) {
    
    
//     using client = websocketpp::client<websocketpp::config::asio_tls_client>;
//     client c;
//     c.init_asio();

//     c.set_tls_init_handler([](websocketpp::connection_hdl) {
//         return websocketpp::lib::make_shared<boost::asio::ssl::context>(
//             boost::asio::ssl::context::tlsv12_client
//         );
//     });

//     c.set_open_handler([&](websocketpp::connection_hdl hdl) {
//         json handshake = {
//             {"msg_type", 3},
//             {"protocol_version", 1},
//             {"meeting_uuid", meeting_uuid},
//             {"rtms_stream_id", stream_id},
//             {"signature", generate_signature(CLIENT_ID, meeting_uuid, stream_id, CLIENT_SECRET)},
//             {"media_type", 32},
//             {"payload_encryption", false},
//             {"media_params", {
//                 {"audio", {
//                     {"content_type", 1}, {"sample_rate", 1}, {"channel", 1},
//                     {"codec", 1}, {"data_opt", 1}, {"send_rate", 100}
//                 }},
//                 {"video", {
//                     {"codec", 7}, {"resolution", 2}, {"fps", 25}
//                 }}
//             }}
//         };

//         c.send(hdl, handshake.dump(), websocketpp::frame::opcode::text);
//         std::cout << "🎬 Sent media handshake\n";
//     });

//     c.set_message_handler([&](websocketpp::connection_hdl hdl, message_ptr msg) {
//         auto payload = json::parse(msg->get_payload());
//         int msg_type = payload.value("msg_type", -1);

//         if (msg_type == 4 && payload.value("status_code", -1) == 0) {
//             std::cout << "✅ Media handshake success — starting stream\n";
//             json start_stream = {
//                 {"msg_type", 7},
//                 {"rtms_stream_id", stream_id}
//             };
//             c.send(signaling_hdl, start_stream.dump(), websocketpp::frame::opcode::text);
//         }  else if (msg_type == 14) {
//             //std::cout << "🔊 Received AUDIO data\n";
//         } 
        
//         else if (msg_type == 15) {
//     //std::cout << "🎥 Received VIDEO (H.264) data\n";

//     if (!payload.contains("content") || !payload["content"].is_object()) return;
//     const auto& content = payload["content"];

//     std::string encoded_data = content.value("data", "");
//     int width = content.value("width", 1280);
//     int height = content.value("height", 720);

//     if (encoded_data.empty()) {
//         std::cerr << "❌ Empty H.264 data string\n";
//         return;
//     }

//     std::string decoded_h264 = websocketpp::base64_decode(encoded_data);
//     const uint8_t* h264_data = reinterpret_cast<const uint8_t*>(decoded_h264.data());
//     size_t h264_size = decoded_h264.size();

//     decode_h264_and_push(h264_data, h264_size, width, height);
// }


//         else if (msg_type == 17) {
//             std::cout << "📝 Received TRANSCRIPT data\n";
//         }
//     });

//     websocketpp::lib::error_code ec;
//     auto con = c.get_connection(media_url, ec);
//     if (ec) {
//         std::cerr << "❌ Media connection error: " << ec.message() << "\n";
//         return;
//     }

//     c.connect(con);
//     c.run();
// }


#include "../ZoomSDKVideoSource.h"

#include "media_ws.h"
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <boost/asio/ssl/context.hpp>

#include <iostream>
#include "utils.h"

#include "nlohmann/json.hpp"
#include <websocketpp/base64/base64.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}

static ZoomSDKVideoSource* gVideoSource = nullptr;
static ZoomSDKVirtualAudioMicEvent* gAudioSource = nullptr;
AVCodecContext* codec_ctx = nullptr;

extern std::string CLIENT_ID;
extern std::string CLIENT_SECRET;

using json = nlohmann::json;
using client = websocketpp::client<websocketpp::config::asio_tls_client>;
using message_ptr = websocketpp::config::asio_client::message_type::ptr;

void set_video_source_reference(ZoomSDKVideoSource* ptr) {
    gVideoSource = ptr;
}

void set_audio_source_reference(ZoomSDKVirtualAudioMicEvent* ptr) {
    gAudioSource = ptr;
}

bool init_decoder() {
    static bool initialized = false;
    if (initialized) return true;

    avcodec_register_all();
    AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "❌ H.264 decoder not found\n";
        return false;
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        std::cerr << "❌ Failed to allocate codec context\n";
        return false;
    }

    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        std::cerr << "❌ Failed to open H.264 codec\n";
        return false;
    }

    std::cout << "✅ H.264 decoder initialized\n";
    initialized = true;
    return true;
}

void decode_h264_and_push(const uint8_t* h264_data, size_t h264_size) {
    if (!init_decoder()) {
        std::cerr << "❌ Decoder not initialized\n";
        return;
    }

    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        std::cerr << "❌ Failed to allocate packet\n";
        return;
    }

    packet->data = const_cast<uint8_t*>(h264_data);
    packet->size = static_cast<int>(h264_size);

    if (avcodec_send_packet(codec_ctx, packet) < 0) {
        std::cerr << "❌ Failed to send packet\n";
        av_packet_free(&packet);
        return;
    }

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        std::cerr << "❌ Failed to allocate frame\n";
        av_packet_free(&packet);
        return;
    }

    while (avcodec_receive_frame(codec_ctx, frame) == 0) {
        if (frame->format != AV_PIX_FMT_YUV420P) {
            std::cerr << "❌ Frame is not YUV420P (got " << frame->format << ")\n";
            continue;
        }

        std::cout << "🧪 Decoded frame: " << frame->width << "x" << frame->height
                  << ", linesize Y=" << frame->linesize[0]
                  << ", U=" << frame->linesize[1]
                  << ", V=" << frame->linesize[2] << "\n";

        int width = frame->width;
        int height = frame->height;
        int y_size = width * height;
        int uv_size = y_size / 4;
        size_t total_size = y_size + 2 * uv_size;

        std::vector<uint8_t> yuv_data(total_size);

        for (int i = 0; i < height; ++i)
            memcpy(yuv_data.data() + i * width, frame->data[0] + i * frame->linesize[0], width);

        for (int i = 0; i < height / 2; ++i)
            memcpy(yuv_data.data() + y_size + i * width / 2,
                   frame->data[1] + i * frame->linesize[1], width / 2);

        for (int i = 0; i < height / 2; ++i)
            memcpy(yuv_data.data() + y_size + uv_size + i * width / 2,
                   frame->data[2] + i * frame->linesize[2], width / 2);

        if (!gVideoSource) {
            std::cerr << "❌ gVideoSource is null — cannot push frame\n";
            break;
        }
        gVideoSource->getFrameBuffer().push(yuv_data.data(), total_size);
    }

    av_frame_free(&frame);
    av_packet_free(&packet);
}

void connect_to_media_server(const std::string& media_url, const std::string& meeting_uuid, const std::string& stream_id, websocketpp::connection_hdl signaling_hdl) {
    client c;
    c.init_asio();

    c.set_tls_init_handler([](websocketpp::connection_hdl) {
        return websocketpp::lib::make_shared<boost::asio::ssl::context>(
            boost::asio::ssl::context::tlsv12_client
        );
    });

    c.set_open_handler([&](websocketpp::connection_hdl hdl) {
        json handshake = {
            {"msg_type", 3},
            {"protocol_version", 1},
            {"meeting_uuid", meeting_uuid},
            {"rtms_stream_id", stream_id},
            {"signature", generate_signature(CLIENT_ID, meeting_uuid, stream_id, CLIENT_SECRET)},
            {"media_type", 32},
            {"payload_encryption", false},
            {"media_params", {
                {"audio", {
                    {"content_type", 1}, {"sample_rate", 1}, {"channel", 1},
                    {"codec", 1}, {"data_opt", 1}, {"send_rate", 100}
                }},
                {"video", {
                    {"codec", 7}, {"resolution", 2}, {"fps", 25}
                }}
            }}
        };

        c.send(hdl, handshake.dump(), websocketpp::frame::opcode::text);
        std::cout << "🎬 Sent media handshake\n";
    });

    c.set_message_handler([&](websocketpp::connection_hdl hdl, message_ptr msg) {
        auto payload = json::parse(msg->get_payload());
        int msg_type = payload.value("msg_type", -1);

        if (msg_type == 4 && payload.value("status_code", -1) == 0) {
            std::cout << "✅ Media handshake success — starting stream\n";
            json start_stream = {
                {"msg_type", 7},
                {"rtms_stream_id", stream_id}
            };
            c.send(signaling_hdl, start_stream.dump(), websocketpp::frame::opcode::text);
        } else if (msg_type == 14) {
            if (!payload.contains("content") || !payload["content"].is_object()) return;
            const auto& content = payload["content"];
            std::string encoded_data = content.value("data", "");
            if (encoded_data.empty()) {
                std::cerr << "❌ Empty Audio data string\n";
                return;
            }
            std::string decoded_pcm = websocketpp::base64_decode(encoded_data);
            const uint8_t* pcm_data = reinterpret_cast<const uint8_t*>(decoded_pcm.data());
            size_t pcm_size = decoded_pcm.size();

            gAudioSource->getAudioBuffer().push(pcm_data,pcm_size);

        }else if (msg_type == 15) {
            if (!payload.contains("content") || !payload["content"].is_object()) return;
            const auto& content = payload["content"];

            std::string encoded_data = content.value("data", "");
            if (encoded_data.empty()) {
                std::cerr << "❌ Empty H.264 data string\n";
                return;
            }

            std::string decoded_h264 = websocketpp::base64_decode(encoded_data);
            const uint8_t* h264_data = reinterpret_cast<const uint8_t*>(decoded_h264.data());
            size_t h264_size = decoded_h264.size();

            decode_h264_and_push(h264_data, h264_size);
        } else if (msg_type == 17) {
            std::cout << "📝 Received TRANSCRIPT data\n";
        }
    });

    websocketpp::lib::error_code ec;
    auto con = c.get_connection(media_url, ec);
    if (ec) {
        std::cerr << "❌ Media connection error: " << ec.message() << "\n";
        return;
    }

    c.connect(con);
    c.run();
}
