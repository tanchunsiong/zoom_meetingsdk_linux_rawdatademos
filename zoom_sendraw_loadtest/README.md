# Zoom SendRaw Load Test

Docker/Portainer-oriented load-test variant of `SendRawVideoAndAudioExample`.
Each container joins one Zoom meeting participant, fetches a Meeting SDK JWT from
`https://nodejs.asdc.cc/meeting` by default, and sends one randomly selected raw
video/audio pair.

This folder is for authorized meeting load tests only.

## What Is Committed

The code, Docker files, helper scripts, examples, and empty media folders are
tracked.

The following are intentionally local-only and ignored by git:

- `videosamples/*`
- `media/manifest.json`
- `media/*.yuv`
- `media/*.pcm`
- `demo/config.json`
- `.env`
- Any `.mp4`, `.wav`, `.pcm`, `.yuv`, `.m4a`, `.mp3`, or `.flac` files in this folder

Only `media/.gitkeep` and `videosamples/.gitkeep` are committed from the media
folders.

## Quick Workflow

1. Put source MP4 files in `videosamples/`.
2. Convert them into local raw media:

```bash
./scripts/convert-videosamples.sh
```

3. Build the Docker image:

```bash
./scripts/build-image.sh
```

4. In Portainer, run the image and set the replica count to the number of
participants you want.

## Build The Image

Run this from the load-test folder:

```bash
cd /home/dreamtcs/temp/zoom_meetingsdk_linux_rawdatademos_upstream_demo-compact_20260324/zoom_sendraw_loadtest
./scripts/build-image.sh
```

The build script stages the local Zoom Meeting SDK from:

```bash
/home/dreamtcs/temp/sdk/zoom-meeting-sdk-linux_x86_64-6.7.5.7391
```

Override it if needed:

```bash
SDK_SOURCE=/path/to/zoom-meeting-sdk-linux_x86_64-x.y.z ./scripts/build-image.sh
```

The Docker image includes `media/`, so regenerate media before building when you
change source videos.

## Media Conversion

Source MP4 files go in `videosamples/`. The conversion script writes 720p
I420/YUV video, 48 kHz mono signed 16-bit PCM audio, and `media/manifest.json`:

```bash
./scripts/convert-videosamples.sh
```

Useful conversion overrides:

```bash
DURATION=60 WIDTH=1280 HEIGHT=720 SAFE_SCALE_PERCENT=40 ./scripts/convert-videosamples.sh
```

`SAFE_SCALE_PERCENT=40` means the visible content is scaled to 40% of the
1280x720 canvas and centered with black padding.

## Portainer

Use this image:

```bash
zoom-sendraw-loadtest:latest
```

Set the replica/instance count in Portainer to the number of meeting
participants you want.

Required environment variables:

```bash
MEETING_NUMBER=1234567890
MEETING_PASSWORD=
ZOOM_USERNAME_PREFIX=LoadBot
JWT_ENDPOINT=https://nodejs.asdc.cc/meeting
```

Useful optional variables:

```bash
MEDIA_MANIFEST=../media/manifest.json
MEDIA_INDEX=-1
SEND_VIDEO_RAW_DATA=true
SEND_AUDIO_RAW_DATA=true
CHAT_DEMO=true
EXIT_ON_MEETING_END=true
VIDEO_WIDTH=1280
VIDEO_HEIGHT=720
VIDEO_FPS=30
AUDIO_SAMPLE_RATE=48000
AUDIO_CHANNELS=1
```

`MEDIA_INDEX=-1` randomly selects a pair from the manifest. Set it to a zero-based
index to force one specific video/audio pair.

If raw sending is enabled and the manifest or selected media files are missing,
the process exits instead of falling back to bundled sample media.

Set `SEND_VIDEO_RAW_DATA=false` and `SEND_AUDIO_RAW_DATA=false` only when you
want join-only participants.

## Local Docker Test

The helper scripts are available for direct Docker testing without Portainer:

```bash
./scripts/start-loadtest.sh 25 1234567890 'meeting-passcode' LoadBot
./scripts/stop-loadtest.sh
```

The first argument is the container count. The second is the meeting number. The
third is the meeting passcode. The fourth is the username prefix.

## Local CMake Build

From the raw-data demos repository root, this folder is available as the
`ZoomSendRawLoadTestDemo` target:

```bash
cmake -S . -B build -DZOOM_MEETING_SDK_DIR=/home/dreamtcs/temp/sdk/zoom-meeting-sdk-linux_x86_64-6.7.5.7391
cmake --build build --target ZoomSendRawLoadTestDemo -j
```

The generated launcher is:

```bash
zoom_sendraw_loadtest/demo/bin/run_ZoomSendRawLoadTestDemo.sh
```
