#!/usr/bin/env python3
"""Radar chart and summary table for policy-level metrics.

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

from analysis.utils_io import discover_policy_dirs, load_csv_with_hints


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser()
    p.add_argument("--data_root", required=True, type=Path)
    p.add_argument("--out_dir", type=Path, default=Path("analysis/test_round1"))
    return p.parse_args()


def _jain(x: np.ndarray) -> float:
    x = x[np.isfinite(x)]
    if len(x) == 0:
        return np.nan
    return float((x.sum() ** 2) / (len(x) * np.square(x).sum())) if np.square(x).sum() > 0 else np.nan


def _collect(policy_dir: Path) -> dict:
    rec = {"policy": policy_dir.name}
    js = policy_dir / "summary.json"
    obj = json.loads(js.read_text()) if js.exists() else {}
    n = load_csv_with_hints(policy_dir / "network_timeseries.csv")
    q = load_csv_with_hints(policy_dir / "charge_events.csv")
    d = load_csv_with_hints(policy_dir / "death_events.csv")
    rec["overall_pdr"] = float(obj.get("overall_pdr", n.get("window_pdr", pd.Series(dtype=float)).mean()))
    rec["mean_delay"] = float(obj.get("mean_delay_ms", n.get("window_delay_mean_ms", pd.Series(dtype=float)).mean()))
    rec["mean_wait"] = float(obj.get("mean_wait_ms", q.get("waiting_time_ms", pd.Series(dtype=float)).replace(0, np.nan).mean()))
    fleet = float(obj.get("fleet_size", max(1, d.get("fleet_size", pd.Series([7])).max())))
    dead = float(obj.get("death_count", len(d)))
    rec["survival_rate"] = max(0.0, 1.0 - dead / fleet)
    rec["recovery_latency"] = float(obj.get("recovery_latency_s", np.nan))
    fair = obj.get("fairness_index", None)
    if fair is None:
        waits = pd.to_numeric(q.get("waiting_time_ms", pd.Series(dtype=float)), errors="coerce").replace(0, np.nan).dropna().to_numpy()
        fair = _jain(waits) if len(waits) else np.nan
    rec["fairness_index"] = float(fair) if fair is not None else np.nan
    return rec


def main() -> int:
    args = parse_args()
    policies = discover_policy_dirs(args.data_root)
    df = pd.DataFrame([_collect(p) for p in policies])

    metrics = ["overall_pdr", "mean_delay", "mean_wait", "survival_rate", "recovery_latency", "fairness_index"]
    higher_better = {"overall_pdr": True, "mean_delay": False, "mean_wait": False, "survival_rate": True, "recovery_latency": False, "fairness_index": True}

    norm = df[["policy"] + metrics].copy()
    for m in metrics:
        col = pd.to_numeric(norm[m], errors="coerce")
        lo, hi = col.min(skipna=True), col.max(skipna=True)
        if not np.isfinite(lo) or not np.isfinite(hi) or hi == lo:
            norm[m] = 0.5
        else:
            v = (col - lo) / (hi - lo)
            norm[m] = v if higher_better[m] else (1 - v)

    angles = np.linspace(0, 2 * np.pi, len(metrics), endpoint=False).tolist()
    angles += angles[:1]
    fig, ax = plt.subplots(figsize=(7, 7), subplot_kw={"projection": "polar"})
    for _, row in norm.iterrows():
        vals = [row[m] for m in metrics] + [row[metrics[0]]]
        ax.plot(angles, vals, label=row["policy"])
        ax.fill(angles, vals, alpha=0.1)
    ax.set_xticks(angles[:-1])
    ax.set_xticklabels(metrics)
    ax.set_yticklabels([])
    ax.set_title("Policy radar (normalized, higher=better)")
    ax.legend(loc="upper right", bbox_to_anchor=(1.3, 1.1))
    fig.tight_layout()

    out = args.out_dir / "policy_radar"
    out.mkdir(parents=True, exist_ok=True)
    fig.savefig(out / "policy_radar.png", dpi=200)
    fig.savefig(out / "policy_radar.pdf")
    plt.close(fig)
    df.to_csv(out / "policy_summary_table.csv", index=False)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
