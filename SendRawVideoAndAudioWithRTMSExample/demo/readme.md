# SendRawVideoAndAudioWithRTMSExample

This sample connects RTMS media flows to the Zoom Meeting SDK and injects decoded media
into a live meeting.

## What it covers

- Zoom Meeting SDK meeting join and auth flow
- RTMS event, signaling, and media WebSocket handling
- OAuth-based token fetches for RTMS services
- H.264 decode with FFmpeg and forwarding into the SDK virtual camera path
- Optional raw audio injection into the meeting

## Dependencies

Install the base Linux toolchain plus the RTMS-specific libraries:

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  pkg-config \
  libglib2.0-dev \
  libcurl4-openssl-dev \
  libssl-dev \
  libboost-all-dev \
  libavcodec-dev \
  libavutil-dev \
  ffmpeg
```

## SDK setup

This repository expects the Zoom Meeting SDK under repo-local `sdk/`.

Supported layouts:

```text
sdk/<sdk-files>
```

or:

```text
sdk/<version-directory>/<sdk-files>
```

The extracted SDK must contain:

- `h/`
- `qt_libs/`
- `json/`
- `images/`
- `cpthost`
- `libmeetingsdk.so`
- `libcml.so`
- `libmpg123.so`

## Build

From this directory:

```bash
cmake -B build
cmake --build build -j
```

If the SDK is not under repo-local `sdk/`, override it explicitly:

```bash
cmake -B build -DZOOM_MEETING_SDK_DIR=/path/to/sdk
cmake --build build -j
```

The build populates `bin/` with:

- `SendRawVideoAndAudioWithRTMSDemo`
- `run_SendRawVideoAndAudioWithRTMSDemo.sh`
- `run_cpthost.sh`
- `env.sh`
- copied SDK runtime assets

Use the launcher script:

```bash
./bin/run_SendRawVideoAndAudioWithRTMSDemo.sh
```

## Runtime configuration

Create local config files from the provided examples:

```bash
cp config.json.example config.json
cp .env.example .env
```

`config.json` controls meeting join and media behavior. Example fields:

```json
{
  "meeting_number": "1234567890",
  "token": "xxxxxx.yyyyyyyyy.zzzzzzzz",
  "meeting_password": "",
  "recording_token": "",
  "remote_url": "https://example.com/meeting-csdk/",
  "useJWTTokenFromWebService": "false",
  "useRecordingTokenFromWebService": "false",
  "SendVideoRawData": "false",
  "SendAudioRawData": "false"
}
```

`.env` holds RTMS-related credentials and endpoints. Keep real values out of git.

## Notes

- If `useJWTTokenFromWebService` is `true`, the sample expects your own token service.
- Raw video and audio injection still depends on the meeting role and account capability.
- The generated launcher script sets `LD_LIBRARY_PATH` so the SDK Qt libraries and
  `cpthost` resolve correctly.
- `websocketpp` in `external/` is a vendored third-party dependency.

## Debugging

The sample contains optional code paths that can dump raw YUV for inspection. If you
write YUV frames to disk, you can preview them with:

```bash
ffplay -f rawvideo -pixel_format yuv420p -video_size 1280x720 output_debug.yuv
```
