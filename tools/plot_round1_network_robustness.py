#!/usr/bin/env python3
"""Create round1 network-robustness plots (SVG) from existing experiment logs."""

from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
from pathlib import Path
from statistics import mean
from typing import Dict, Iterable, List, Tuple

DEFAULT_INPUT = Path("experiment_data_collection/test_round1")
DEFAULT_OUTPUT = Path("analysis/test_round1")
PALETTE = ["#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd", "#8c564b"]


@dataclass
class MessageEvent:
    control_type: str
    time_s: float
    delivered: bool
    row_idx: int


@dataclass
class Metrics:
    policy: str
    ch_downtime_ratio: float
    recovery_latency_s: float
    transition_pdr: float
    routing_convergence_s: float


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Plot round1 network robustness metrics")
    p.add_argument("--input-root", type=Path, default=DEFAULT_INPUT)
    p.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT)
    return p.parse_args()


def parse_bool(value: str) -> bool:
    return (value or "").strip().lower() in {"1", "true", "yes"}


def load_rows(path: Path) -> Iterable[Dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            yield row


def ch_downtime_ratio(status_csv: Path) -> float:
    by_time: Dict[float, bool] = {}
    for row in load_rows(status_csv):
        try:
            t = float(row.get("time", ""))
            role = int(float(row.get("role", "0")))
        except ValueError:
            continue
        active = role == 1 and parse_bool(row.get("backbone_active", "false"))
        by_time[t] = by_time.get(t, False) or active

    if not by_time:
        return 0.0
    down = sum(1 for v in by_time.values() if not v)
    return down / len(by_time)


def load_messages(messages_csv: Path) -> List[MessageEvent]:
    events: List[MessageEvent] = []
    for idx, row in enumerate(load_rows(messages_csv)):
        try:
            t = float(row.get("creation_time_s", ""))
        except ValueError:
            continue
        events.append(
            MessageEvent(
                control_type=(row.get("control_type") or "").strip(),
                time_s=t,
                delivered=parse_bool(row.get("delivered", "false")),
                row_idx=idx,
            )
        )
    if not events:
        return []

    unique_t = len({e.time_s for e in events})
    coarse = unique_t <= max(3, len(events) // 200)
    if not coarse:
        return events

    t0 = min(e.time_s for e in events)
    t1 = max(e.time_s for e in events)
    duration = max(1.0, t1 - t0)
    idx0 = min(e.row_idx for e in events)
    idx1 = max(e.row_idx for e in events)
    span = max(1, idx1 - idx0)

    out: List[MessageEvent] = []
    for e in events:
        pseudo_t = t0 + ((e.row_idx - idx0) / span) * duration
        out.append(MessageEvent(e.control_type, pseudo_t, e.delivered, e.row_idx))
    return out


def transition_windows(events: List[MessageEvent]) -> List[Tuple[float, float]]:
    failures = sorted(e.time_s for e in events if e.control_type == "FAILURE_EVENT")
    respawns = sorted(e.time_s for e in events if e.control_type == "RESPAWN_COMPLETED")

    windows: List[Tuple[float, float]] = []
    j = 0
    for ft in failures:
        while j < len(respawns) and respawns[j] < ft:
            j += 1
        if j < len(respawns):
            windows.append((ft, respawns[j]))
            j += 1
    return windows


def recovery_latency_seconds(windows: List[Tuple[float, float]]) -> float:
    vals = [b - a for a, b in windows if b >= a]
    return mean(vals) if vals else 0.0


def pdr_during_windows(events: List[MessageEvent], windows: List[Tuple[float, float]]) -> float:
    if not windows:
        return 0.0
    windows = sorted(windows)
    sent = delivered = 0
    wi = 0
    for e in sorted(events, key=lambda x: x.time_s):
        while wi < len(windows) and e.time_s > windows[wi][1]:
            wi += 1
        if wi >= len(windows):
            break
        a, b = windows[wi]
        if a <= e.time_s <= b:
            sent += 1
            if e.delivered:
                delivered += 1
    return (delivered / sent) if sent else 0.0


def routing_convergence_seconds(events: List[MessageEvent], windows: List[Tuple[float, float]]) -> float:
    if not windows:
        return 0.0
    status = sorted(e.time_s for e in events if e.control_type == "STATUS_CH" and e.delivered)
    if not status:
        return 0.0
    deltas: List[float] = []
    i = 0
    for _, respawn_t in sorted(windows, key=lambda x: x[1]):
        while i < len(status) and status[i] < respawn_t:
            i += 1
        if i < len(status):
            deltas.append(status[i] - respawn_t)
            i += 1
    return mean(deltas) if deltas else 0.0


def compute_metrics(policy_dir: Path) -> Metrics:
    events = load_messages(policy_dir / "messages.csv")
    windows = transition_windows(events)
    return Metrics(
        policy=policy_dir.name,
        ch_downtime_ratio=ch_downtime_ratio(policy_dir / "status_timeseries.csv"),
        recovery_latency_s=recovery_latency_seconds(windows),
        transition_pdr=pdr_during_windows(events, windows),
        routing_convergence_s=routing_convergence_seconds(events, windows),
    )


def esc(s: str) -> str:
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def make_bar_svg(title: str, y_label: str, labels: List[str], values: List[float], out_path: Path) -> None:
    width, height = 1100, 650
    left, right, top, bottom = 90, 50, 80, 120
    plot_w = width - left - right
    plot_h = height - top - bottom

    vmax = max(values) if values else 1.0
    vmax = vmax if vmax > 0 else 1.0

    def x_pos(i: int) -> float:
        n = max(1, len(labels))
        return left + i * (plot_w / n) + (plot_w / n) * 0.15

    bw = (plot_w / max(1, len(labels))) * 0.7

    def y_pos(v: float) -> float:
        return top + (1 - (v / vmax)) * plot_h

    lines = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}">',
        '<rect width="100%" height="100%" fill="white"/>',
        f'<text x="{left}" y="40" font-family="Arial" font-size="26" font-weight="bold">{esc(title)}</text>',
        f'<line x1="{left}" y1="{top+plot_h}" x2="{left+plot_w}" y2="{top+plot_h}" stroke="black"/>',
        f'<line x1="{left}" y1="{top}" x2="{left}" y2="{top+plot_h}" stroke="black"/>',
        f'<text x="25" y="{top+plot_h/2}" transform="rotate(-90,25,{top+plot_h/2})" font-size="14" font-family="Arial">{esc(y_label)}</text>',
    ]
    for i in range(6):
        frac = i / 5
        y = top + (1 - frac) * plot_h
        lines.append(f'<line x1="{left}" y1="{y:.2f}" x2="{left+plot_w}" y2="{y:.2f}" stroke="#e8e8e8"/>')
        lines.append(f'<text x="{left-8}" y="{y+4:.2f}" text-anchor="end" font-size="12" font-family="Arial">{(frac*vmax):.3f}</text>')

    for i, (lab, val) in enumerate(zip(labels, values)):
        x = x_pos(i)
        y = y_pos(val)
        h = top + plot_h - y
        color = PALETTE[i % len(PALETTE)]
        lines.append(f'<rect x="{x:.2f}" y="{y:.2f}" width="{bw:.2f}" height="{h:.2f}" fill="{color}"/>')
        lines.append(f'<text x="{x+bw/2:.2f}" y="{y-8:.2f}" text-anchor="middle" font-size="12" font-family="Arial">{val:.4f}</text>')
        lines.append(f'<text x="{x+bw/2:.2f}" y="{top+plot_h+24}" text-anchor="middle" font-size="12" font-family="Arial">{esc(lab)}</text>')

    lines.append('</svg>')
    out_path.write_text("\n".join(lines), encoding="utf-8")


