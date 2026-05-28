#!/usr/bin/env bash
set -euo pipefail

if [[ -z "${MEETING_NUMBER:-}" ]]; then
  echo "MEETING_NUMBER is required." >&2
  exit 64
fi

json_escape() {
  local value=${1:-}
  value=${value//\\/\\\\}
  value=${value//\"/\\\"}
  value=${value//$'\n'/\\n}
  value=${value//$'\r'/\\r}
  value=${value//$'\t'/\\t}
  printf '%s' "$value"
}

bool_string() {
  local value
  value=$(printf '%s' "${1:-false}" | tr '[:upper:]' '[:lower:]')
  case "$value" in
    1|true|yes|y|on) printf 'true' ;;
    *) printf 'false' ;;
  esac
}

JWT_ENDPOINT=${JWT_ENDPOINT:-https://nodejs.asdc.cc/meeting}
if [[ "$JWT_ENDPOINT" != http://* && "$JWT_ENDPOINT" != https://* ]]; then
  JWT_ENDPOINT="https://${JWT_ENDPOINT}"
fi
SDK_JWT=${JWT_TOKEN:-${JWT_SIGNATURE:-${SIGNATURE:-}}}
if [[ -z "$SDK_JWT" ]]; then
  echo "JWT_TOKEN, JWT_SIGNATURE, or SIGNATURE is required." >&2
  exit 64
fi
USER_ZAK=${USER_ZAK:-${ZAK_TOKEN:-${ZOOM_ZAK:-}}}
if [[ -z "$USER_ZAK" ]]; then
  echo "USER_ZAK or ZAK_TOKEN is required to start a meeting as host." >&2
  exit 64
fi
MEDIA_MANIFEST=${MEDIA_MANIFEST:-../media/manifest.json}

if [[ -n "${ZOOM_USERNAME_PREFIX:-}" ]]; then
  suffix=${ZOOM_INSTANCE_ID:-${HOSTNAME:-1}}
  suffix=${suffix:0:12}
  ZOOM_USERNAME="${ZOOM_USERNAME_PREFIX}-${suffix}"
else
  ZOOM_USERNAME=${ZOOM_USERNAME:-ZoomLoadBot}
fi

CONFIG_FILE=/opt/zoom-loadtest/bin/config.json
cat > "$CONFIG_FILE" <<JSON
{
  "meeting_number": "$(json_escape "$MEETING_NUMBER")",
  "token": "$(json_escape "$SDK_JWT")",
  "meeting_password": "$(json_escape "${MEETING_PASSWORD:-}")",
  "recording_token": "$(json_escape "${RECORDING_TOKEN:-}")",
  "remote_url": "$(json_escape "$JWT_ENDPOINT")",
  "user_name": "$(json_escape "$ZOOM_USERNAME")",
  "user_zak": "$(json_escape "$USER_ZAK")",
  "media_manifest": "$(json_escape "$MEDIA_MANIFEST")",
  "media_index": "$(json_escape "${MEDIA_INDEX:--1}")",
  "video_width": "$(json_escape "${VIDEO_WIDTH:-1280}")",
  "video_height": "$(json_escape "${VIDEO_HEIGHT:-720}")",
  "video_fps": "$(json_escape "${VIDEO_FPS:-30}")",
  "audio_sample_rate": "$(json_escape "${AUDIO_SAMPLE_RATE:-48000}")",
  "audio_channels": "$(json_escape "${AUDIO_CHANNELS:-1}")",
  "jwt_role": "$(json_escape "${JWT_ROLE:-1}")",
  "useJWTTokenFromWebService": "$(bool_string "${USE_JWT_TOKEN_FROM_WEB_SERVICE:-false}")",
  "useRecordingTokenFromWebService": "$(bool_string "${USE_RECORDING_TOKEN_FROM_WEB_SERVICE:-false}")",
  "SendVideoRawData": "$(bool_string "${SEND_VIDEO_RAW_DATA:-true}")",
  "SendAudioRawData": "$(bool_string "${SEND_AUDIO_RAW_DATA:-true}")",
  "chatDemo": "$(bool_string "${CHAT_DEMO:-true}")",
  "exitOnMeetingEnd": "$(bool_string "${EXIT_ON_MEETING_END:-true}")"
}
JSON

echo "Starting ${ZOOM_USERNAME} for meeting ${MEETING_NUMBER}"
echo "Using runtime Meeting SDK JWT/signature"
echo "Media manifest: ${MEDIA_MANIFEST}"

/opt/zoom-loadtest/setup-pulseaudio.sh
exec /opt/zoom-loadtest/bin/ZoomSendRawLoadTestStartMeetingDemo "$@"
