# SendRawShareandAudioExample Demo

This demo injects prerecorded video and PCM audio through the Meeting SDK share channel. The output appears as shared content and shared computer audio rather than camera and microphone media.

## Configure and build

From this directory:

```bash
cp config.json.example config.json
cmake -B build
cmake --build build -j
./bin/run_SendRawShareandAudioDemo.sh
```

Enable `SendShareVideoRawData` and `SendShareAudioRawData` in `config.json`. The sample uses FFmpeg for video decoding and requires a supported 16-bit PCM WAV input. The build expects the extracted Linux Meeting SDK in `../../sdk/`.
