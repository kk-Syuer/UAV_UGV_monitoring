#!/usr/bin/env python3
"""Run all analysis figures with fail-fast orchestration.

# requirements.txt snippet:
# pandas>=1.5
# numpy>=1.23
# matplotlib>=3.6
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser()
    p.add_argument("--data_root", required=True, type=Path)
    p.add_argument("--out_dir", type=Path, default=Path("analysis/test_round1"))
    return p.parse_args()


def run_cmd(cmd: list[str]) -> None:
    print("Running:", " ".join(cmd))
    proc = subprocess.run(cmd)
    if proc.returncode != 0:
        raise RuntimeError(f"Command failed ({proc.returncode}): {' '.join(cmd)}")


def main() -> int:
    a = parse_args()
    py = sys.executable
    cmds = [
        [py, "-m", "tools.plot_network_timeseries", "--data_root", str(a.data_root), "--out_dir", str(a.out_dir), "--metric", m] for m in ["pdr", "delay", "jitter"]
    ] + [
        [py, "-m", "tools.plot_message_distributions", "--data_root", str(a.data_root), "--out_dir", str(a.out_dir), "--metric", "e2e_delay"],
        [py, "-m", "tools.plot_message_distributions_violin", "--data_root", str(a.data_root), "--out_dir", str(a.out_dir), "--metric", "hop_count"],
        [py, "-m", "tools.plot_qos_composition", "--data_root", str(a.data_root), "--out_dir", str(a.out_dir), "--view", "heatmap_pdr"],
        [py, "-m", "tools.plot_charge_queue_timeseries", "--data_root", str(a.data_root), "--out_dir", str(a.out_dir), "--metric", "queue_len", "--role_scope", "all"],
        [py, "-m", "tools.plot_waiting_time_cdf", "--data_root", str(a.data_root), "--out_dir", str(a.out_dir), "--role", "ALL"],
        [py, "-m", "tools.plot_battery_weather_relationship", "--data_root", str(a.data_root), "--out_dir", str(a.out_dir), "--view", "drain_vs_wind_scatter"],
        [py, "-m", "tools.plot_battery_weather_relationship", "--data_root", str(a.data_root), "--out_dir", str(a.out_dir), "--view", "drain_vs_rain_scatter"],
        [py, "-m", "tools.plot_lifetime_weather_heatmap", "--data_root", str(a.data_root), "--out_dir", str(a.out_dir)],
        [py, "-m", "tools.plot_recovery_latency", "--data_root", str(a.data_root), "--out_dir", str(a.out_dir)],
        [py, "-m", "tools.plot_pdr_aligned_to_failure", "--data_root", str(a.data_root), "--out_dir", str(a.out_dir)],
        [py, "-m", "tools.plot_policy_radar", "--data_root", str(a.data_root), "--out_dir", str(a.out_dir)],
        [py, "-m", "tools.plot_collapse_threshold", "--data_root", str(a.data_root), "--out_dir", str(a.out_dir)],
        [py, "-m", "tools.plot_death_events_timeseries", "--data_root", str(a.data_root), "--out_dir", str(a.out_dir), "--role", "ALL"],
        [py, "-m", "tools.plot_energy_charged_timeseries", "--data_root", str(a.data_root), "--out_dir", str(a.out_dir), "--role", "ALL"],
        [py, "-m", "tools.plot_charge_priority_scatter", "--data_root", str(a.data_root), "--out_dir", str(a.out_dir)],
        [py, "-m", "tools.plot_round1_network_robustness", "--input-root", str(a.data_root), "--output-dir", str(a.out_dir)],
        [py, "-m", "tools.plot_round1_uav_battery_timeseries", "--input-root", str(a.data_root), "--output-dir", str(a.out_dir)],
        [py, "-m", "tools.plot_ugv_locker_usage_comparison", "--data_root", str(a.data_root), "--out_dir", str(a.out_dir)],
    ]

    try:
        for c in cmds:
            run_cmd(c)
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    generated = sorted(str(p) for p in a.out_dir.rglob("*.png"))
    print("\nGenerated files manifest:")
    for g in generated:
        print(g)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
