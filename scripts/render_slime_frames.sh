#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
PATH_TRACER="$BUILD_DIR/pathtracer"

THREADS="${THREADS:-8}"
SAMPLES="${SAMPLES:-32}"
LIGHT_SAMPLES="${LIGHT_SAMPLES:-8}"
MAX_DEPTH="${MAX_DEPTH:-6}"
START_FRAME="${START_FRAME:-1}"
END_FRAME="${END_FRAME:-41}"
CAMERA_SETTINGS="${CAMERA_SETTINGS:-$BUILD_DIR/slime_0001_cam_settings.txt}"
ENVMAP="${ENVMAP:-$ROOT_DIR/assets/hdri/venice_sky.exr}"
INPUT_DIR="${INPUT_DIR:-$ROOT_DIR/dae/slime/frames}"
OUTPUT_DIR="${OUTPUT_DIR:-$ROOT_DIR/validation/anim}"

if [[ ! -x "$PATH_TRACER" ]]; then
  echo "Missing executable: $PATH_TRACER" >&2
  echo "Build it first from $BUILD_DIR with: make -j8" >&2
  exit 1
fi

if [[ ! -f "$CAMERA_SETTINGS" ]]; then
  echo "Missing camera settings file: $CAMERA_SETTINGS" >&2
  exit 1
fi

mkdir -p "$OUTPUT_DIR"

for frame in $(seq -f "%04g" "$START_FRAME" "$END_FRAME"); do
  input="$INPUT_DIR/slime_${frame}.dae"
  output="$OUTPUT_DIR/slime_${frame}.png"

  if [[ ! -f "$input" ]]; then
    echo "Skipping missing frame: $input" >&2
    continue
  fi

  echo "Rendering slime_${frame}.dae -> slime_${frame}.png"
  "$PATH_TRACER" \
    -t "$THREADS" \
    -s "$SAMPLES" \
    -l "$LIGHT_SAMPLES" \
    -m "$MAX_DEPTH" \
    -S \
    -e "$ENVMAP" \
    -c "$CAMERA_SETTINGS" \
    -f "$output" \
    "$input"
done
