#!/usr/bin/env python3
"""Generate comparison graphs for charging protocols from network monitor logs."""

from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional

import matplotlib.pyplot as plt
import pandas as pd


@dataclass
class RunData:
    run_id: str
    path: Path
    frame: pd.DataFrame


REQUIRED_COLUMNS = {
    "run_id",
    "outcome",
    "failure_reason",
    "decision_policy",
    "decision_latency_ms",
    "waiting_time_ms",
    "energy_recovered",
    "decision_ctrl_pdr",
    "decision_ctrl_delay_mean_ms",
    "charge_completed",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Create comparison charts for charging protocols.")
    parser.add_argument(
        "--log-root",
        type=Path,
        default=Path("log"),
        help="Root directory containing run_id subfolders.")
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("analysis/charging_protocol_comparison"),
        help="Directory to store generated charts.")
    parser.add_argument(
        "--protocol-map",
        type=Path,
        default=None,
        help="Optional CSV mapping with columns run_id,protocol.")
    parser.add_argument(
        "--run-ids",
        nargs="*",
        default=None,
        help="Specific run_id folders to include.")
    parser.add_argument(
        "--min-samples",
        type=int,
        default=3,
        help="Minimum samples per protocol to render box plots.")
    return parser.parse_args()


def load_protocol_map(path: Optional[Path]) -> Dict[str, str]:
    if path is None:
        return {}
    mapping: Dict[str, str] = {}
    with path.open(newline="") as handle:
        reader = csv.DictReader(handle)
        if "run_id" not in reader.fieldnames or "protocol" not in reader.fieldnames:
            raise ValueError("Protocol map must include run_id and protocol columns.")
        for row in reader:
            if row.get("run_id") and row.get("protocol"):
                mapping[row["run_id"].strip()] = row["protocol"].strip()
    return mapping


def find_runs(log_root: Path, run_ids: Optional[List[str]]) -> Iterable[Path]:
    if run_ids:
        for run_id in run_ids:
            yield log_root / run_id
        return
    for path in sorted(log_root.iterdir()):
        if path.is_dir():
            yield path


def read_charge_events(path: Path) -> pd.DataFrame:
    frame = pd.read_csv(path)
    for column in REQUIRED_COLUMNS:
        if column not in frame.columns:
            frame[column] = pd.NA
    return frame


def collect_runs(log_root: Path, run_ids: Optional[List[str]]) -> List[RunData]:
    runs: List[RunData] = []
    for run_path in find_runs(log_root, run_ids):
        csv_path = run_path / "charge_events.csv"
        if not csv_path.exists():
            continue
        frame = read_charge_events(csv_path)
        run_id = frame["run_id"].iloc[0] if not frame.empty else run_path.name
        runs.append(RunData(run_id=run_id, path=run_path, frame=frame))
    return runs


def assign_protocol(frame: pd.DataFrame, protocol_map: Dict[str, str]) -> pd.Series:
    run_id = frame["run_id"].fillna("unknown")
    protocol = frame["decision_policy"].fillna("").astype(str).str.strip()
    protocol = protocol.where(protocol != "", run_id)
    protocol = protocol.replace(protocol_map)
    return protocol


def filter_positive(series: pd.Series) -> pd.Series:
    numeric = pd.to_numeric(series, errors="coerce")
    return numeric[numeric >= 0]


def plot_outcome_distribution(frame: pd.DataFrame, output_dir: Path) -> None:
    outcomes = frame.pivot_table(
        index="protocol",
        columns="outcome",
        values="request_msg_id",
        aggfunc="count",
        fill_value=0,
    )
    totals = outcomes.sum(axis=1).replace(0, pd.NA)
    frac = outcomes.div(totals, axis=0).fillna(0)

    ax = frac.plot(kind="bar", stacked=True, figsize=(10, 6), colormap="tab20")
    ax.set_ylabel("Share of sessions")
    ax.set_xlabel("Charging protocol")
    ax.set_title("Charging outcome distribution")
    ax.legend(title="Outcome", bbox_to_anchor=(1.02, 1), loc="upper left")
    plt.tight_layout()
    plt.savefig(output_dir / "outcome_distribution.png", dpi=150)
    plt.close()


