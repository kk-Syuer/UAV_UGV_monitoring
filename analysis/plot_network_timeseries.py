#!/usr/bin/env python3
"""Plot network robustness time series from network_timeseries.csv.

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

from analysis.utils_clean import align_time_seconds, drop_time_resets, normalize_role, safe_numeric
from analysis.utils_io import discover_policy_dirs, load_csv_with_hints, require_columns

FONT_SIZE = 12
N_BOOT = 500
RNG = np.random.default_rng(42)
FLOW_CHOICES = ["HEARTBEAT", "CHARGE_REQUEST", "CHARGE_DECISION", "ALL"]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Plot network metrics over mission time")
    parser.add_argument("--data_root", required=True, type=Path)
    parser.add_argument("--out_dir", type=Path, default=Path("analysis/test_round1"))
    parser.add_argument("--metric", required=True, choices=["pdr", "delay", "jitter"])
    parser.add_argument("--flow_filter", default="ALL", choices=FLOW_CHOICES)
    parser.add_argument(
        "--shade_ch_failures",
        action="store_true",
        help="Annotate CH failure periods (status_timeseries preferred) or CH death times.",
    )
    return parser.parse_args()


def metric_column(df: pd.DataFrame, metric: str, flow_filter: str) -> tuple[str, str]:
    if flow_filter == "ALL":
        candidates = {
            "pdr": ["window_pdr"],
            "delay": ["window_delay_mean_ms", "window_delay_mean_s"],
            "jitter": ["window_jitter_ms", "window_jitter_s"],
        }[metric]
    else:
        flow_map = {
            "HEARTBEAT": "heartbeat",
            "CHARGE_REQUEST": "ctrl_charge_req",
            "CHARGE_DECISION": "ctrl_charge_dec",
        }
        prefix = flow_map[flow_filter]
        suffix = {"pdr": "pdr", "delay": "delay_mean_ms", "jitter": "jitter_ms"}[metric]
        candidates = [f"{prefix}_{suffix}"]

    for col in candidates:
        if col in df.columns:
            unit = "ms" if col.endswith("_ms") else ("s" if col.endswith("_s") else "ratio")
            return col, unit

    raise ValueError(f"Could not find column for metric={metric}, flow_filter={flow_filter}. Tried: {candidates}")


def bootstrap_ci(values: np.ndarray, n_boot: int = N_BOOT) -> tuple[float, float]:
    if len(values) <= 1:
        return np.nan, np.nan
    stats = np.empty(n_boot, dtype=float)
    for i in range(n_boot):
        sample = RNG.choice(values, size=len(values), replace=True)
        stats[i] = np.nanmean(sample)
    return float(np.nanpercentile(stats, 2.5)), float(np.nanpercentile(stats, 97.5))


def load_ch_failure_windows_from_status(policy_dir: Path) -> list[tuple[float, float]]:
    status_csv = policy_dir / "status_timeseries.csv"
    if not status_csv.exists():
        return []

    status_df = load_csv_with_hints(status_csv)
    require_columns(status_df, ["time", "role", "backbone_active"], status_csv)
    status_df = safe_numeric(status_df, ["time", "backbone_active"])
    status_df["role_label"] = normalize_role(status_df["role"])
    status_df = align_time_seconds(status_df, "time")
    status_df, _ = drop_time_resets(status_df, "time")

    ch = status_df.loc[
        status_df["role_label"].eq("CH") & status_df["time"].notna() & status_df["backbone_active"].notna(),
        ["time", "backbone_active"],
    ].copy()
    if ch.empty:
        return []

    ch = ch.sort_values("time")
    ch["down"] = ch["backbone_active"].eq(0)

    windows: list[tuple[float, float]] = []
    in_window = False
    start = np.nan
    prev_t = np.nan
    for t, down in ch[["time", "down"]].itertuples(index=False):
        if down and not in_window:
            start = float(t)
            in_window = True
        elif (not down) and in_window:
            windows.append((start, float(t)))
            in_window = False
        prev_t = float(t)

    if in_window and pd.notna(prev_t):
        windows.append((start, float(prev_t)))
    return [(a, b) for a, b in windows if b >= a]


def load_ch_death_times(policy_dir: Path) -> list[float]:
    death_csv = policy_dir / "death_events.csv"
    if not death_csv.exists():
        return []

    death_df = load_csv_with_hints(death_csv)
    require_columns(death_df, ["role", "time"], death_csv)

    death_df["role_label"] = normalize_role(death_df["role"])
    death_df = safe_numeric(death_df, ["time"])
    death_df = align_time_seconds(death_df, "time")
    death_df, _ = drop_time_resets(death_df, "time")

    return sorted(death_df.loc[death_df["role_label"].eq("CH") & death_df["time"].notna(), "time"].tolist())


def annotate_ch_failures(ax: plt.Axes, policy_dir: Path) -> None:
    status_csv = policy_dir / "status_timeseries.csv"
    if status_csv.exists():
        try:
            windows = load_ch_failure_windows_from_status(policy_dir)
        except Exception as exc:
            print(f"WARNING [{policy_dir.name}]: could not parse CH failure windows from status_timeseries.csv: {exc}", file=sys.stderr)
            windows = []
        for t0, t1 in windows:
            ax.axvspan(t0, t1, color="#808080", alpha=0.12, lw=0)
        if windows:
            return

    try:
        death_times = load_ch_death_times(policy_dir)
    except Exception as exc:
        print(f"WARNING [{policy_dir.name}]: could not parse CH death times from death_events.csv: {exc}", file=sys.stderr)
        death_times = []

    for t in death_times:
        ax.axvline(t, color="#808080", alpha=0.35, lw=1.0, linestyle="--")


def main() -> int:
    args = parse_args()

    plt.rcParams.update({"font.size": FONT_SIZE, "axes.labelsize": FONT_SIZE, "legend.fontsize": FONT_SIZE - 1})
    fig_name = f"network_{args.metric}_timeseries"
    out_dir = args.out_dir / fig_name
    out_dir.mkdir(parents=True, exist_ok=True)

    try:
        policies = discover_policy_dirs(args.data_root)
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    fig, ax = plt.subplots(figsize=(10, 5))
    y_unit = ""

    for policy_dir in policies:
        csv_path = policy_dir / "network_timeseries.csv"
        try:
            df = load_csv_with_hints(csv_path, dtype_hints={"run_id": "string"})
            require_columns(df, ["time"], csv_path)
            col, unit = metric_column(df, args.metric, args.flow_filter)
            require_columns(df, [col], csv_path)
        except Exception as exc:
            print(f"ERROR [{policy_dir.name}]: {exc}", file=sys.stderr)
            return 1

        y_unit = "(ratio)" if args.metric == "pdr" else f"({unit})"
        df = safe_numeric(df, ["time", col])
        df = align_time_seconds(df, "time")
        df, dropped = drop_time_resets(df, "time")
        if dropped:
            print(f"WARNING [{policy_dir.name}]: dropped {dropped} rows with decreasing time", file=sys.stderr)

        series = df[["time", col] + (["run_id"] if "run_id" in df.columns else [])].dropna(subset=["time", col]).copy()
        if args.metric == "pdr":
            outside = (~series[col].between(0, 1)).sum()
            if outside:
                print(f"WARNING [{policy_dir.name}]: {int(outside)} PDR rows outside [0,1], clamping", file=sys.stderr)
            series[col] = series[col].clip(0, 1)

        if "run_id" in series.columns and series["run_id"].nunique(dropna=True) > 1:
            by_run = (
                series.groupby(["run_id", "time"], as_index=False)[col]
                .mean()
                .sort_values(["run_id", "time"])
            )
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

        if args.shade_ch_failures:
            annotate_ch_failures(ax, policy_dir)

    ylabel_map = {
        "pdr": f"Packet Delivery Ratio {y_unit}",
        "delay": f"Mean Delay {y_unit}",
        "jitter": f"Jitter {y_unit}",
    }
    ax.set_xlabel("Mission time (s)")
    ax.set_ylabel(ylabel_map[args.metric])
    ax.set_title(f"Network {args.metric.upper()} over mission time")
    ax.grid(alpha=0.3)
    ax.legend(loc="best")
    fig.tight_layout()

    png_path = out_dir / f"{fig_name}.png"
    pdf_path = out_dir / f"{fig_name}.pdf"
    svg_path = out_dir / f"{fig_name}.svg"
    fig.savefig(png_path, dpi=200)
    fig.savefig(pdf_path)
    fig.savefig(svg_path)
    plt.close(fig)
    print(f"Saved: {png_path}")
    print(f"Saved: {pdf_path}")
    print(f"Saved: {svg_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
