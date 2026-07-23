# Zoom Meeting SDK Linux Raw Data Demos

This repository contains Linux C++ sample apps built on the Zoom Meeting SDK.
It has been updated to use a shared repo-local `sdk/` directory instead of copying
SDK headers and libraries into each demo.

The current SDK target is `6.7.5.7391`. See `version.txt`.

## Included demos

- [AllInOneExample](AllInOneExample/readme.md): join a meeting, chat, and exercise raw audio/video send and receive paths
- [ChatExample](ChatExample/readme.md): join a meeting and demonstrate chat messaging
- [GetRawShareExample](GetRawShareExample/readme.md): subscribe to raw shared-screen frames
- [GetRawVideoAndAudioExample](GetRawVideoAndAudioExample/readme.md): subscribe to raw video and audio
- [GetRawVideoAndAudioAPIExample](GetRawVideoAndAudioAPIExample/readme.md): raw data sample with the included Python helper
- [SendRawVideoAndAudioExample](SendRawVideoAndAudioExample/readme.md): send video and audio into a meeting
- [SendRawShareandAudioExample](SendRawShareandAudioExample/readme.md): send video and audio through the meeting share channel
- [SendRawVideoAndAudioWithRTMSExample](SendRawVideoAndAudioWithRTMSExample/readme.md): RTMS-driven media injection sample
- [SkeletonExample](SkeletonExample/readme.md): minimal join/auth sample
- `cloud-loadtesting/on-premise/zoom_sendraw_loadtest-meeting`: single Docker/Portainer raw audio/video load-test image for both join and host start modes
- `cloud-loadtesting/on-premise/zoom_loadtest_manager`: Node management website for Zoom S2S OAuth actions and Docker run control
- `cloud-loadtesting/ecs-fargate`: source-only ECS/Fargate copy for AWS scale-to-zero load testing
- `cloud-loadtesting/azure-container-apps`: source-only Azure Container Apps Jobs copy for Azure scale-to-zero load testing

## Repository layout

```text
.
├── sdk/
├── cmake/
├── version.txt
├── readme.md
├── cloud-loadtesting/
│   ├── azure-container-apps/
│   ├── ecs-fargate/
│   └── on-premise/
│       ├── zoom_sendraw_loadtest-meeting/
│       └── zoom_loadtest_manager/
└── <DemoName>/demo
```

`sdk/` is ignored by git. You can place the extracted Zoom Meeting SDK files there
directly, or keep the extracted version inside a single child directory under `sdk/`.
Both layouts are supported.

Valid layouts:

```text
sdk/
├── h/
├── qt_libs/
├── json/
├── images/
├── cpthost
├── libmeetingsdk.so
├── libcml.so
└── libmpg123.so
```

or:

```text
sdk/
└── zoom-meeting-sdk-linux_x86_64-6.7.5.7391/
    ├── h/
    ├── qt_libs/
    ├── json/
    ├── images/
    ├── cpthost
    ├── libmeetingsdk.so
    ├── libcml.so
    └── libmpg123.so
```

## Build prerequisites

These samples are developed for Linux environments such as Ubuntu 22 and CentOS 8/9.
Install the base toolchain first:

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  pkg-config \
  libglib2.0-dev \
  libcurl4-openssl-dev \
  pulseaudio \
  pulseaudio-utils \
  ffmpeg
```

Some demos may also need additional platform packages such as X11/XCB runtime libs,
Boost, or FFmpeg development packages. The RTMS sample has extra dependencies listed
in `SendRawVideoAndAudioWithRTMSExample/demo/readme.md`.

## Build a demo

Example for `AllInOneExample`:

```bash
cd AllInOneExample/demo
cmake -B build
cmake --build build -j
```

The build copies the required SDK runtime assets into `demo/bin/`.

If your SDK is not under repo-local `sdk/`, you can override the path:

```bash
cmake -B build -DZOOM_MEETING_SDK_DIR=/path/to/sdk
cmake --build build -j
```

## Run a demo

Each demo build generates its binary in `demo/bin/` together with helper launchers:

- `run_<DemoName>.sh`
- `run_cpthost.sh`
- `env.sh`

Use the launcher script instead of running the binary directly:

```bash
./bin/run_AllInOneDemo.sh
```

The wrapper sets `LD_LIBRARY_PATH` so the bundled Qt libraries and `cpthost` resolve
correctly at runtime.

## Configuration

Each demo now ships a `config.json.example`. Copy it to `config.json` in the same
directory and fill in your own values.

Typical workflow:

```bash
cp config.json.example config.json
```

Common fields:

- `meeting_number`
- `token`
- `meeting_password`
- `recording_token`
- `remote_url`
- `useJWTTokenFromWebService`
- `useRecordingTokenFromWebService`

Some demos also expose raw-data flags such as:

- `GetVideoRawData`
- `GetAudioRawData`
- `SendVideoRawData`
- `SendAudioRawData`

The config file is intentionally ignored by git.

## Notes

- Fill in the display name in the demo source where required, for example
  `withoutloginParam.userName`.
- Raw audio and video subscriptions only work after the meeting join succeeds.
- Starting raw recording requires host, co-host, or explicit local recording privilege.
- Some Linux environments still require a `zoomus.conf` under the user config directory
  for audio-related behavior. The setup varies by distro and container environment.
- If you are using an unpublished Meeting SDK app, meeting join behavior is limited to
  the app's allowed account scope.

## Docker

Per-demo Dockerfiles are included under each sample directory. See
`readme for docker.md` for example build and run commands.

The `cloud-loadtesting/on-premise/zoom_sendraw_loadtest-meeting` folder is the unified load-test variant. It
builds one Docker image, accepts the Meeting SDK JWT/signature as a runtime
parameter, includes converted raw media pairs for randomized send tests, and
selects join vs host start with `MEETING_MODE=join|start`. See
`cloud-loadtesting/on-premise/zoom_sendraw_loadtest-meeting/README.md`.

The `cloud-loadtesting/on-premise/zoom_loadtest_manager` folder is a Node management website for creating and
selecting custCreate users, resolving their PMI/passcode, starting RTMS for
container-associated live meetings, fetching Meeting SDK JWT/signatures from the
configured `MEETING_TOKEN_ENDPOINT`, and starting/killing the unified Docker
load-test image.
See `cloud-loadtesting/on-premise/zoom_loadtest_manager/README.md`.

The `cloud-loadtesting/ecs-fargate` folder contains the AWS variant as a child copy of this repo.
It is committed as source only: generated builds, local env files, raw/audio/video
media, Terraform state/cache, SDK binaries, compressed third-party archives, and
dependency folders are excluded.
See `cloud-loadtesting/ecs-fargate/readme.md` and `cloud-loadtesting/ecs-fargate/infra/terraform/README.md`.

The `cloud-loadtesting/azure-container-apps` folder contains the Azure variant. It uses manually
triggered Azure Container Apps Jobs for scale-to-zero runner executions, ACR for
the runner image, and a resource group as the cleanup boundary.
See `cloud-loadtesting/azure-container-apps/readme.md` and `cloud-loadtesting/azure-container-apps/infra/terraform/README.md`.

## WSL / IDE notes

- `readme for vs code and WSL.md`
- `readme for visual studio support.md`