def plot_latency_boxplots(frame: pd.DataFrame, output_dir: Path, min_samples: int) -> None:
    metrics = {
        "waiting_time_ms": "Queue waiting time (ms)",
        "decision_latency_ms": "Decision latency (ms)",
    }
    fig, axes = plt.subplots(1, 2, figsize=(12, 5), sharey=False)
    for ax, (column, label) in zip(axes, metrics.items()):
        data = []
        labels = []
        for protocol, group in frame.groupby("protocol"):
            values = filter_positive(group[column])
            if len(values) < min_samples:
                continue
            data.append(values)
            labels.append(protocol)
        ax.boxplot(data, labels=labels, showfliers=False)
        ax.set_title(label)
        ax.set_xlabel("Charging protocol")
        ax.tick_params(axis="x", rotation=35)
    fig.suptitle("Charging latency comparison")
    plt.tight_layout()
    plt.savefig(output_dir / "latency_boxplots.png", dpi=150)
    plt.close()


def plot_energy_recovered(frame: pd.DataFrame, output_dir: Path, min_samples: int) -> None:
    data = []
    labels = []
    for protocol, group in frame.groupby("protocol"):
        values = filter_positive(group["energy_recovered"])
        if len(values) < min_samples:
            continue
        data.append(values)
        labels.append(protocol)
    plt.figure(figsize=(8, 5))
    plt.boxplot(data, labels=labels, showfliers=False)
    plt.ylabel("Energy recovered (battery fraction)")
    plt.xlabel("Charging protocol")
    plt.title("Energy recovered per session")
    plt.xticks(rotation=35)
    plt.tight_layout()
    plt.savefig(output_dir / "energy_recovered.png", dpi=150)
    plt.close()


def plot_qos_context(frame: pd.DataFrame, output_dir: Path) -> None:
    agg = frame.groupby("protocol").agg(
        mean_ctrl_pdr=("decision_ctrl_pdr", "mean"),
        mean_ctrl_delay=("decision_ctrl_delay_mean_ms", "mean"),
    )

    fig, axes = plt.subplots(1, 2, figsize=(12, 5))
    agg["mean_ctrl_pdr"].plot(kind="bar", ax=axes[0], color="#4C78A8")
    axes[0].set_ylabel("Mean control PDR")
    axes[0].set_xlabel("Charging protocol")
    axes[0].set_ylim(0, 1)

    agg["mean_ctrl_delay"].plot(kind="bar", ax=axes[1], color="#F58518")
    axes[1].set_ylabel("Mean control delay (ms)")
    axes[1].set_xlabel("Charging protocol")

    for ax in axes:
        ax.tick_params(axis="x", rotation=35)
    fig.suptitle("Network context at decision time")
    plt.tight_layout()
    plt.savefig(output_dir / "qos_context.png", dpi=150)
    plt.close()


def main() -> None:
    args = parse_args()
    output_dir: Path = args.output_dir
    output_dir.mkdir(parents=True, exist_ok=True)

    protocol_map = load_protocol_map(args.protocol_map)
    runs = collect_runs(args.log_root, args.run_ids)
    if not runs:
        raise SystemExit("No charge_events.csv files found.")

    combined = pd.concat([run.frame for run in runs], ignore_index=True)
    combined["protocol"] = assign_protocol(combined, protocol_map)

    plot_outcome_distribution(combined, output_dir)
    plot_latency_boxplots(combined, output_dir, args.min_samples)
    plot_energy_recovered(combined, output_dir, args.min_samples)
    plot_qos_context(combined, output_dir)

    summary_path = output_dir / "protocol_summary.csv"
    combined.groupby("protocol").agg(
        sessions=("request_msg_id", "count"),
        success_rate=("charge_completed", lambda x: x.astype(str).str.lower().isin(["true", "1"]).mean()),
        median_wait_ms=("waiting_time_ms", "median"),
        median_decision_ms=("decision_latency_ms", "median"),
        median_energy=("energy_recovered", "median"),
    ).to_csv(summary_path)

    print(f"Wrote charts to {output_dir}")


if __name__ == "__main__":
    main()
