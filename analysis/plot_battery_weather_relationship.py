#!/usr/bin/env python3
"""Battery drain relationships with weather conditions.

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
from analysis.utils_io import discover_policy_dirs, load_csv_with_hints, require_columns


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser()
    p.add_argument("--data_root", required=True, type=Path)
    p.add_argument("--out_dir", type=Path, default=Path("analysis/test_round1"))
    p.add_argument("--view", required=True, choices=["drain_vs_wind_scatter", "drain_vs_rain_scatter"])
    return p.parse_args()


def _load_policy(policy_dir: Path) -> pd.DataFrame:
    s = load_csv_with_hints(policy_dir / "status_timeseries.csv")
    w = load_csv_with_hints(policy_dir / "weather_timeseries.csv")
    require_columns(s, ["time", "uav_id", "battery_level"], policy_dir / "status_timeseries.csv")
    require_columns(w, ["time", "wind_speed", "rain_intensity"], policy_dir / "weather_timeseries.csv")
    s = safe_numeric(s, ["time", "battery_level"])
    w = safe_numeric(w, ["time", "wind_speed", "rain_intensity"])
    s = align_time_seconds(s, "time")
    w = align_time_seconds(w, "time")
    s = s.sort_values(["uav_id", "time"]).dropna(subset=["uav_id", "time", "battery_level"])

    s["dt"] = s.groupby("uav_id")["time"].diff()
    s["db"] = s.groupby("uav_id")["battery_level"].diff()
    s["drain_rate_pct_per_s"] = -(s["db"] / s["dt"])  # positive means draining
    s.loc[s["dt"].le(0), "drain_rate_pct_per_s"] = np.nan

    rates = s.groupby("time", as_index=False)["drain_rate_pct_per_s"].median()
    merged = pd.merge_asof(
        rates.sort_values("time"),
        w[["time", "wind_speed", "rain_intensity"]].sort_values("time"),
        on="time",
        direction="nearest",
    )
    merged["policy"] = policy_dir.name
    return merged.dropna(subset=["drain_rate_pct_per_s"]) 


def main() -> int:
    args = parse_args()
    try:
        policies = discover_policy_dirs(args.data_root)
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    rows = []
    for p in policies:
        try:
            rows.append(_load_policy(p))
        except Exception as exc:
            print(f"ERROR [{p.name}]: {exc}", file=sys.stderr)
            return 1
    df = pd.concat(rows, ignore_index=True)

    if args.view == "drain_vs_wind_scatter":
        xcol, out = "wind_speed", "battery_drain_vs_wind"
        xlabel = "Wind speed (m/s)"
    else:
        xcol, out = "rain_intensity", "battery_drain_vs_rain"
        xlabel = "Rain intensity (arb. units)"

    fig, ax = plt.subplots(figsize=(9, 5))
    for pol, part in df.groupby("policy"):
        ax.scatter(part[xcol], part["drain_rate_pct_per_s"], s=12, alpha=0.3, label=pol)

    fit_df = df[[xcol, "drain_rate_pct_per_s"]].dropna()
    if len(fit_df) >= 2:
        m, b = np.polyfit(fit_df[xcol], fit_df["drain_rate_pct_per_s"], 1)
        xs = np.linspace(fit_df[xcol].min(), fit_df[xcol].max(), 100)
        ax.plot(xs, m * xs + b, color="black", lw=2, label="linear fit")

    ax.set_xlabel(xlabel)
    ax.set_ylabel("Battery drain rate (%/s)")
    ax.set_title("Battery drain vs weather")
    ax.grid(alpha=0.3)
    ax.legend(loc="best")
    fig.tight_layout()

    d = args.out_dir / out
    d.mkdir(parents=True, exist_ok=True)
    fig.savefig(d / f"{out}.png", dpi=200)
    fig.savefig(d / f"{out}.pdf")
    plt.close(fig)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
