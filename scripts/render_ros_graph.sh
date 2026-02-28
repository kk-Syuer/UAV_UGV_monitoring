#!/usr/bin/env bash
set -euo pipefail

DOT_FILE="artifacts/ros_graph.dot"
OUT_DIR="artifacts"
BASE="ros_graph_a4"

if ! command -v dot >/dev/null 2>&1; then
  echo "ERROR: Graphviz 'dot' not found. Install graphviz first." >&2
  exit 1
fi

if [[ ! -f "$DOT_FILE" ]]; then
  echo "ERROR: Missing $DOT_FILE (run python3 scripts/build_graphviz.py first)." >&2
  exit 1
fi

mkdir -p "$OUT_DIR"
TMP_DOT="$(mktemp "${OUT_DIR}/ros_graph_fallback_XXXXXX.dot")"
trap 'rm -f "$TMP_DOT"' EXIT

create_fallback_dot() {
  # Some Graphviz builds crash (segfault) on complex ortho+concentrate layouts.
  # Build a safer fallback by relaxing the most crash-prone knobs.
  cp "$DOT_FILE" "$TMP_DOT"
  sed -i 's/splines=ortho;/splines=polyline;/' "$TMP_DOT"
  sed -i 's/concentrate=true;/concentrate=false;/' "$TMP_DOT"
  sed -i 's/newrank=true;/newrank=false;/' "$TMP_DOT"
  sed -i 's/overlap=false;/overlap=prism;/' "$TMP_DOT"
  sed -i 's/ratio=compress/ratio=auto/' "$TMP_DOT"
}

render_one() {
  local fmt="$1"
  local out="$2"
  shift 2

  set +e
  dot "$@" -T"$fmt" "$DOT_FILE" -o "$out"
  local rc=$?
  set -e

  if [[ $rc -eq 0 ]]; then
    return 0
  fi

  echo "WARN: dot failed for $fmt (exit=$rc). Retrying with fallback DOT settings..." >&2
  create_fallback_dot

  set +e
  dot "$@" -T"$fmt" "$TMP_DOT" -o "$out"
  local rc2=$?
  set -e

  if [[ $rc2 -ne 0 ]]; then
    echo "ERROR: fallback rendering also failed for $fmt (exit=$rc2)." >&2
    if [[ $rc -eq 139 || $rc2 -eq 139 ]]; then
      echo "HINT: exit=139 indicates a Graphviz segmentation fault; try upgrading graphviz." >&2
    fi
    return $rc2
  fi

  echo "INFO: rendered $fmt via fallback DOT profile." >&2
}

render_one pdf "$OUT_DIR/${BASE}.pdf"
render_one png "$OUT_DIR/${BASE}.png" -Gdpi=600
render_one svg "$OUT_DIR/${BASE}.svg"

if command -v file >/dev/null 2>&1; then
  file "$OUT_DIR/${BASE}.pdf" "$OUT_DIR/${BASE}.png" "$OUT_DIR/${BASE}.svg"
fi

echo "Rendered:"
echo "  $OUT_DIR/${BASE}.pdf"
echo "  $OUT_DIR/${BASE}.png (high-DPI raster preview)"
echo "  $OUT_DIR/${BASE}.svg"

echo
echo "NOTE: Do NOT use '-Tps' when writing .png/.svg files."
echo "      Type and extension must match (e.g., -Tpng -> .png, -Tsvg -> .svg)."
echo "      For thesis print, prefer PDF/SVG (vector) to avoid blur."
