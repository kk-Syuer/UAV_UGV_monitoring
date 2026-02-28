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

dot -Tpdf "$DOT_FILE" -o "$OUT_DIR/${BASE}.pdf"
dot -Gdpi=600 -Tpng "$DOT_FILE" -o "$OUT_DIR/${BASE}.png"
dot -Tsvg "$DOT_FILE" -o "$OUT_DIR/${BASE}.svg"

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
