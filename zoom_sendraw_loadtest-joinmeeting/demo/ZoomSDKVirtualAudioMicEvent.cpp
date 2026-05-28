//SendAudioRawData
#include <iostream>
#include <cstdint>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <atomic>

#include "rawdata/rawdata_audio_helper_interface.h"
#include "ZoomSDKVirtualAudioMicEvent.h"
#include "zoom_sdk_def.h"

#include <thread>
#include <chrono>

using namespace std;
using namespace ZOOM_SDK_NAMESPACE;

int audio_play_flag = -1;

struct AudioPcmData {
	vector<char> pcm;
	int sample_rate = 0;
	ZoomSDKAudioChannel channel = ZoomSDKAudioChannel_Mono;
	int bytes_per_frame = 0;
};

static uint16_t ReadLE16(const char* value) {
	return static_cast<uint16_t>(
		(static_cast<unsigned char>(value[1]) << 8) |
		static_cast<unsigned char>(value[0])
	);
}

static uint32_t ReadLE32(const char* value) {
	return (static_cast<uint32_t>(static_cast<unsigned char>(value[3])) << 24) |
		(static_cast<uint32_t>(static_cast<unsigned char>(value[2])) << 16) |
		(static_cast<uint32_t>(static_cast<unsigned char>(value[1])) << 8) |
		static_cast<uint32_t>(static_cast<unsigned char>(value[0]));
}

