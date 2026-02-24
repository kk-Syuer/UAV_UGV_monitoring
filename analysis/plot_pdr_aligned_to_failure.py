#!/usr/bin/env python3
"""PDR recovery curve aligned to CH failure events.

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

from analysis.utils_clean import align_time_seconds, normalize_role, safe_numeric
from analysis.utils_io import discover_policy_dirs, load_csv_with_hints

RNG = np.random.default_rng(42)


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser()
    p.add_argument("--data_root", required=True, type=Path)
    p.add_argument("--out_dir", type=Path, default=Path("analysis/test_round1"))
    p.add_argument("--pre_window", type=float, default=60.0)
    p.add_argument("--post_window", type=float, default=180.0)
    return p.parse_args()


def _failure_times(policy_dir: Path) -> list[float]:
    s_path = policy_dir / "status_timeseries.csv"
    if s_path.exists():
        s = load_csv_with_hints(s_path)
        if {"time", "role", "backbone_active"}.issubset(s.columns):
            s = safe_numeric(s, ["time", "backbone_active"])
            s["role_label"] = normalize_role(s["role"])
            s = align_time_seconds(s, "time").sort_values("time")
            ch = s[s["role_label"] == "CH"].copy()
            ch["prev"] = ch["backbone_active"].shift(1)
            ts = ch[(ch["prev"] == 1) & (ch["backbone_active"] == 0)]["time"].dropna().tolist()
            if ts:
                return ts
    d = load_csv_with_hints(policy_dir / "death_events.csv")
    d = safe_numeric(d, ["time"])
    if "role" in d.columns:
        d["role_label"] = normalize_role(d["role"])
        d = d[d["role_label"] == "CH"]
    d = align_time_seconds(d, "time")
    return d["time"].dropna().tolist()


def _bootstrap_ci(vals: np.ndarray, nboot: int = 500) -> tuple[float, float]:
    if len(vals) <= 1:
        return np.nan, np.nan
    samples = [np.nanmean(RNG.choice(vals, size=len(vals), replace=True)) for _ in range(nboot)]
    return float(np.percentile(samples, 2.5)), float(np.percentile(samples, 97.5))


def main() -> int:
    args = parse_args()
    policies = discover_policy_dirs(args.data_root)

    fig, ax = plt.subplots(figsize=(10, 5))
    for p in policies:
        n = load_csv_with_hints(p / "network_timeseries.csv")
        n = safe_numeric(n, ["time", "window_pdr"])
        n = align_time_seconds(n, "time")
        n = n.dropna(subset=["time", "window_pdr"])
        fails = _failure_times(p)
        if not fails:
            continue
        clips = []
        for t0 in fails:
            part = n[(n["time"] >= t0 - args.pre_window) & (n["time"] <= t0 + args.post_window)].copy()
            if part.empty:
                continue
            part["t_rel"] = part["time"] - t0
            clips.append(part[["t_rel", "window_pdr"]])
        if not clips:
            continue
        joined = pd.concat(clips, ignore_index=True)
        mean = joined.groupby("t_rel", as_index=False)["window_pdr"].mean().sort_values("t_rel")
        ci_rows = []
        for t, grp in joined.groupby("t_rel"):
            lo, hi = _bootstrap_ci(grp["window_pdr"].to_numpy(dtype=float))
            ci_rows.append((t, lo, hi))
        ci = pd.DataFrame(ci_rows, columns=["t_rel", "lo", "hi"]).sort_values("t_rel")
        ax.plot(mean["t_rel"], mean["window_pdr"], label=p.name)
        ax.fill_between(ci["t_rel"], ci["lo"], ci["hi"], alpha=0.2)

    ax.axvline(0, color="black", ls="--", lw=1)
    ax.set_xlabel("Aligned time from failure t0 (s)")
    ax.set_ylabel("PDR (ratio)")
    ax.set_title("PDR recovery aligned to CH failure")
    ax.grid(alpha=0.3)
    ax.legend(loc="best")
    fig.tight_layout()
    out = args.out_dir / "pdr_aligned_failure"
    out.mkdir(parents=True, exist_ok=True)
    fig.savefig(out / "pdr_aligned_failure.png", dpi=200)
    fig.savefig(out / "pdr_aligned_failure.pdf")
    plt.close(fig)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
