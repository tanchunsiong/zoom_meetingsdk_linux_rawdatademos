#include "ZoomSDKShareAudioSource.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <thread>
#include <utility>
#include <vector>

namespace {

struct WaveData {
	std::vector<char> pcm;
	uint32_t sample_rate = 0;
	uint16_t channels = 0;
	uint16_t bits_per_sample = 0;
};

uint16_t ReadU16(const char* value)
{
	return static_cast<uint16_t>(static_cast<unsigned char>(value[0])) |
		(static_cast<uint16_t>(static_cast<unsigned char>(value[1])) << 8);
}

uint32_t ReadU32(const char* value)
{
	return static_cast<uint32_t>(static_cast<unsigned char>(value[0])) |
		(static_cast<uint32_t>(static_cast<unsigned char>(value[1])) << 8) |
		(static_cast<uint32_t>(static_cast<unsigned char>(value[2])) << 16) |
		(static_cast<uint32_t>(static_cast<unsigned char>(value[3])) << 24);
}

bool LoadWaveFile(const std::string& path, WaveData& wave)
{
	std::ifstream input(path, std::ios::binary);
	char header[12];
	if (!input.read(header, sizeof(header)) ||
		std::memcmp(header, "RIFF", 4) != 0 ||
		std::memcmp(header + 8, "WAVE", 4) != 0) {
		return false;
	}

	bool found_format = false;
	bool found_data = false;
	while (input && (!found_format || !found_data)) {
		char chunk_header[8];
		if (!input.read(chunk_header, sizeof(chunk_header))) {
			break;
		}

		const uint32_t chunk_size = ReadU32(chunk_header + 4);
		if (std::memcmp(chunk_header, "fmt ", 4) == 0) {
			std::vector<char> format(chunk_size);
			if (!input.read(format.data(), chunk_size) || chunk_size < 16) {
				return false;
			}
			const uint16_t audio_format = ReadU16(format.data());
			wave.channels = ReadU16(format.data() + 2);
			wave.sample_rate = ReadU32(format.data() + 4);
			wave.bits_per_sample = ReadU16(format.data() + 14);
			found_format = audio_format == 1;
		} else if (std::memcmp(chunk_header, "data", 4) == 0) {
			wave.pcm.resize(chunk_size);
			if (!input.read(wave.pcm.data(), chunk_size)) {
				return false;
			}
			found_data = true;
		} else {
			input.seekg(chunk_size, std::ios::cur);
		}

		if (chunk_size % 2 != 0) {
			input.seekg(1, std::ios::cur);
		}
	}

	return found_format && found_data && wave.bits_per_sample == 16 &&
		(wave.channels == 1 || wave.channels == 2);
}

void StreamShareAudio(
	IZoomSDKShareAudioSender* sender,
	const std::string& audio_source,
	std::atomic<bool>* sending)
{
	WaveData wave;
	if (!LoadWaveFile(audio_source, wave)) {
		std::cerr << "Share audio must be a mono or stereo 16-bit PCM WAV file: "
				  << audio_source << std::endl;
		sending->store(false);
		return;
	}

	const ZoomSDKAudioChannel channel =
		wave.channels == 2 ? ZoomSDKAudioChannel_Stereo : ZoomSDKAudioChannel_Mono;
	const size_t bytes_per_20ms =
		static_cast<size_t>(wave.sample_rate) * wave.channels * sizeof(int16_t) / 50;

	while (sending->load()) {
		for (size_t offset = 0; offset < wave.pcm.size() && sending->load();
			 offset += bytes_per_20ms) {
			const size_t remaining = wave.pcm.size() - offset;
			const unsigned int length =
				static_cast<unsigned int>(std::min(bytes_per_20ms, remaining));
			SDKError error = sender->sendShareAudio(
				wave.pcm.data() + offset,
				length,
				static_cast<int>(wave.sample_rate),
				channel);
			if (error != SDKERR_SUCCESS) {
				std::cerr << "sendShareAudio failed: " << error << std::endl;
				sending->store(false);
				return;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		}
	}
}

} // namespace

ZoomSDKShareAudioSource::ZoomSDKShareAudioSource(std::string audio_source)
	: audio_source_(std::move(audio_source))
{
}

void ZoomSDKShareAudioSource::onStartSendAudio(IZoomSDKShareAudioSender* sender)
{
	std::cout << "Share audio channel started" << std::endl;
	sender_ = sender;
	if (sender_ && !sending_.exchange(true)) {
		std::thread(StreamShareAudio, sender_, audio_source_, &sending_).detach();
	}
}

void ZoomSDKShareAudioSource::onStopSendAudio()
{
	std::cout << "Share audio channel stopped" << std::endl;
	sending_.store(false);
	sender_ = nullptr;
}