def write_summary_csv(metrics: List[Metrics], out_csv: Path) -> None:
    with out_csv.open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["policy", "ch_downtime_ratio", "recovery_latency_s", "transition_pdr", "routing_convergence_s"])
        for m in metrics:
            w.writerow([m.policy, f"{m.ch_downtime_ratio:.8f}", f"{m.recovery_latency_s:.8f}", f"{m.transition_pdr:.8f}", f"{m.routing_convergence_s:.8f}"])


def main() -> None:
    args = parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    policies = sorted(p for p in args.input_root.iterdir() if p.is_dir())
    metrics = [compute_metrics(p) for p in policies]

    labels = [m.policy for m in metrics]
    write_summary_csv(metrics, args.output_dir / "network_robustness_metrics_round1.csv")
    make_bar_svg("Round1 Network Robustness: CH Downtime Ratio", "Downtime ratio", labels, [m.ch_downtime_ratio for m in metrics], args.output_dir / "round1_ch_downtime_ratio.svg")
    make_bar_svg("Round1 Network Robustness: Recovery Latency", "Seconds", labels, [m.recovery_latency_s for m in metrics], args.output_dir / "round1_recovery_latency.svg")
    make_bar_svg("Round1 Network Robustness: PDR during CH transitions", "PDR", labels, [m.transition_pdr for m in metrics], args.output_dir / "round1_transition_pdr.svg")
    make_bar_svg("Round1 Network Robustness: Routing Convergence Time", "Seconds", labels, [m.routing_convergence_s for m in metrics], args.output_dir / "round1_routing_convergence.svg")

    print("Generated files:")
    for n in [
        "network_robustness_metrics_round1.csv",
        "round1_ch_downtime_ratio.svg",
        "round1_recovery_latency.svg",
        "round1_transition_pdr.svg",
        "round1_routing_convergence.svg",
    ]:
        print(f" - {args.output_dir / n}")


if __name__ == "__main__":
    main()
