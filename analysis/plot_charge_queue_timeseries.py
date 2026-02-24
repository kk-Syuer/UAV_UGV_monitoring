#!/usr/bin/env python3
"""Plot charge queue dynamics over mission time.

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

from analysis.utils_clean import align_time_seconds, drop_time_resets, safe_numeric
from analysis.utils_io import discover_policy_dirs, load_csv_with_hints, require_columns

FONT_SIZE = 12
N_BOOT = 500
RNG = np.random.default_rng(42)


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Plot charge queue metrics over mission time")
    p.add_argument("--data_root", required=True, type=Path)
    p.add_argument("--out_dir", type=Path, default=Path("analysis/test_round1"))
    p.add_argument("--metric", required=True, choices=["queue_len", "utilisation", "active_sessions", "mean_wait"])
    p.add_argument("--role_scope", default="all", choices=["all", "ch", "member"])
    return p.parse_args()


def resolve_metric_column(df: pd.DataFrame, metric: str, role_scope: str) -> tuple[str, str]:
    candidates: list[str]
    ylabel: str

    if metric == "queue_len":
        map_cols = {
            "all": ["queue_length_ugv", "queue_length"],
            "ch": ["queue_length_ch"],
            "member": ["queue_length_member"],
        }
        candidates = map_cols[role_scope]
        ylabel = "Queue length (vehicles)"
    elif metric == "utilisation":
        map_cols = {
            "all": ["ugv_dock_utilization", "dock_utilization"],
            "ch": ["ugv_dock_utilization_ch", "dock_utilization_ch"],
            "member": ["ugv_dock_utilization_member", "dock_utilization_member"],
        }
        candidates = map_cols[role_scope]
        ylabel = "Dock utilisation (ratio)"
    elif metric == "active_sessions":
        map_cols = {
            "all": ["active_charging_ugv_sessions", "active_sessions"],
            "ch": ["active_charging_ch_sessions", "active_sessions_ch"],
            "member": ["active_charging_member_sessions", "active_sessions_member"],
        }
        candidates = map_cols[role_scope]
        ylabel = "Active charging sessions (count)"
    else:  # mean_wait
        if role_scope == "all":
            if "mean_wait_ms" in df.columns:
                return "mean_wait_ms", "Mean queue wait (ms)"
            if "mean_wait_ch_ms" in df.columns and "mean_wait_member_ms" in df.columns:
                df["_mean_wait_all_ms"] = df[["mean_wait_ch_ms", "mean_wait_member_ms"]].mean(axis=1, skipna=True)
                return "_mean_wait_all_ms", "Mean queue wait (ms)"
            candidates = ["mean_wait_ugv_ms", "mean_wait"]
        elif role_scope == "ch":
            candidates = ["mean_wait_ch_ms"]
        else:
            candidates = ["mean_wait_member_ms"]
        ylabel = "Mean queue wait (ms)"

    for col in candidates:
        if col in df.columns:
            return col, ylabel

    raise ValueError(f"No compatible column found for metric={metric}, role_scope={role_scope}. Tried: {candidates}")


def bootstrap_ci(values: np.ndarray, n_boot: int = N_BOOT) -> tuple[float, float]:
    if len(values) <= 1:
        return np.nan, np.nan
    stats = np.empty(n_boot, dtype=float)
    for i in range(n_boot):
        sample = RNG.choice(values, size=len(values), replace=True)
        stats[i] = np.nanmean(sample)
    return float(np.nanpercentile(stats, 2.5)), float(np.nanpercentile(stats, 97.5))


def main() -> int:
    args = parse_args()

    plt.rcParams.update({"font.size": FONT_SIZE, "axes.labelsize": FONT_SIZE, "legend.fontsize": FONT_SIZE - 1})

    fig_name = f"charge_queue_{args.metric}_{args.role_scope}"
    target_dir = args.out_dir / fig_name
    target_dir.mkdir(parents=True, exist_ok=True)

    try:
        policies = discover_policy_dirs(args.data_root)
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    fig, ax = plt.subplots(figsize=(10, 5))

    for policy_dir in policies:
        csv_path = policy_dir / "charge_queue_timeseries.csv"
        try:
            df = load_csv_with_hints(csv_path, dtype_hints={"run_id": "string"})
            require_columns(df, ["time"], csv_path)
            col, ylabel = resolve_metric_column(df, args.metric, args.role_scope)
            require_columns(df, [col], csv_path)
        except Exception as exc:
            print(f"ERROR [{policy_dir.name}]: {exc}", file=sys.stderr)
            return 1

        df = safe_numeric(df, ["time", col])
        df = align_time_seconds(df, "time")
        df, dropped = drop_time_resets(df, "time")
        if dropped:
            print(f"WARNING [{policy_dir.name}]: dropped {dropped} rows with decreasing time", file=sys.stderr)

        series = df[["time", col] + (["run_id"] if "run_id" in df.columns else [])].dropna(subset=["time", col]).copy()

        if args.metric == "queue_len":
            neg_mask = series[col].lt(0).fillna(False)
            neg_count = int(neg_mask.sum())
            if neg_count:
                print(f"WARNING [{policy_dir.name}]: replacing {neg_count} negative queue_len rows with NaN", file=sys.stderr)
                series.loc[neg_mask, col] = np.nan
            series = series.dropna(subset=[col])

        if args.metric == "utilisation":
            out_mask = (~series[col].between(0, 1)).fillna(False)
            out_count = int(out_mask.sum())
            if out_count:
                print(f"WARNING [{policy_dir.name}]: clamping {out_count} utilisation rows outside [0,1]", file=sys.stderr)
                series[col] = series[col].clip(0, 1)

        if "run_id" in series.columns and series["run_id"].nunique(dropna=True) > 1:
            by_run = series.groupby(["run_id", "time"], as_index=False)[col].mean().sort_values(["run_id", "time"])
            mean_curve = by_run.groupby("time", as_index=False)[col].mean()
            ci_rows = []
            for t, part in by_run.groupby("time"):
                lo, hi = bootstrap_ci(part[col].to_numpy(dtype=float))
                ci_rows.append((t, lo, hi))
            ci = pd.DataFrame(ci_rows, columns=["time", "lo", "hi"]).sort_values("time")
            ax.plot(mean_curve["time"], mean_curve[col], label=policy_dir.name)
            ax.fill_between(ci["time"], ci["lo"], ci["hi"], alpha=0.2)
        else:
            agg = series.groupby("time", as_index=False)[col].mean().sort_values("time")
            ax.plot(agg["time"], agg[col], label=policy_dir.name)

    ylabel_map = {
        "queue_len": "Queue length (vehicles)",
        "utilisation": "Dock utilisation (ratio)",
        "active_sessions": "Active charging sessions (count)",
        "mean_wait": "Mean queue wait (ms)",
    }
    ax.set_xlabel("Mission time (s)")
    ax.set_ylabel(ylabel_map[args.metric])
    ax.set_title(f"Charge queue dynamics: {args.metric} ({args.role_scope})")
    ax.grid(alpha=0.3)
    ax.legend(loc="best")
    fig.tight_layout()

    png = target_dir / f"{fig_name}.png"
    pdf = target_dir / f"{fig_name}.pdf"
    fig.savefig(png, dpi=200)
    fig.savefig(pdf)
    plt.close(fig)

    print(f"Saved: {png}")
    print(f"Saved: {pdf}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
