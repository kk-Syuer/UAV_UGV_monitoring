#!/usr/bin/env python3
"""Generate UAV battery time-series plots as SVG files (no third-party deps)."""

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


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Plot round1 UAV battery series to SVG")
    parser.add_argument("--input-root", type=Path, default=DEFAULT_INPUT)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT)
    return parser.parse_args()


def is_uav_like(uav_id: str) -> bool:
    low = (uav_id or "").lower()
    return low.startswith("uav") or low.startswith("ch")


def load_status_rows(csv_path: Path) -> Dict[str, List[Tuple[float, float]]]:
    records: Dict[str, List[Tuple[float, float]]] = defaultdict(list)
    with csv_path.open(newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            uav_id = (row.get("uav_id") or "").strip()
            if not is_uav_like(uav_id):
                continue
            try:
                t = float(row.get("time", ""))
                b = float(row.get("battery_level", ""))
            except ValueError:
                continue
            records[uav_id].append((t, b))
    return records


def esc(text: str) -> str:
    return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def build_svg(records: Dict[str, List[Tuple[float, float]]], title: str) -> str:
    width, height = 1200, 700
    left, right, top, bottom = 80, 260, 70, 70
    plot_w = width - left - right
    plot_h = height - top - bottom

    all_points = [(t, b) for pts in records.values() for t, b in pts]
    t_min = min(t for t, _ in all_points)
    t_max = max(t for t, _ in all_points)
    if t_max <= t_min:
        t_max = t_min + 1.0

    def x_map(t: float) -> float:
        return left + ((t - t_min) / (t_max - t_min)) * plot_w

    def y_map(b: float) -> float:
        b = max(0.0, min(100.0, b))
        return top + (1.0 - (b / 100.0)) * plot_h

    lines = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}">',
        '<rect width="100%" height="100%" fill="white"/>',
        f'<text x="{left}" y="35" font-size="24" font-family="Arial" font-weight="bold">{esc(title)}</text>',
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
        t = t_min + frac * (t_max - t_min)
        elapsed_min = (t - t_min) / 60.0
        lines.append(f'<line x1="{x:.2f}" y1="{top}" x2="{x:.2f}" y2="{top + plot_h}" stroke="#f0f0f0"/>')
        lines.append(f'<text x="{x:.2f}" y="{top + plot_h + 20}" text-anchor="middle" font-size="12" font-family="Arial">{elapsed_min:.1f}</text>')

    lines.append(f'<text x="{left + plot_w/2:.2f}" y="{height - 20}" text-anchor="middle" font-size="14" font-family="Arial">Elapsed time (minutes)</text>')
    lines.append(f'<text x="20" y="{top + plot_h/2:.2f}" transform="rotate(-90,20,{top + plot_h/2:.2f})" text-anchor="middle" font-size="14" font-family="Arial">Battery level (%)</text>')

    legend_x = left + plot_w + 20
    legend_y = top + 10

    for i, uav_id in enumerate(sorted(records)):
        color = PALETTE[i % len(PALETTE)]
        pts = sorted(records[uav_id], key=lambda x: x[0])
        poly = " ".join(f"{x_map(t):.2f},{y_map(b):.2f}" for t, b in pts)
        lines.append(f'<polyline points="{poly}" fill="none" stroke="{color}" stroke-width="1.5"/>')

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

    title = f"UAV Battery Time Series - {policy_dir.name}"
    svg = build_svg(records, title)

    output_dir.mkdir(parents=True, exist_ok=True)
    out_path = output_dir / f"{policy_dir.name}_uav_battery_timeseries.svg"
    out_path.write_text(svg, encoding="utf-8")
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
