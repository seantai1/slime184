#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INPUT_DIR="${INPUT_DIR:-$ROOT_DIR/validation/anim}"
OUTPUT_FILE="${OUTPUT_FILE:-$ROOT_DIR/validation/slime_animation.mp4}"
FPS="${FPS:-24}"
START_FRAME="${START_FRAME:-1}"
FRAME_COUNT="${FRAME_COUNT:-41}"

if ! command -v ffmpeg >/dev/null 2>&1; then
  echo "ffmpeg is required but was not found on PATH." >&2
  exit 1
fi

first_frame="$INPUT_DIR/slime_$(printf "%04d" "$START_FRAME").png"
if [[ ! -f "$first_frame" ]]; then
  echo "Missing first frame: $first_frame" >&2
  exit 1
fi

mkdir -p "$(dirname "$OUTPUT_FILE")"

ffmpeg -y \
  -framerate "$FPS" \
  -start_number "$START_FRAME" \
  -i "$INPUT_DIR/slime_%04d.png" \
  -frames:v "$FRAME_COUNT" \
  -c:v libx264 \
  -pix_fmt yuv420p \
  -movflags +faststart \
  "$OUTPUT_FILE"

echo "Wrote video: $OUTPUT_FILE"
