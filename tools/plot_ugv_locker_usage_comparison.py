#!/usr/bin/env python3
"""Plot UGV charger locker usage and energy comparisons across protocols.

Reads each protocol folder under --data_root (e.g., ugv_fcfs, ugv_edf, ...)
and computes:
- mean locker utilization,
- mean active charging sessions,
- peak queue length,
- started/completed charge events,
- total recovered energy,
- mean recovered energy per event,
- run duration,
- recovered energy rate (Wh/h),
- first-half vs second-half recovered energy.
"""

from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

import matplotlib.pyplot as plt


@dataclass
class ProtocolMetrics:
    protocol: str
    mean_locker_utilization: float
    mean_active_sessions: float
    peak_queue_length: float
    started_charge_events: int
    completed_charge_events: int
    total_recovered_energy_wh: float
    mean_energy_per_event_wh: float
    run_duration_h: float
    recovered_energy_rate_wh_per_h: float
    first_half_energy_wh: float
    second_half_energy_wh: float


def _mean(values: Iterable[float]) -> float:
    vals = list(values)
    return sum(vals) / len(vals) if vals else 0.0


def _load_queue_metrics(path: Path) -> tuple[float, float, float, list[float]]:
    util: list[float] = []
    active: list[float] = []
    queue: list[float] = []
    times: list[float] = []

    with path.open(newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            util.append(float(row["ugv_dock_utilization"]))
            active.append(float(row["active_charging_ugv_sessions"]))
            queue.append(float(row["queue_length_ugv"]))
            times.append(float(row["time"]))

    if not times:
        raise ValueError(f"No timeseries rows found: {path}")

    return _mean(util), _mean(active), max(queue) if queue else 0.0, times


def _load_event_metrics(path: Path) -> tuple[int, int, float, float, list[tuple[float, float]]]:
    started = 0
    completed = 0
    energies: list[float] = []
    timed_energy: list[tuple[float, float]] = []

    with path.open(newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            if row.get("outcome") == "STARTED":
                started += 1

            if row.get("charge_completed", "").strip().lower() in {"true", "1", "yes"}:
                completed += 1

            raw_energy = row.get("energy_recovered", "")
            if raw_energy in {"", "-1"}:
                continue

            energy = float(raw_energy)
            if energy < 0:
                continue

            terminal_time = float(row["terminal_time"])
            energies.append(energy)
            timed_energy.append((terminal_time, energy))

    return started, completed, sum(energies), _mean(energies), timed_energy


def collect_metrics(policy_dir: Path) -> ProtocolMetrics:
    queue_csv = policy_dir / "charge_queue_timeseries.csv"
    events_csv = policy_dir / "charge_events.csv"

    mean_util, mean_active, peak_queue, times = _load_queue_metrics(queue_csv)
    started, completed, total_energy, mean_event_energy, timed_energy = _load_event_metrics(events_csv)

    t_min, t_max = min(times), max(times)
    duration_h = (t_max - t_min) / 3600.0 if t_max > t_min else 0.0
    energy_rate = (total_energy / duration_h) if duration_h > 0 else 0.0

    midpoint = t_min + (t_max - t_min) / 2.0
    first_half = sum(e for t, e in timed_energy if t <= midpoint)
    second_half = total_energy - first_half

    return ProtocolMetrics(
        protocol=policy_dir.name,
        mean_locker_utilization=mean_util,
        mean_active_sessions=mean_active,
        peak_queue_length=peak_queue,
        started_charge_events=started,
        completed_charge_events=completed,
        total_recovered_energy_wh=total_energy,
        mean_energy_per_event_wh=mean_event_energy,
        run_duration_h=duration_h,
        recovered_energy_rate_wh_per_h=energy_rate,
        first_half_energy_wh=first_half,
        second_half_energy_wh=second_half,
    )


def write_summary_csv(metrics: list[ProtocolMetrics], out_csv: Path) -> None:
    out_csv.parent.mkdir(parents=True, exist_ok=True)
    with out_csv.open("w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow([
            "protocol",
            "mean_locker_utilization",
            "mean_active_sessions",
            "peak_queue_length",
            "started_charge_events",
            "completed_charge_events",
            "total_recovered_energy_wh",
            "mean_energy_per_event_wh",
            "run_duration_h",
            "recovered_energy_rate_wh_per_h",
            "first_half_energy_wh",
            "second_half_energy_wh",
        ])
        for m in metrics:
            writer.writerow([
                m.protocol,
                f"{m.mean_locker_utilization:.6f}",
                f"{m.mean_active_sessions:.6f}",
                f"{m.peak_queue_length:.0f}",
                m.started_charge_events,
                m.completed_charge_events,
                f"{m.total_recovered_energy_wh:.6f}",
                f"{m.mean_energy_per_event_wh:.6f}",
                f"{m.run_duration_h:.6f}",
                f"{m.recovered_energy_rate_wh_per_h:.6f}",
                f"{m.first_half_energy_wh:.6f}",
                f"{m.second_half_energy_wh:.6f}",
            ])


def plot(metrics: list[ProtocolMetrics], out_dir: Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    labels = [m.protocol for m in metrics]

    # Figure 1: locker usage
    fig1, ax1 = plt.subplots(figsize=(11, 5))
    x = range(len(metrics))
    ax1.bar(x, [m.mean_locker_utilization for m in metrics], label="Mean locker utilization")
    ax1.plot(x, [m.mean_active_sessions for m in metrics], marker="o", color="tab:orange", label="Mean active sessions")
    ax1.set_xticks(list(x))
    ax1.set_xticklabels(labels, rotation=25, ha="right")
    ax1.set_ylim(bottom=0)
    ax1.set_ylabel("Usage")
    ax1.set_title("UGV locker usage by protocol")
    ax1.grid(alpha=0.3, axis="y")
    ax1.legend(loc="best")
    fig1.tight_layout()
    fig1.savefig(out_dir / "ugv_locker_usage_by_protocol.png", dpi=200)
    plt.close(fig1)

    # Figure 2: energy throughput
    fig2, ax2 = plt.subplots(figsize=(11, 5))
    ax2.bar(x, [m.total_recovered_energy_wh for m in metrics], label="Total recovered energy (Wh)")
    ax2.plot(x, [m.recovered_energy_rate_wh_per_h for m in metrics], marker="o", color="tab:red", label="Recovered energy rate (Wh/h)")
    ax2.set_xticks(list(x))
    ax2.set_xticklabels(labels, rotation=25, ha="right")
    ax2.set_ylim(bottom=0)
    ax2.set_ylabel("Energy")
    ax2.set_title("Recovered energy by protocol")
    ax2.grid(alpha=0.3, axis="y")
    ax2.legend(loc="best")
    fig2.tight_layout()
    fig2.savefig(out_dir / "ugv_energy_recovered_by_protocol.png", dpi=200)
    plt.close(fig2)

    # Figure 3: first-half vs second-half composition
    fig3, ax3 = plt.subplots(figsize=(11, 5))
    first = [m.first_half_energy_wh for m in metrics]
    second = [m.second_half_energy_wh for m in metrics]
    ax3.bar(x, first, label="First half")
    ax3.bar(x, second, bottom=first, label="Second half")
    ax3.set_xticks(list(x))
    ax3.set_xticklabels(labels, rotation=25, ha="right")
    ax3.set_ylim(bottom=0)
    ax3.set_ylabel("Recovered energy (Wh)")
    ax3.set_title("Energy-over-time split by protocol")
    ax3.grid(alpha=0.3, axis="y")
    ax3.legend(loc="best")
    fig3.tight_layout()
    fig3.savefig(out_dir / "ugv_energy_time_split_by_protocol.png", dpi=200)
    plt.close(fig3)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Plot UGV locker usage and energy comparison")
    parser.add_argument(
        "--data_root",
        type=Path,
        default=Path("experiment_data_collection/test_round1"),
        help="Directory containing protocol folders (e.g., ugv_fcfs, ugv_edf, ...)",
    )
    parser.add_argument(
        "--out_dir",
        type=Path,
        default=Path("analysis/test_round1/ugv_locker_usage"),
        help="Output directory for plots and summary CSV",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    policy_dirs = sorted([p for p in args.data_root.iterdir() if p.is_dir() and p.name.startswith("ugv_")])
    if not policy_dirs:
        raise SystemExit(f"No protocol dirs found under {args.data_root}")

    metrics = [collect_metrics(p) for p in policy_dirs]
    metrics.sort(key=lambda m: m.protocol)

    write_summary_csv(metrics, args.out_dir / "ugv_locker_usage_summary.csv")
    plot(metrics, args.out_dir)

    print(f"Saved: {args.out_dir / 'ugv_locker_usage_summary.csv'}")
    print(f"Saved: {args.out_dir / 'ugv_locker_usage_by_protocol.png'}")
    print(f"Saved: {args.out_dir / 'ugv_energy_recovered_by_protocol.png'}")
    print(f"Saved: {args.out_dir / 'ugv_energy_time_split_by_protocol.png'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
