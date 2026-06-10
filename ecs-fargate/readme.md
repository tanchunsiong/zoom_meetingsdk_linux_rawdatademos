# Zoom Meeting SDK Linux Raw Data Demos - ECS Fargate Copy

This repository contains Linux C++ sample apps built on the Zoom Meeting SDK.
It has been updated to use a shared repo-local `sdk/` directory instead of copying
SDK headers and libraries into each demo.

This copy is the AWS ECS/Fargate variant for disposable load testing. It keeps
the raw Meeting SDK sample code, the unified send-raw runner, the Node manager,
and a minimal Terraform template that can create the AWS control-plane resources
from AWS credentials plus a region.

The current SDK target is `6.7.5.7391`. See `version.txt`.

## Included demos

- `AllInOneExample`: join a meeting, chat, and exercise raw audio/video send and receive paths
- `ChatExample`: join a meeting and receive chat events
- `GetRawVideoAndAudioExample`: subscribe to raw video and audio
- `GetRawVideoAndAudioAPIExample`: raw data sample with the included Python helper
- `SendRawVideoAndAudioExample`: send video and audio into a meeting
- `SendRawVideoAndAudioWithRTMSExample`: RTMS-driven media injection sample
- `SkeletonExample`: minimal join/auth sample
- `zoom_sendraw_loadtest-meeting`: single raw audio/video load-test runner image for both join and host start modes
- `zoom_loadtest_manager`: Node management website for Zoom S2S OAuth actions and ECS Fargate task control
- `infra/terraform`: minimal AWS resources for CloudFront, API Gateway, Lambda, ECS Fargate, ECR, DynamoDB, S3, SSM, and networking

## Repository layout

```text
.
├── sdk/
├── cmake/
├── version.txt
├── readme.md
├── infra/terraform/
├── zoom_sendraw_loadtest-meeting/
├── zoom_loadtest_manager/
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

Per-demo Dockerfiles are included under each sample directory for local builds.
The load-test path uses the unified runner image plus ECS Fargate one-off tasks.

The `zoom_sendraw_loadtest-meeting` folder is the unified load-test variant. It
builds one Docker image, accepts the Meeting SDK JWT/signature as a runtime
parameter, includes converted raw media pairs for randomized send tests, and
selects join vs host start with `MEETING_MODE=join|start`. See
`zoom_sendraw_loadtest-meeting/README.md`.

The `zoom_loadtest_manager` folder is a Node management website for creating and
selecting custCreate users, resolving their PMI/passcode, starting RTMS for
task-associated live meetings, fetching Meeting SDK JWT/signatures from the
configured `MEETING_TOKEN_ENDPOINT`, and starting/stopping the unified ECS
Fargate load-test tasks.
See `zoom_loadtest_manager/README.md`.

## AWS ECS/Fargate

The ECS copy is designed to scale to zero. There is no always-on runner service:
the manager starts Fargate tasks only when you request load-test instances.

The deployment machine must have AWS CLI v2, Terraform `>= 1.6.0`, and Docker
installed. Configure AWS credentials for a dedicated IAM user or temporary role,
then verify access with `aws sts get-caller-identity` before running Terraform.
Do not use root-user access keys.

Default runner size:

```text
0.25 vCPU / 0.5GB RAM
```

The Terraform template in `infra/terraform` creates the disposable AWS resources
for one region. Apply it separately per AWS region when you want independent
regional capacity. Resources are tagged with `Project`, `Stack`, and `ManagedBy`
so they can be found and destroyed later.

CloudWatch log groups, ECS log configuration, NAT Gateway, ALB, Secrets Manager,
and always-on ECS services are intentionally excluded.

## Sensitive Files

Do not commit local credentials, generated media, SDK binaries, Terraform state,
dependency folders, or local key material. The root `.gitignore` excludes:

- `.env` and local env variants
- `node_modules/`
- `.terraform/` and Terraform state files
- generated `.yuv`, `.pcm`, `.mp4`, `.wav`, `.m4a`, `.mp3`, and `.flac` media
- local certificate/key files such as `.pem`, `.key`, `.p12`, and `.pfx`

## WSL / IDE notes

- `readme for vs code and WSL.md`
- `readme for visual studio support.md`
