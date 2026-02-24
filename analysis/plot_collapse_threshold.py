#!/usr/bin/env python3
"""Collapse threshold analysis from network PDR.

# requirements.txt snippet:
# pandas>=1.5
# numpy>=1.23
# matplotlib>=3.6
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

from analysis.utils_clean import align_time_seconds, safe_numeric
from analysis.utils_io import discover_policy_dirs, load_csv_with_hints


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser()
    p.add_argument("--data_root", required=True, type=Path)
    p.add_argument("--out_dir", type=Path, default=Path("analysis/test_round1"))
    p.add_argument("--pdr_threshold", type=float, default=0.5)
    p.add_argument("--min_duration", type=float, default=10.0)
    p.add_argument(
        "--align_mission_time",
        choices=["none", "common_window"],
        default="common_window",
        help="Truncate all policies to a shared mission-time window for fair visual comparison.",
    )
    return p.parse_args()


def _policy_mission_end(policy_dir: Path) -> float:
    status_csv = policy_dir / "status_timeseries.csv"
    if not status_csv.exists():
        return np.nan
    s = load_csv_with_hints(status_csv)
    if "time" not in s.columns:
        return np.nan
    s = safe_numeric(s, ["time"])
    s = align_time_seconds(s, "time")
    t = s["time"].dropna()
    return float(t.max()) if not t.empty else np.nan


def _time_to_collapse(df: pd.DataFrame, threshold: float, min_duration: float) -> float:
    d = df.sort_values("time").copy()
    d["below"] = d["window_pdr"] < threshold
    start = None
    for _, r in d.iterrows():
        if r["below"] and start is None:
            start = r["time"]
        if (not r["below"]) and start is not None:
            if r["time"] - start >= min_duration:
                return float(start)
            start = None
    if start is not None and d["time"].iloc[-1] - start >= min_duration:
        return float(start)
    return np.nan


def main() -> int:
    args = parse_args()
    policies = discover_policy_dirs(args.data_root)
    ttc_rows = []

    global_end = np.nan
    policy_window_end: dict[str, float] = {}
    if args.align_mission_time == "common_window":
        ends = []
        for p in policies:
            try:
                end_t = _policy_mission_end(p)
                if pd.notna(end_t):
                    policy_window_end[p.name] = float(end_t)
                    ends.append(float(end_t))
            except Exception as exc:
                print(f"WARNING [{p.name}]: failed to estimate mission end time: {exc}", file=sys.stderr)
        if not ends:
            print("ERROR: could not infer mission end times for alignment", file=sys.stderr)
            return 1
        global_end = float(min(ends))
        print(f"INFO: using common mission-time end = {global_end:.2f}s", file=sys.stderr)

    for p in policies:
        n = load_csv_with_hints(p / "network_timeseries.csv")
        n = safe_numeric(n, ["time", "window_pdr"])
        n = align_time_seconds(n, "time")
        n = n.dropna(subset=["time", "window_pdr"])
        if args.align_mission_time == "common_window":
            n = n[n["time"] <= global_end].copy()
            end_report = policy_window_end.get(p.name, np.nan)
            if pd.notna(end_report):
                print(
                    f"INFO [{p.name}]: truncation window policy_end={end_report:.2f}s, effective_end={global_end:.2f}s",
                    file=sys.stderr,
                )

        fig, ax = plt.subplots(figsize=(10, 4))
        for run, part in n.groupby("run_id") if "run_id" in n.columns else [(p.name, n)]:
            part = part.sort_values("time")
            ax.plot(part["time"], part["window_pdr"], alpha=0.5)
            ttc = _time_to_collapse(part, args.pdr_threshold, args.min_duration)
            ttc_rows.append(
                {
                    "policy": p.name,
                    "run_id": run,
                    "time_to_collapse_s": ttc,
                    "truncation_applied": args.align_mission_time == "common_window",
                    "truncation_cutoff_s": global_end if args.align_mission_time == "common_window" else np.nan,
                }
            )
        ax.axhline(args.pdr_threshold, color="red", ls="--", label="threshold")
        ax.set_xlabel("Mission time (s)")
        ax.set_ylabel("PDR (ratio)")
        title_suffix = " (common window)" if args.align_mission_time == "common_window" else ""
        ax.set_title(f"Collapse threshold PDR timeseries: {p.name}{title_suffix}")
        ax.grid(alpha=0.3)
        ax.legend(loc="best")
        fig.tight_layout()
        out = args.out_dir / f"collapse_pdr_timeseries_{p.name}"
        out.mkdir(parents=True, exist_ok=True)
        fig.savefig(out / f"collapse_pdr_timeseries_{p.name}.png", dpi=200)
        fig.savefig(out / f"collapse_pdr_timeseries_{p.name}.pdf")
        plt.close(fig)

    ttc = pd.DataFrame(ttc_rows)
    fig, ax = plt.subplots(figsize=(10, 5))
    labels = sorted(ttc["policy"].unique().tolist())
    data = [ttc[ttc["policy"] == p]["time_to_collapse_s"].dropna().to_list() for p in labels]
    ax.boxplot(data, labels=labels)
    ax.set_xlabel("Policy")
    ax.set_ylabel("Time-to-collapse (s)")
    title_suffix = " (common window)" if args.align_mission_time == "common_window" else ""
    ax.set_title(f"Time-to-collapse by policy{title_suffix}")
    ax.grid(axis="y", alpha=0.3)
    fig.tight_layout()
    out = args.out_dir / "time_to_collapse_boxplot"
    out.mkdir(parents=True, exist_ok=True)
    fig.savefig(out / "time_to_collapse_boxplot.png", dpi=200)
    fig.savefig(out / "time_to_collapse_boxplot.pdf")
    plt.close(fig)
    ttc.to_csv(out / "collapse_stats.csv", index=False)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
