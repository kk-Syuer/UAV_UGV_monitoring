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
    return p.parse_args()


def _time_to_collapse(df: pd.DataFrame, threshold: float, min_duration: float) -> float:
    d = df.sort_values("time").copy()
    d = d.dropna(subset=["window_pdr"])
    if d.empty:
        return np.nan
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

    for p in policies:
        n = load_csv_with_hints(p / "network_timeseries.csv")
        numeric_cols = ["time", "window_pdr"]
        if "window_generated" in n.columns:
            numeric_cols.append("window_generated")
        n = safe_numeric(n, numeric_cols)
        n = align_time_seconds(n, "time")
        n = n.dropna(subset=["time"])

        mask_window_generated = (
            n["window_generated"].le(0) if "window_generated" in n.columns else pd.Series(False, index=n.index)
        )
        mask_negative_pdr = n["window_pdr"].lt(0)
        mask_any = mask_window_generated | mask_negative_pdr
        n.loc[mask_any, "window_pdr"] = np.nan
        print(
            (
                f"INFO [{p.name}]: masked window_pdr bins total={int(mask_any.sum())} "
                f"(window_generated<=0: {int(mask_window_generated.sum())}, "
                f"negative_window_pdr: {int(mask_negative_pdr.sum())})"
            ),
            file=sys.stderr,
        )

        fig, ax = plt.subplots(figsize=(10, 4))
        for run, part in n.groupby("run_id") if "run_id" in n.columns else [(p.name, n)]:
            part = part.sort_values("time")
            ax.plot(part["time"], part["window_pdr"], alpha=0.5)
            ttc = _time_to_collapse(part, args.pdr_threshold, args.min_duration)
            ttc_rows.append({"policy": p.name, "run_id": run, "time_to_collapse_s": ttc})
        ax.axhline(args.pdr_threshold, color="red", ls="--", label="threshold")
        ax.set_xlabel("Mission time (s)")
        ax.set_ylabel("PDR (ratio)")
        ax.set_title(f"Collapse threshold PDR timeseries: {p.name}")
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
    ax.set_title("Time-to-collapse by policy")
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