static bool EndsWith(const std::string& value, const std::string& suffix) {
	return value.size() >= suffix.size() &&
		value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static bool LoadRawPcm(const string& audio_source, int sample_rate, int channels, AudioPcmData& audio) {
	ifstream file(audio_source, ios::binary | ios::ate);
	if (!file.is_open()) {
		std::cout << "Error: File not found. Tried to open " << audio_source << std::endl;
		return false;
	}

	const streamsize file_size = file.tellg();
	if (file_size <= 0) {
		std::cout << "Error: Empty PCM file " << audio_source << std::endl;
		return false;
	}

	if (channels != 1 && channels != 2) {
		std::cout << "Error: PCM channels must be 1 or 2. Got " << channels << std::endl;
		return false;
	}

	audio.pcm.resize(static_cast<size_t>(file_size));
	file.seekg(0, ios::beg);
	if (!file.read(audio.pcm.data(), file_size)) {
		return false;
	}

	audio.sample_rate = sample_rate;
	audio.channel = channels == 2 ? ZoomSDKAudioChannel_Stereo : ZoomSDKAudioChannel_Mono;
	audio.bytes_per_frame = channels * 2;
	return true;
}

static bool LoadWavPcm(const string& audio_source, AudioPcmData& wav) {
	ifstream file(audio_source, ios::binary);
	if (!file.is_open()) {
		std::cout << "Error: File not found. Tried to open " << audio_source << std::endl;
		return false;
	}

	char riff_header[12];
	if (!file.read(riff_header, sizeof(riff_header)) ||
		std::strncmp(riff_header, "RIFF", 4) != 0 ||
		std::strncmp(riff_header + 8, "WAVE", 4) != 0) {
		std::cout << "Error: Unsupported WAV file " << audio_source << std::endl;
		return false;
	}

	bool found_fmt = false;
	bool found_data = false;
	uint16_t audio_format = 0;
	uint16_t channel_count = 0;
	uint16_t bits_per_sample = 0;

	while (file) {
		char chunk_header[8];
		if (!file.read(chunk_header, sizeof(chunk_header))) {
			break;
		}

		uint32_t chunk_size = ReadLE32(chunk_header + 4);
		if (std::strncmp(chunk_header, "fmt ", 4) == 0) {
			vector<char> fmt(chunk_size);
			if (!file.read(fmt.data(), chunk_size) || chunk_size < 16) {
				return false;
			}

			audio_format = ReadLE16(fmt.data());
			channel_count = ReadLE16(fmt.data() + 2);
			wav.sample_rate = static_cast<int>(ReadLE32(fmt.data() + 4));
			bits_per_sample = ReadLE16(fmt.data() + 14);
			found_fmt = true;
		}
		else if (std::strncmp(chunk_header, "data", 4) == 0) {
			wav.pcm.resize(chunk_size);
			if (!file.read(wav.pcm.data(), chunk_size)) {
				return false;
			}
			found_data = true;
		}
		else {
			file.seekg(chunk_size, ios::cur);
		}

		if (chunk_size % 2 == 1) {
			file.seekg(1, ios::cur);
		}
	}

	if (!found_fmt || !found_data || audio_format != 1 || bits_per_sample != 16 ||
		(channel_count != 1 && channel_count != 2) || wav.pcm.empty()) {
		std::cout << "Error: WAV must be 16-bit PCM mono or stereo: " << audio_source << std::endl;
		return false;
	}

	wav.channel = channel_count == 2 ? ZoomSDKAudioChannel_Stereo : ZoomSDKAudioChannel_Mono;
	wav.bytes_per_frame = channel_count * 2;
	return true;
}

void PlayAudioFileToVirtualMic(IZoomSDKAudioRawDataSender* audio_sender, string audio_source, int sample_rate, int channels)
{
	AudioPcmData audio;
	bool loaded = EndsWith(audio_source, ".pcm")
		? LoadRawPcm(audio_source, sample_rate, channels, audio)
		: LoadWavPcm(audio_source, audio);
	if (!loaded) {
		return;
	}

	const int chunk_ms = 10;
	const size_t frames_per_chunk = std::max<size_t>(1, static_cast<size_t>(audio.sample_rate * chunk_ms / 1000));
	const size_t chunk_bytes = frames_per_chunk * audio.bytes_per_frame;
	std::cout << "Streaming audio file " << audio_source << " at " << audio.sample_rate << " Hz" << std::endl;

	while (audio_play_flag > 0 && audio_sender) {
		for (size_t offset = 0; audio_play_flag > 0 && offset < audio.pcm.size(); offset += chunk_bytes) {
			size_t bytes_to_send = std::min(chunk_bytes, audio.pcm.size() - offset);
			bytes_to_send -= bytes_to_send % 2;
			if (bytes_to_send == 0) {
				break;
			}

			SDKError err = audio_sender->send(audio.pcm.data() + offset, static_cast<unsigned int>(bytes_to_send), audio.sample_rate, audio.channel);
			if (err != SDKERR_SUCCESS) {
				std::cout << "Error: Failed to send audio data to virtual mic. Error code: " << err << std::endl;
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}

			const int frames_sent = static_cast<int>(bytes_to_send / audio.bytes_per_frame);
			const int sleep_ms = std::max(1, frames_sent * 1000 / audio.sample_rate);
			std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
		}
	}
}

void ZoomSDKVirtualAudioMicEvent::onMicInitialize(IZoomSDKAudioRawDataSender* pSender) {
	pSender_ = pSender;
	printf("ZoomSDKVirtualAudioMicEvent OnMicInitialize; waiting for onMicStartSend\n");
}

void ZoomSDKVirtualAudioMicEvent::onMicStartSend() {
	printf("onMicStartSend\n");
	startAudioThread();
}

void ZoomSDKVirtualAudioMicEvent::startAudioThread() {
	if (pSender_ && audio_play_flag != 1) {
		audio_play_flag = 1;
		audio_thread_ = thread(PlayAudioFileToVirtualMic, pSender_, audio_source_, sample_rate_, channels_);
	}
}

void ZoomSDKVirtualAudioMicEvent::onMicStopSend() {
	printf("onMicStopSend\n");
	audio_play_flag = 0;
	if (audio_thread_.joinable()) {
		audio_thread_.join();
	}
}

void ZoomSDKVirtualAudioMicEvent::onMicUninitialized() {
	std::cout << "onMicUninitialized" << std::endl;
	audio_play_flag = 0;
	if (audio_thread_.joinable()) {
		audio_thread_.join();
	}
	pSender_ = nullptr;
}

ZoomSDKVirtualAudioMicEvent::~ZoomSDKVirtualAudioMicEvent()
{
	audio_play_flag = 0;
	if (audio_thread_.joinable()) {
		audio_thread_.join();
	}
}

ZoomSDKVirtualAudioMicEvent::ZoomSDKVirtualAudioMicEvent(std::string audio_source, int sample_rate, int channels)
{
	pSender_ = nullptr;
	audio_source_ = audio_source;
	sample_rate_ = sample_rate;
	channels_ = channels;
}
