#!/usr/bin/env python3
"""Stepwise data cleaning, KPI computation, and plotting for round1 experiment logs."""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import Dict, List, Optional

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns

try:
    from lifelines import KaplanMeierFitter
except Exception:  # pragma: no cover
    KaplanMeierFitter = None

NA_VALUES = [-1, "", "null", "NULL", "None"]


CSV_FILES = {
    "network": "network_timeseries.csv",
    "queue": "charge_queue_timeseries.csv",
    "charge": "charge_events.csv",
    "status": "status_timeseries.csv",
    "death": "death_events.csv",
    "recovery": "recovery_events.csv",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Round1 metrics analysis and visualization")
    parser.add_argument(
        "--input-root",
        type=Path,
        default=Path("experiment_data_collection/test_round1"),
        help="Directory containing per-policy run folders",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("analysis/round1_stepwise"),
        help="Directory to save computed metrics and figures",
    )
    parser.add_argument(
        "--policy-map",
        type=str,
        default="",
        help="Optional comma-separated mapping run_name=PolicyLabel",
    )
    return parser.parse_args()


def parse_policy_map(policy_map_text: str) -> Dict[str, str]:
    if not policy_map_text.strip():
        return {}
    mapping: Dict[str, str] = {}
    for token in policy_map_text.split(","):
        if "=" not in token:
            continue
        run_name, label = token.split("=", 1)
        mapping[run_name.strip()] = label.strip()
    return mapping


def infer_policy(run_name: str, run_id: Optional[str], policy_map: Dict[str, str]) -> str:
    if run_name in policy_map:
        return policy_map[run_name]
    if run_id and run_id in policy_map:
        return policy_map[run_id]

    source = (run_id or run_name).lower()
    if "fcfs" in source:
        return "FCFS"
    if "p_edf" in source or source.startswith("ugv_p_edf"):
        return "Preemptive EDF"
    if "edf" in source:
        return "EDF"
    if "dynamic" in source and "p_" in source:
        return "Preemptive Dynamic Score"
    if "dynamic" in source:
        return "Dynamic Score"
    if "role" in source and "priority" in source and "p_" in source:
        return "Preemptive Role Priority"
    if "role" in source and "priority" in source:
        return "Role Priority"
    return run_id or run_name


def read_table(path: Path) -> pd.DataFrame:
    if not path.exists():
        return pd.DataFrame()
    return pd.read_csv(path, na_values=NA_VALUES)


def coerce_time_column(df: pd.DataFrame) -> pd.DataFrame:
    if df.empty:
        return df
    out = df.copy()
    for c in ["time", "creation_time", "creation_time_s", "window_sec", "request_time"]:
        if c in out.columns:
            out[c] = pd.to_numeric(out[c], errors="coerce")
    if "time" not in out.columns:
        if "creation_time" in out.columns:
            out["time"] = out["creation_time"]
        elif "creation_time_s" in out.columns:
            out["time"] = out["creation_time_s"]
        elif "window_sec" in out.columns:
            out["time"] = out["window_sec"]
    return out


def load_runs(input_root: Path, policy_map: Dict[str, str]) -> Dict[str, pd.DataFrame]:
    bundles: Dict[str, List[pd.DataFrame]] = {k: [] for k in CSV_FILES}
    for run_dir in sorted(p for p in input_root.iterdir() if p.is_dir()):
        tables = {name: coerce_time_column(read_table(run_dir / filename)) for name, filename in CSV_FILES.items()}
        if all(df.empty for df in tables.values()):
            continue

        run_id = None
        for df in tables.values():
            if not df.empty and "run_id" in df.columns:
                run_id = str(df["run_id"].dropna().astype(str).head(1).iloc[0])
                break

        policy = infer_policy(run_dir.name, run_id, policy_map)
        for df in tables.values():
            if df.empty:
                continue
            if "run_id" not in df.columns:
                df["run_id"] = run_id or run_dir.name
            df["policy"] = policy

        for name, df in tables.items():
            if not df.empty:
                bundles[name].append(df)

    return {k: (pd.concat(v, ignore_index=True) if v else pd.DataFrame()) for k, v in bundles.items()}


def merge_network_queue(network: pd.DataFrame, queue: pd.DataFrame) -> pd.DataFrame:
    if network.empty or queue.empty or "time" not in network.columns or "time" not in queue.columns:
        return pd.DataFrame()

    net = network.sort_values(["run_id", "time"]).copy()
    que = queue.sort_values(["run_id", "time"]).copy()
    return pd.merge_asof(
        net,
        que,
        on="time",
        by="run_id",
        direction="nearest",
        suffixes=("_net", "_queue"),
    )


def compute_waiting_summary(charge: pd.DataFrame) -> pd.DataFrame:
    if charge.empty or "waiting_time_ms" not in charge.columns:
        return pd.DataFrame()
    out = charge.copy()
    out["waiting_time_ms"] = pd.to_numeric(out["waiting_time_ms"], errors="coerce")
    out["role_label"] = out.get("role", pd.Series(dtype="float")).map({1: "Cluster Head", 0: "Member"}).fillna("Unknown")

    summary = (
        out.groupby(["policy", "role_label"], dropna=False)["waiting_time_ms"]
        .agg(["count", "mean", "median", "std", "min", "max"])
        .reset_index()
    )

    fairness = (
        out.groupby(["policy", "role_label"], dropna=False)["waiting_time_ms"].mean().unstack(fill_value=np.nan)
    )
    if {"Cluster Head", "Member"}.issubset(set(fairness.columns)):
        fairness["fairness_gap_member_minus_ch_ms"] = fairness["Member"] - fairness["Cluster Head"]
        fairness = fairness.reset_index()
        summary = summary.merge(fairness[["policy", "fairness_gap_member_minus_ch_ms"]], on="policy", how="left")

    return summary


def compute_lifetimes(status: pd.DataFrame, death: pd.DataFrame) -> pd.DataFrame:
    if status.empty or "uav_id" not in status.columns or "time" not in status.columns:
        return pd.DataFrame()

    s = status.copy()
    s["time"] = pd.to_numeric(s["time"], errors="coerce")
    start = s.groupby(["run_id", "policy", "uav_id"], dropna=False)["time"].min().rename("start_time")
    end = s.groupby(["run_id", "policy", "uav_id"], dropna=False)["time"].max().rename("last_seen_time")
    life = pd.concat([start, end], axis=1).reset_index()

    if not death.empty and {"run_id", "uav_id", "time"}.issubset(death.columns):
        d = death[["run_id", "uav_id", "time"]].copy()
        d["time"] = pd.to_numeric(d["time"], errors="coerce")
        d = d.rename(columns={"time": "death_time"})
        life = life.merge(d, on=["run_id", "uav_id"], how="left")
    else:
        life["death_time"] = np.nan

    life["event"] = (~life["death_time"].isna()).astype(int)
    life["end_time"] = life["death_time"].fillna(life["last_seen_time"])
    life["lifetime"] = life["end_time"] - life["start_time"]
    return life


def compute_recovery_delay(recovery: pd.DataFrame, status: pd.DataFrame) -> pd.DataFrame:
    if recovery.empty or status.empty:
        return pd.DataFrame()

    rec = recovery.copy()
    rec["event_time"] = pd.to_numeric(rec.get("creation_time", rec.get("time")), errors="coerce")
    st = status.copy()
    st["time"] = pd.to_numeric(st["time"], errors="coerce")
    st["backbone_active"] = st["backbone_active"].astype(str).str.lower().isin(["1", "true", "t"])

    records = []
    for row in rec.itertuples(index=False):
        run_id = getattr(row, "run_id", None)
        event_time = getattr(row, "event_time", np.nan)
        if pd.isna(event_time) or run_id is None:
            continue

        candidates = st[
            (st["run_id"] == run_id)
            & (st["time"] >= event_time)
            & (st.get("role", 0) == 1)
            & (st["backbone_active"])
        ]
        if candidates.empty:
            continue

        recovery_time = candidates["time"].min()
        records.append(
            {
                "run_id": run_id,
                "policy": getattr(row, "policy", "Unknown"),
                "event_time": event_time,
                "recovery_time": recovery_time,
                "recovery_delay_s": recovery_time - event_time,
                "control_type": getattr(row, "control_type", ""),
            }
        )

    return pd.DataFrame(records)


def ensure_output_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def save_plot_pdr(merged: pd.DataFrame, output_dir: Path) -> None:
    if merged.empty or "window_pdr" not in merged.columns:
        return
    plt.figure(figsize=(10, 5))
    sns.lineplot(data=merged, x="time", y="window_pdr", hue="policy")
    plt.xlabel("Time (s)")
    plt.ylabel("Packet Delivery Ratio")
    plt.title("PDR over Time by Policy")
    plt.tight_layout()
    plt.savefig(output_dir / "pdr_over_time.png", dpi=300)
    plt.close()


def save_plot_waiting_distribution(charge: pd.DataFrame, output_dir: Path) -> None:
    if charge.empty or "waiting_time_ms" not in charge.columns:
        return

    c = charge.copy()
    c["waiting_time_ms"] = pd.to_numeric(c["waiting_time_ms"], errors="coerce")
    c["role_label"] = c.get("role", pd.Series(dtype="float")).map({1: "Cluster Head", 0: "Member"}).fillna("Unknown")

    plt.figure(figsize=(10, 6))
    sns.violinplot(data=c, x="policy", y="waiting_time_ms", hue="role_label", cut=0)
    plt.xlabel("Scheduling Policy")
    plt.ylabel("Waiting Time (ms)")
    plt.title("Waiting Time Distribution by Policy and Role")
    plt.xticks(rotation=20, ha="right")
    plt.tight_layout()
    plt.savefig(output_dir / "waiting_time_distribution.png", dpi=300)
    plt.close()


def save_plot_survival(lifetimes: pd.DataFrame, output_dir: Path) -> None:
    if lifetimes.empty or "lifetime" not in lifetimes.columns:
        return

    plt.figure(figsize=(10, 6))
    if KaplanMeierFitter is not None:
        kmf = KaplanMeierFitter()
        for policy, grp in lifetimes.groupby("policy"):
            valid = grp.dropna(subset=["lifetime"])
            if valid.empty:
                continue
            kmf.fit(valid["lifetime"], event_observed=valid["event"], label=policy)
            kmf.plot_survival_function(ci_show=False)
    else:
        for policy, grp in lifetimes.groupby("policy"):
            data = grp.dropna(subset=["lifetime"]).sort_values("lifetime")
            if data.empty:
                continue
            n = len(data)
            at_risk = n
            surv = 1.0
            xs = [0.0]
            ys = [1.0]
            for t, tg in data.groupby("lifetime"):
                d = int((tg["event"] == 1).sum())
                c = int((tg["event"] == 0).sum())
                if at_risk > 0 and d > 0:
                    surv *= 1.0 - d / at_risk
                    xs.append(float(t))
                    ys.append(surv)
                at_risk -= d + c
            plt.step(xs, ys, where="post", label=policy)

    plt.xlabel("Time (s)")
    plt.ylabel("Survival Probability")
    plt.title("Kaplan–Meier Survival Curves")
    plt.ylim(0, 1.02)
    plt.legend()
    plt.tight_layout()
    plt.savefig(output_dir / "survival_curves.png", dpi=300)
    plt.close()


def save_plot_queue(queue: pd.DataFrame, output_dir: Path) -> None:
    if queue.empty or "time" not in queue.columns:
        return

    q = queue.copy()
    q["mean_wait_ms"] = pd.to_numeric(
        q.get("mean_wait_member_ms", np.nan), errors="coerce"
    ).fillna(pd.to_numeric(q.get("mean_wait_ch_ms", np.nan), errors="coerce"))

    fig, axes = plt.subplots(2, 1, sharex=True, figsize=(11, 7))
    if "queue_length_ugv" in q.columns:
        sns.lineplot(ax=axes[0], data=q, x="time", y="queue_length_ugv", hue="policy")
    axes[0].set_ylabel("Queue Length")

    sns.lineplot(ax=axes[1], data=q, x="time", y="mean_wait_ms", hue="policy")
    axes[1].set_ylabel("Mean Wait (ms)")
    axes[1].set_xlabel("Time (s)")
    plt.suptitle("Queue Metrics Over Time")
    plt.tight_layout()
    plt.savefig(output_dir / "queue_metrics.png", dpi=300)
    plt.close(fig)


def save_metrics_csvs(output_dir: Path, merged: pd.DataFrame, wait_summary: pd.DataFrame,
                     lifetimes: pd.DataFrame, recovery_delay: pd.DataFrame) -> None:
    if not merged.empty:
        merged.to_csv(output_dir / "network_queue_merged.csv", index=False)
    if not wait_summary.empty:
        wait_summary.to_csv(output_dir / "waiting_time_summary.csv", index=False)
    if not lifetimes.empty:
        lifetimes.to_csv(output_dir / "uav_lifetimes.csv", index=False)
    if not recovery_delay.empty:
        recovery_delay.to_csv(output_dir / "recovery_delay.csv", index=False)


def main() -> None:
    args = parse_args()
    policy_map = parse_policy_map(args.policy_map)
    ensure_output_dir(args.output_dir)

    tables = load_runs(args.input_root, policy_map)
    merged = merge_network_queue(tables["network"], tables["queue"])
    wait_summary = compute_waiting_summary(tables["charge"])
    lifetimes = compute_lifetimes(tables["status"], tables["death"])
    recovery_delay = compute_recovery_delay(tables["recovery"], tables["status"])

    save_metrics_csvs(args.output_dir, merged, wait_summary, lifetimes, recovery_delay)
    save_plot_pdr(merged, args.output_dir)
    save_plot_waiting_distribution(tables["charge"], args.output_dir)
    save_plot_survival(lifetimes, args.output_dir)
    save_plot_queue(tables["queue"], args.output_dir)

    print(f"Saved outputs to: {args.output_dir}")


if __name__ == "__main__":
    main()
