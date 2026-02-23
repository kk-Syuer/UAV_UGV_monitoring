#!/usr/bin/env python3
"""Generate UAV battery time-series plots as SVG files.

The script reads round1 experiment status logs and draws one plot per policy.
If `time` values are heavily quantized (e.g., only a few unique timestamps because of
scientific notation precision), it falls back to sequence-based timeline reconstruction.
"""

from __future__ import annotations

import argparse
import csv
from collections import defaultdict
from pathlib import Path
from typing import Dict, List, Tuple

DEFAULT_INPUT = Path("experiment_data_collection/test_round1")
DEFAULT_OUTPUT = Path("analysis/test_round1")

PALETTE = [
    "#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd", "#8c564b",
    "#e377c2", "#7f7f7f", "#bcbd22", "#17becf", "#393b79", "#637939",
]

Point = Tuple[float, float, int]  # (raw_time_sec, battery_level, row_index)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Plot round1 UAV battery series to SVG")
    parser.add_argument("--input-root", type=Path, default=DEFAULT_INPUT)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT)
    return parser.parse_args()


def is_uav_like(uav_id: str) -> bool:
    low = (uav_id or "").lower()
    return low.startswith("uav") or low.startswith("ch")


def load_status_rows(csv_path: Path) -> Dict[str, List[Point]]:
    records: Dict[str, List[Point]] = defaultdict(list)
    row_idx = 0
    with csv_path.open(newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            uav_id = (row.get("uav_id") or "").strip()
            if not is_uav_like(uav_id):
                row_idx += 1
                continue
            try:
                t = float(row.get("time", ""))
                b = float(row.get("battery_level", ""))
            except ValueError:
                row_idx += 1
                continue
            records[uav_id].append((t, b, row_idx))
            row_idx += 1
    return records


def esc(text: str) -> str:
    return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def to_elapsed_minutes(
    records: Dict[str, List[Point]],
) -> Tuple[Dict[str, List[Tuple[float, float]]], bool, float]:
    """Convert raw points to elapsed-minute series.

    Returns:
      - series per UAV: [(elapsed_min, battery), ...]
      - whether sequence-based fallback is used
      - inferred total duration in minutes
    """
    all_time_values = [t for pts in records.values() for t, _, _ in pts]
    if not all_time_values:
        return {}, False, 0.0

    unique_times = len(set(all_time_values))
    total_points = len(all_time_values)

    # Heuristic: if time precision is too coarse, timeline is unreliable.
    coarse_time = unique_times <= max(3, total_points // 200)

    t_min = min(all_time_values)
    t_max = max(all_time_values)
    raw_duration_min = max(0.0, (t_max - t_min) / 60.0)

    out: Dict[str, List[Tuple[float, float]]] = {}

    if not coarse_time:
        for uav_id, pts in records.items():
            ordered = sorted(pts, key=lambda x: x[0])
            out[uav_id] = [((t - t_min) / 60.0, b) for t, b, _ in ordered]
        return out, False, raw_duration_min

    # Fallback: reconstruct timeline from row order, keep global duration if available.
    if raw_duration_min <= 0.0:
        raw_duration_min = 1.0

    for uav_id, pts in records.items():
        ordered = sorted(pts, key=lambda x: x[2])
        if len(ordered) == 1:
            out[uav_id] = [(0.0, ordered[0][1])]
            continue

        seq0 = ordered[0][2]
        seqN = ordered[-1][2]
        span = max(1, seqN - seq0)

        series: List[Tuple[float, float]] = []
        for _, b, seq in ordered:
            frac = (seq - seq0) / span
            x_min = frac * raw_duration_min
            series.append((x_min, b))
        out[uav_id] = series

    return out, True, raw_duration_min


def build_svg(series: Dict[str, List[Tuple[float, float]]], title: str, used_fallback: bool) -> str:
    width, height = 1200, 700
    left, right, top, bottom = 80, 260, 70, 70
    plot_w = width - left - right
    plot_h = height - top - bottom

    all_points = [(x, b) for pts in series.values() for x, b in pts]
    x_min = min(x for x, _ in all_points)
    x_max = max(x for x, _ in all_points)
    if x_max <= x_min:
        x_max = x_min + 1.0

    def x_map(x: float) -> float:
        return left + ((x - x_min) / (x_max - x_min)) * plot_w

    def y_map(b: float) -> float:
        b = max(0.0, min(100.0, b))
        return top + (1.0 - (b / 100.0)) * plot_h

    subtitle = "(timeline reconstructed from sample order)" if used_fallback else ""

    lines = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}">',
        '<rect width="100%" height="100%" fill="white"/>',
        f'<text x="{left}" y="35" font-size="24" font-family="Arial" font-weight="bold">{esc(title)}</text>',
        f'<text x="{left}" y="56" font-size="12" font-family="Arial" fill="#555">{esc(subtitle)}</text>',
        f'<line x1="{left}" y1="{top + plot_h}" x2="{left + plot_w}" y2="{top + plot_h}" stroke="black" stroke-width="1"/>',
        f'<line x1="{left}" y1="{top}" x2="{left}" y2="{top + plot_h}" stroke="black" stroke-width="1"/>',
    ]

    for i in range(6):
        yv = i * 20
        y = y_map(yv)
        lines.append(f'<line x1="{left}" y1="{y:.2f}" x2="{left + plot_w}" y2="{y:.2f}" stroke="#dddddd"/>')
        lines.append(f'<text x="{left - 10}" y="{y + 4:.2f}" text-anchor="end" font-size="12" font-family="Arial">{yv}</text>')

    for i in range(7):
        frac = i / 6
        x = left + frac * plot_w
        t = x_min + frac * (x_max - x_min)
        lines.append(f'<line x1="{x:.2f}" y1="{top}" x2="{x:.2f}" y2="{top + plot_h}" stroke="#f0f0f0"/>')
        lines.append(f'<text x="{x:.2f}" y="{top + plot_h + 20}" text-anchor="middle" font-size="12" font-family="Arial">{t:.1f}</text>')

    lines.append(f'<text x="{left + plot_w/2:.2f}" y="{height - 20}" text-anchor="middle" font-size="14" font-family="Arial">Elapsed time (minutes)</text>')
    lines.append(f'<text x="20" y="{top + plot_h/2:.2f}" transform="rotate(-90,20,{top + plot_h/2:.2f})" text-anchor="middle" font-size="14" font-family="Arial">Battery level (%)</text>')

    legend_x = left + plot_w + 20
    legend_y = top + 10

    for i, uav_id in enumerate(sorted(series)):
        color = PALETTE[i % len(PALETTE)]
        pts = series[uav_id]
        poly = " ".join(f"{x_map(x):.2f},{y_map(b):.2f}" for x, b in pts)
        lines.append(f'<polyline points="{poly}" fill="none" stroke="{color}" stroke-width="1.2"/>')

        ly = legend_y + i * 20
        lines.append(f'<line x1="{legend_x}" y1="{ly}" x2="{legend_x + 22}" y2="{ly}" stroke="{color}" stroke-width="2"/>')
        lines.append(f'<text x="{legend_x + 28}" y="{ly + 4}" font-size="12" font-family="Arial">{esc(uav_id)}</text>')

    lines.append("</svg>")
    return "\n".join(lines)


