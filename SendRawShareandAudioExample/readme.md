# SendRawShareandAudioExample

This sample joins a meeting and injects prerecorded video and audio through the Meeting SDK raw share channels. The media appears as shared content and shared computer audio, not as the participant's camera and microphone.

See the [demo README](demo/readme.md) for the source-directory build details.

## What it demonstrates

- Starting an external share with `IZoomSDKShareSourceHelper::setExternalShareSource`.
- Sending decoded I420 frames with `IZoomSDKShareSender::sendShareFrame`.
- Sending paced 16-bit PCM with `IZoomSDKShareAudioSender::sendShareAudio`.
- Sharing video and user-defined audio together through one share session.
- Enabling each channel with `SendShareVideoRawData` and `SendShareAudioRawData` in `demo/config.json`.

The participant must be allowed to share in the meeting. The bundled video is decoded with FFmpeg, and the audio input must be a mono or stereo 16-bit PCM WAV file using a sample rate supported by the SDK.

## Build and run

```bash
cd demo
cp config.json.example config.json
cmake -B build
cmake --build build -j
./bin/run_SendRawShareandAudioDemo.sh
```

The sample uses the repository's shared `sdk/` directory. Update `demo/config.json` before running and see the [repository overview](../readme.md) for common setup.
