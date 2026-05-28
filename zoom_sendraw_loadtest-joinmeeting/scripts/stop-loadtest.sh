#!/usr/bin/env bash
set -euo pipefail

PROJECT=${1:-${PROJECT:-zoom-loadtest-joinmeeting}}

mapfile -t containers < <(docker ps -aq --filter "label=zoom-loadtest.project=${PROJECT}")

if [[ ${#containers[@]} -eq 0 ]]; then
  echo "No containers found for project ${PROJECT}."
  exit 0
fi

docker rm -f "${containers[@]}"
echo "Stopped ${#containers[@]} container(s) for project ${PROJECT}."
