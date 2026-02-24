#!/usr/bin/env python3
"""Recovery latency boxplot by policy.

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

from tools.utils_clean import align_time_seconds, normalize_role, safe_numeric
from tools.utils_io import discover_policy_dirs, load_csv_with_hints, require_columns


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser()
    p.add_argument("--data_root", required=True, type=Path)
    p.add_argument("--out_dir", type=Path, default=Path("analysis/test_round1"))
    return p.parse_args()


def _infer_completion_from_status(policy_dir: Path, trigger_times: list[float]) -> list[float]:
    s_path = policy_dir / "status_timeseries.csv"
    if not s_path.exists():
        return []
    s = load_csv_with_hints(s_path)
    require_columns(s, ["time", "role", "backbone_active"], s_path)
    s = safe_numeric(s, ["time", "backbone_active"])
    s["role_label"] = normalize_role(s["role"])
    s = align_time_seconds(s, "time")
    ch_up = sorted(s[(s["role_label"] == "CH") & (s["backbone_active"] == 1)]["time"].dropna().tolist())
    out = []
    j = 0
    for t in sorted(trigger_times):
        while j < len(ch_up) and ch_up[j] < t:
            j += 1
        if j < len(ch_up):
            out.append(ch_up[j])
    return out


def _policy_latencies(policy_dir: Path) -> list[float]:
    rec = load_csv_with_hints(policy_dir / "recovery_events.csv")
    require_columns(rec, ["control_type", "creation_time"], policy_dir / "recovery_events.csv")
    rec = safe_numeric(rec, ["creation_time", "epoch"])
    rec = rec.rename(columns={"creation_time": "time"})
    rec = align_time_seconds(rec, "time")
    trig = rec[rec["control_type"].astype(str).str.contains("TRIGGERED", na=False)]
    done = rec[rec["control_type"].astype(str).str.contains("RESPAWN_COMPLETED|COMPLETED", regex=True, na=False)]

    lat = []
    if "msg_id" in rec.columns:
        for _, trow in trig.iterrows():
            key = str(trow.get("msg_id", "")).split("_TRIGGERED")[0]
            cand = done[done.get("msg_id", pd.Series(dtype=str)).astype(str).str.startswith(key)]
            if not cand.empty:
                lat.append(float(cand.iloc[0]["time"] - trow["time"]))
    if not lat:
        trigger_times = sorted(trig["time"].dropna().tolist())
        done_times = sorted(done["time"].dropna().tolist())
        if not done_times:
            done_times = _infer_completion_from_status(policy_dir, trigger_times)
        j = 0
        for t in trigger_times:
            while j < len(done_times) and done_times[j] < t:
                j += 1
            if j < len(done_times):
                lat.append(done_times[j] - t)
                j += 1
    return [x for x in lat if x >= 0]


def main() -> int:
    args = parse_args()
    policies = discover_policy_dirs(args.data_root)
    labels, data = [], []
    table = []
    for p in policies:
        lat = _policy_latencies(p)
        labels.append(p.name)
        data.append(lat)
        table.append({"policy": p.name, "n": len(lat), "mean_latency_s": float(np.mean(lat)) if lat else np.nan})

    fig, ax = plt.subplots(figsize=(10, 5))
    ax.boxplot(data, labels=labels)
    ax.set_xlabel("Policy")
    ax.set_ylabel("Recovery latency (s)")
    ax.set_title("Recovery latency distribution by policy")
    ax.grid(axis="y", alpha=0.3)
    fig.tight_layout()

    out = args.out_dir / "recovery_latency_boxplot"
    out.mkdir(parents=True, exist_ok=True)
    fig.savefig(out / "recovery_latency_boxplot.png", dpi=200)
    fig.savefig(out / "recovery_latency_boxplot.pdf")
    plt.close(fig)
    pd.DataFrame(table).to_csv(out / "recovery_latency_table.csv", index=False)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
