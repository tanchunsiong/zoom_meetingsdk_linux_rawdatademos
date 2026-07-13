# SendRawVideoAndAudioWithRTMSExample

This sample receives media from Zoom Realtime Media Streams (RTMS), decodes it, and injects it into a Meeting SDK session as external video and optional external audio.

## What it demonstrates

- Connecting to RTMS signaling and media WebSockets.
- Authenticating and handling RTMS stream events.
- Decoding H.264 media with FFmpeg.
- Forwarding media to an external video source and virtual microphone.

This sample also requires OpenSSL, Boost, WebSocket++, FFmpeg, and RTMS credentials. See the [detailed demo README](demo/readme.md) for dependency and environment configuration.

## Build and run

```bash
cd demo
cp config.json.example config.json
cp .env.example .env
cmake -B build
cmake --build build -j
./bin/run_SendRawVideoAndAudioWithRTMSDemo.sh
```

The sample uses the repository's shared `sdk/` directory. Update `demo/config.json` and `demo/.env` before running, do not commit credentials, and see the [repository overview](../readme.md) for common setup.
