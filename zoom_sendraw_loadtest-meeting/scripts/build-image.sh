#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
IMAGE=${IMAGE:-dcr.asdc.cc/zoom-sendraw-loadtest-meeting:latest}
SDK_SOURCE=${SDK_SOURCE:-/home/dreamtcs/temp/sdk/zoom-meeting-sdk-linux_x86_64-6.7.5.7391}
SDK_STAGE_ROOT="${ROOT_DIR}/.docker-sdk"
SDK_STAGE="${SDK_STAGE_ROOT}/$(basename "$SDK_SOURCE")"

if [[ ! -d "$SDK_SOURCE" ]]; then
  echo "SDK_SOURCE does not exist or is not a directory: ${SDK_SOURCE}" >&2
  exit 66
fi

rm -rf "$SDK_STAGE_ROOT"
mkdir -p "$SDK_STAGE_ROOT"

if command -v rsync >/dev/null 2>&1; then
  rsync -a --delete "$SDK_SOURCE/" "$SDK_STAGE/"
else
  cp -a "$SDK_SOURCE" "$SDK_STAGE_ROOT/"
fi

docker build -t "$IMAGE" "$ROOT_DIR"

case "${PUSH_IMAGE:-false}" in
  1|true|TRUE|yes|YES|on|ON)
    docker push "$IMAGE"
    ;;
esac
