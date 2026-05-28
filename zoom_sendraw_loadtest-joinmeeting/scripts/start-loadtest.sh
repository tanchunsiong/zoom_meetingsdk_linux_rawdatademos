#!/usr/bin/env bash
set -euo pipefail

usage() {
  echo "Usage: $0 COUNT MEETING_NUMBER JWT_TOKEN [MEETING_PASSWORD] [USERNAME_PREFIX]" >&2
  echo "Example: $0 25 1234567890 'meeting-sdk-jwt-or-signature' 'passcode' LoadBot" >&2
}

if [[ $# -lt 3 ]]; then
  usage
  exit 64
fi

COUNT=$1
MEETING_NUMBER=$2
SDK_JWT=${3:-${JWT_TOKEN:-${JWT_SIGNATURE:-${SIGNATURE:-}}}}
MEETING_PASSWORD=${4:-${MEETING_PASSWORD:-}}
USERNAME_PREFIX=${5:-${ZOOM_USERNAME_PREFIX:-ZoomLoadBot}}

if [[ ! "$COUNT" =~ ^[0-9]+$ || "$COUNT" -lt 1 ]]; then
  echo "COUNT must be a positive integer." >&2
  exit 64
fi

if [[ -z "$SDK_JWT" ]]; then
  echo "JWT_TOKEN is required." >&2
  exit 64
fi

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
IMAGE=${IMAGE:-dcr.asdc.cc/zoom-sendraw-loadtest-joinmeeting:latest}
PROJECT=${PROJECT:-zoom-loadtest-joinmeeting}
RUN_ID=${RUN_ID:-$(date +%Y%m%d%H%M%S)}
JWT_ENDPOINT=${JWT_ENDPOINT:-https://nodejs.asdc.cc/meeting}
JWT_ROLE=${JWT_ROLE:-0}
RESTART_POLICY=${RESTART_POLICY:-no}
SHM_SIZE=${SHM_SIZE:-256m}

for i in $(seq 1 "$COUNT"); do
  name="${PROJECT}-${RUN_ID}-${i}"
  docker rm -f "$name" >/dev/null 2>&1 || true

  args=(
    docker run -d
    --name "$name"
    --restart "$RESTART_POLICY"
    --shm-size "$SHM_SIZE"
    --label "zoom-loadtest=true"
    --label "zoom-loadtest.project=${PROJECT}"
    --label "zoom-loadtest.run-id=${RUN_ID}"
  )

  if [[ -f "${ROOT_DIR}/.env" ]]; then
    args+=(--env-file "${ROOT_DIR}/.env")
  fi

  if [[ -n "${CPUS:-}" ]]; then
    args+=(--cpus "$CPUS")
  fi

  if [[ -n "${MEMORY:-}" ]]; then
    args+=(--memory "$MEMORY")
  fi

  if [[ -n "${DOCKER_NETWORK:-}" ]]; then
    args+=(--network "$DOCKER_NETWORK")
  fi

  args+=(
    -e "MEETING_NUMBER=${MEETING_NUMBER}"
    -e "MEETING_PASSWORD=${MEETING_PASSWORD}"
    -e "ZOOM_USERNAME=${USERNAME_PREFIX}-${i}"
    -e "ZOOM_INSTANCE_ID=${i}"
    -e "JWT_TOKEN=${SDK_JWT}"
    -e "JWT_ENDPOINT=${JWT_ENDPOINT}"
    -e "JWT_ROLE=${JWT_ROLE}"
    -e "USE_JWT_TOKEN_FROM_WEB_SERVICE=${USE_JWT_TOKEN_FROM_WEB_SERVICE:-false}"
    -e "USE_RECORDING_TOKEN_FROM_WEB_SERVICE=${USE_RECORDING_TOKEN_FROM_WEB_SERVICE:-false}"
    -e "SEND_VIDEO_RAW_DATA=${SEND_VIDEO_RAW_DATA:-true}"
    -e "SEND_AUDIO_RAW_DATA=${SEND_AUDIO_RAW_DATA:-true}"
    -e "CHAT_DEMO=${CHAT_DEMO:-true}"
    -e "EXIT_ON_MEETING_END=${EXIT_ON_MEETING_END:-true}"
    "$IMAGE"
  )

  container_id=$("${args[@]}")
  echo "${name} ${container_id}"
done

echo "Started ${COUNT} container(s) for run ${RUN_ID}."
