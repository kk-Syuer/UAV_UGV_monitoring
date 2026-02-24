#!/usr/bin/env python3
"""Heatmap of mean UAV lifetime by policy and weather regime.

# requirements.txt snippet:
# pandas>=1.5
# numpy>=1.23
# matplotlib>=3.6
"""

from __future__ import annotations

import argparse
import json
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
    p.add_argument("--regime_mode", choices=["dominant", "fixed_from_metadata"], default="dominant")
    return p.parse_args()


def _dominant_regime(policy_dir: Path, mode: str) -> str:
    if mode == "fixed_from_metadata":
        js = policy_dir / "summary.json"
        if js.exists():
            obj = json.loads(js.read_text())
            if "regime" in obj:
                return str(obj["regime"])
    w = load_csv_with_hints(policy_dir / "weather_timeseries.csv")
    require_columns(w, ["regime"], policy_dir / "weather_timeseries.csv")
    return str(w["regime"].mode(dropna=True).iloc[0])


def _mean_lifetime(policy_dir: Path) -> float:
    s = load_csv_with_hints(policy_dir / "status_timeseries.csv")
    d = load_csv_with_hints(policy_dir / "death_events.csv")
    require_columns(s, ["uav_id", "time"], policy_dir / "status_timeseries.csv")
    require_columns(d, ["uav_id", "time"], policy_dir / "death_events.csv")
    s = safe_numeric(s, ["time"])
    d = safe_numeric(d, ["time"])
    s = align_time_seconds(s, "time")
    d = align_time_seconds(d, "time")
    first = s.groupby("uav_id", as_index=False)["time"].min().rename(columns={"time": "t0"})
    death = d.groupby("uav_id", as_index=False)["time"].min().rename(columns={"time": "t1"})
    m = first.merge(death, on="uav_id", how="inner")
    if m.empty:
        return float("nan")
    return float((m["t1"] - m["t0"]).mean())


def main() -> int:
    args = parse_args()
    try:
        policies = discover_policy_dirs(args.data_root)
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    rows = []
    for p in policies:
        regime = _dominant_regime(p, args.regime_mode)
        life = _mean_lifetime(p)
        rows.append({"policy": p.name, "regime": regime, "mean_lifetime_s": life})
    df = pd.DataFrame(rows)

    pols = sorted(df["policy"].unique().tolist())
    regs = sorted(df["regime"].unique().tolist())
    mat = np.full((len(regs), len(pols)), np.nan)
    for i, r in enumerate(regs):
        for j, p in enumerate(pols):
            v = df[(df["policy"] == p) & (df["regime"] == r)]["mean_lifetime_s"]
            if not v.empty:
                mat[i, j] = float(v.iloc[0])

    fig, ax = plt.subplots(figsize=(max(8, 1.2 * len(pols)), max(4, 0.8 * len(regs))))
    im = ax.imshow(np.ma.masked_invalid(mat), aspect="auto", cmap="magma")
    ax.set_xticks(np.arange(len(pols)))
    ax.set_xticklabels(pols, rotation=30, ha="right")
    ax.set_yticks(np.arange(len(regs)))
    ax.set_yticklabels(regs)
    ax.set_xlabel("Policy")
    ax.set_ylabel("Weather regime")
    ax.set_title("Mean UAV lifetime by weather regime and policy")
    cb = fig.colorbar(im, ax=ax)
    cb.set_label("Mean lifetime (s)")
    fig.tight_layout()

    out = args.out_dir / "lifetime_heatmap_policy_vs_weather"
    out.mkdir(parents=True, exist_ok=True)
    fig.savefig(out / "lifetime_heatmap_policy_vs_weather.png", dpi=200)
    fig.savefig(out / "lifetime_heatmap_policy_vs_weather.pdf")
    plt.close(fig)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
