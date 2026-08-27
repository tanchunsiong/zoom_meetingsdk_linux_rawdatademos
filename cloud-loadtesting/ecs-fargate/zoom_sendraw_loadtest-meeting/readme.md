# Zoom SendRaw Load Test Meeting

Single ECS Fargate runner image for both Meeting SDK join and host start flows.
The image contains two binaries and chooses one at runtime with `MEETING_MODE`.

- `MEETING_MODE=join`: join an existing meeting as a participant.
- `MEETING_MODE=start`: start a meeting as host and join with a ZAK token.

This folder is for authorized Zoom meeting load tests only.

## Image

Terraform-managed image:

```bash
<account-id>.dkr.ecr.<region>.amazonaws.com/zoom-sendraw-loadtest-meeting:latest
```

Build:

```bash
IMAGE="$(terraform -chdir=../infra/terraform output -raw runner_image)" ./scripts/build-image.sh
```

Build and push:

```bash
REGISTRY="$(terraform -chdir=../infra/terraform output -raw runner_ecr_repository_url)"
aws ecr get-login-password --region "$AWS_REGION" | docker login --username AWS --password-stdin "${REGISTRY%/*}"
PUSH_IMAGE=true IMAGE="${REGISTRY}:latest" ./scripts/build-image.sh
```

The image includes local `media/`, so regenerate media before building when
source videos change.

A clean checkout contains only `media/.gitkeep`. Populate `media/manifest.json`
and its referenced YUV/PCM pairs before building the ECR image; otherwise the
runner exits before joining.

## Runtime Parameters

Common:

```bash
MEETING_MODE=join|start
MEETING_NUMBER=1234567890
JWT_TOKEN=meeting-sdk-jwt-or-signature
ZOOM_USERNAME=LoadBot-1
SEND_VIDEO_RAW_DATA=true
SEND_AUDIO_RAW_DATA=true
CHAT_DEMO=true
EXIT_ON_MEETING_END=true
MEDIA_INDEX=-1
```

Join mode:

```bash
MEETING_MODE=join
MEETING_PASSWORD=passcode-if-required
JWT_ROLE=0
```

Start mode:

```bash
MEETING_MODE=start
USER_ZAK=host-zak-token
JWT_ROLE=1
```

The entrypoint writes `config.json`, starts PulseAudio, then executes:

- `ZoomSendRawLoadTestJoinMeetingDemo` for join mode.
- `ZoomSendRawLoadTestStartMeetingDemo` for start mode.

The container does not fetch Meeting SDK JWTs by default. Pass `JWT_TOKEN` at
runtime. For start mode, pass `USER_ZAK` at runtime.

## Local Helper

Join:

```bash
MEETING_MODE=join ./scripts/start-loadtest.sh 5 1234567890 "$JWT_TOKEN" "passcode" LoadBot
```

Start as host:

```bash
MEETING_MODE=start USER_ZAK="$USER_ZAK" ./scripts/start-loadtest.sh 1 1234567890 "$JWT_TOKEN" "" LoadHost
```

The helper applies these controls only to local Docker smoke tests. ECS task CPU
and memory are configured by Terraform and the Fargate manager:

- `CPU_MIN=0.1` -> Docker `--cpu-shares=102`
- `CPU_MAX=0.5` -> Docker `--cpus=0.5`
- `MEMORY_MIN=200m` -> Docker `--memory-reservation=200m`
- `MEMORY_MAX=500m` -> Docker `--memory=500m`

Stop local Docker containers:

```bash
./scripts/stop-loadtest.sh
```

Stop ECS Fargate tasks from the hosted manager UI or its `/api/run/kill` endpoint.

## Media

Source MP4 files go in `videosamples/`. Convert the first minute to 720p I420
YUV and 48 kHz mono PCM:

```bash
./scripts/convert-videosamples.sh
```

Useful override:

```bash
DURATION=60 WIDTH=1280 HEIGHT=720 SAFE_SCALE_PERCENT=40 ./scripts/convert-videosamples.sh
```

Generated media and source videos are local-only and ignored by git:

- `videosamples/*`
- `media/manifest.json`
- `media/*.yuv`
- `media/*.pcm`
- `*.mp4`, `*.wav`, `*.pcm`, `*.yuv`, `*.m4a`, `*.mp3`, `*.flac`

Only `.gitkeep` placeholders are tracked from media folders.
