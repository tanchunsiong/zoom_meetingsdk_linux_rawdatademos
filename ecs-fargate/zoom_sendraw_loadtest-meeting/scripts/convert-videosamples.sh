#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
SOURCE_DIR=${SOURCE_DIR:-"${ROOT_DIR}/videosamples"}
OUTPUT_DIR=${OUTPUT_DIR:-"${ROOT_DIR}/media"}
DURATION=${DURATION:-60}
WIDTH=${WIDTH:-1280}
HEIGHT=${HEIGHT:-720}
FPS=${FPS:-30}
SAFE_SCALE_PERCENT=${SAFE_SCALE_PERCENT:-40}
SAMPLE_RATE=${SAMPLE_RATE:-48000}
CHANNELS=${CHANNELS:-1}
FORCE=${FORCE:-0}

mkdir -p "$OUTPUT_DIR"

CONTENT_WIDTH=$((WIDTH * SAFE_SCALE_PERCENT / 100))
CONTENT_HEIGHT=$((HEIGHT * SAFE_SCALE_PERCENT / 100))
CONTENT_WIDTH=$((CONTENT_WIDTH / 2 * 2))
CONTENT_HEIGHT=$((CONTENT_HEIGHT / 2 * 2))

slugify() {
  local name=${1%.*}
  name=$(printf '%s' "$name" | tr '[:upper:]' '[:lower:]')
  name=$(printf '%s' "$name" | sed -E 's/[^a-z0-9]+/_/g; s/^_+//; s/_+$//')
  printf '%s' "$name"
}

json_escape() {
  local value=${1:-}
  value=${value//\\/\\\\}
  value=${value//\"/\\\"}
  value=${value//$'\n'/\\n}
  value=${value//$'\r'/\\r}
  value=${value//$'\t'/\\t}
  printf '%s' "$value"
}

mapfile -t sources < <(find "$SOURCE_DIR" -maxdepth 1 -type f -iname '*.mp4' | sort)

if [[ ${#sources[@]} -eq 0 ]]; then
  echo "No MP4 files found in ${SOURCE_DIR}" >&2
  exit 66
fi

manifest_tmp=$(mktemp)
trap 'rm -f "$manifest_tmp"' EXIT

{
  printf '{\n'
  printf '  "version": 1,\n'
  printf '  "generated_by": "scripts/convert-videosamples.sh",\n'
  printf '  "duration_seconds": %s,\n' "$DURATION"
  printf '  "safe_scale_percent": %s,\n' "$SAFE_SCALE_PERCENT"
  printf '  "items": [\n'
} > "$manifest_tmp"

first=1
for src in "${sources[@]}"; do
  base=$(basename "$src")
  slug=$(slugify "$base")
  video_file="${slug}_${WIDTH}x${HEIGHT}_${FPS}fps_i420.yuv"
  audio_file="${slug}_${SAMPLE_RATE}hz_s16le_mono.pcm"
  video_out="${OUTPUT_DIR}/${video_file}"
  audio_out="${OUTPUT_DIR}/${audio_file}"

  if [[ "$FORCE" == "1" || ! -s "$video_out" ]]; then
    echo "Converting video: ${base}"
    ffmpeg -hide_banner -y -t "$DURATION" -i "$src" \
      -map 0:v:0 -an \
      -vf "scale=${CONTENT_WIDTH}:${CONTENT_HEIGHT}:force_original_aspect_ratio=decrease,pad=${WIDTH}:${HEIGHT}:(ow-iw)/2:(oh-ih)/2,fps=${FPS},format=yuv420p" \
      -pix_fmt yuv420p \
      -f rawvideo "$video_out"
  else
    echo "Keeping existing video: ${video_file}"
  fi

  if [[ "$FORCE" == "1" || ! -s "$audio_out" ]]; then
    echo "Converting audio: ${base}"
    ffmpeg -hide_banner -y -t "$DURATION" -i "$src" \
      -map 0:a:0 -vn \
      -ac "$CHANNELS" -ar "$SAMPLE_RATE" -sample_fmt s16 \
      -f s16le "$audio_out"
  else
    echo "Keeping existing audio: ${audio_file}"
  fi

  if [[ "$first" -eq 0 ]]; then
    printf ',\n' >> "$manifest_tmp"
  fi
  first=0

  frame_count=$((DURATION * FPS))
  printf '    {\n' >> "$manifest_tmp"
  printf '      "name": "%s",\n' "$(json_escape "$slug")" >> "$manifest_tmp"
  printf '      "source": "%s",\n' "$(json_escape "$base")" >> "$manifest_tmp"
  printf '      "video": "%s",\n' "$(json_escape "$video_file")" >> "$manifest_tmp"
  printf '      "audio": "%s",\n' "$(json_escape "$audio_file")" >> "$manifest_tmp"
  printf '      "video_width": %s,\n' "$WIDTH" >> "$manifest_tmp"
  printf '      "video_height": %s,\n' "$HEIGHT" >> "$manifest_tmp"
  printf '      "video_fps": %s,\n' "$FPS" >> "$manifest_tmp"
  printf '      "video_frame_count": %s,\n' "$frame_count" >> "$manifest_tmp"
  printf '      "audio_sample_rate": %s,\n' "$SAMPLE_RATE" >> "$manifest_tmp"
  printf '      "audio_channels": %s\n' "$CHANNELS" >> "$manifest_tmp"
  printf '    }' >> "$manifest_tmp"
done

{
  printf '\n'
  printf '  ]\n'
  printf '}\n'
} >> "$manifest_tmp"

mv "$manifest_tmp" "${OUTPUT_DIR}/manifest.json"
trap - EXIT

echo "Wrote ${OUTPUT_DIR}/manifest.json"
du -sh "$OUTPUT_DIR"
