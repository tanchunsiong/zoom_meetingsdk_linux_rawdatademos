# Zoom SendRaw Load Test Join Meeting

Docker/Portainer-oriented load-test variant of `SendRawVideoAndAudioExample`.
Each container joins one Zoom meeting participant with a runtime Meeting SDK
JWT/signature and sends one randomly selected raw video/audio pair.

This folder is for authorized meeting load tests only.

## Meeting Start Behavior

This load-test image joins an existing meeting. It does not currently start or
schedule meetings as the host.

Start the meeting separately as host, then scale the Portainer replica count when
you want load-test participants to join.

`JWT_TOKEN`, `JWT_SIGNATURE`, or `SIGNATURE` must be passed at runtime. Docker
does not fetch the Meeting SDK JWT by default.

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

4. Push the image if Portainer pulls from the registry:

```bash
docker login dcr.asdc.cc
PUSH_IMAGE=true ./scripts/build-image.sh
```

Do not commit registry credentials. Keep them in Docker/Portainer credential
storage or enter them interactively with `docker login`.

5. In Portainer, run the image and set the replica count to the number of
participants you want.

## Build The Image

Run this from the load-test folder:

```bash
cd /home/dreamtcs/temp/zoom_meetingsdk_linux_rawdatademos_upstream_demo-compact_20260324/zoom_sendraw_loadtest-joinmeeting
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

The default image name is:

```bash
dcr.asdc.cc/zoom-sendraw-loadtest-joinmeeting:latest
```

Override it if needed:

```bash
IMAGE=dcr.asdc.cc/custom-name:tag ./scripts/build-image.sh
```

To build and push in one command:

```bash
PUSH_IMAGE=true ./scripts/build-image.sh
```

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
dcr.asdc.cc/zoom-sendraw-loadtest-joinmeeting:latest
```

Set the replica/instance count in Portainer to the number of meeting
participants you want.

Required environment variables:

```bash
MEETING_NUMBER=1234567890
JWT_TOKEN=
MEETING_PASSWORD=
ZOOM_USERNAME_PREFIX=LoadBot
```

`MEETING_PASSWORD` is required when the meeting has a passcode.

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
USE_JWT_TOKEN_FROM_WEB_SERVICE=false
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
./scripts/start-loadtest.sh 25 1234567890 'meeting-sdk-jwt-or-signature' 'meeting-passcode' LoadBot
./scripts/stop-loadtest.sh
```

The first argument is the container count. The second is the meeting number. The
third is the Meeting SDK JWT/signature. The fourth is the meeting passcode. The
fifth is the username prefix.

## Local CMake Build

From the raw-data demos repository root, this folder is available as the
`ZoomSendRawLoadTestJoinMeetingDemo` target:

```bash
cmake -S . -B build -DZOOM_MEETING_SDK_DIR=/home/dreamtcs/temp/sdk/zoom-meeting-sdk-linux_x86_64-6.7.5.7391
cmake --build build --target ZoomSendRawLoadTestJoinMeetingDemo -j
```

The generated launcher is:

```bash
zoom_sendraw_loadtest-joinmeeting/demo/bin/run_ZoomSendRawLoadTestJoinMeetingDemo.sh
```