def plot_policy(policy_dir: Path, output_dir: Path) -> Path | None:
    csv_path = policy_dir / "status_timeseries.csv"
    if not csv_path.exists():
        return None

    records = load_status_rows(csv_path)
    if not records:
        return None

    series, used_fallback, duration_min = to_elapsed_minutes(records)
    title = f"UAV Battery Time Series - {policy_dir.name}"
    svg = build_svg(series, title, used_fallback)

    output_dir.mkdir(parents=True, exist_ok=True)
    out_path = output_dir / f"{policy_dir.name}_uav_battery_timeseries.svg"
    out_path.write_text(svg, encoding="utf-8")

    if used_fallback:
        print(f"[WARN] {policy_dir.name}: coarse timestamp precision detected; timeline reconstructed from row order (~{duration_min:.1f} min).")
    else:
        print(f"[OK]   {policy_dir.name}: raw timestamps used.")

    return out_path


def main() -> None:
    args = parse_args()
    policies = sorted(p for p in args.input_root.iterdir() if p.is_dir())
    generated: List[Path] = []

    for policy_dir in policies:
        out = plot_policy(policy_dir, args.output_dir)
        if out:
            generated.append(out)

    if not generated:
        raise RuntimeError("No plots generated.")

    print("Generated plots:")
    for path in generated:
        print(f" - {path}")


if __name__ == "__main__":
    main()
