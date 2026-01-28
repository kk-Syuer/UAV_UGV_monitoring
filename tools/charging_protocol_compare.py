#!/usr/bin/env python3
"""Generate comparison graphs for charging protocols from network monitor logs."""

from __future__ import annotations

import argparse
import csv
import json
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple

import matplotlib.pyplot as plt
import pandas as pd


@dataclass
class RunData:
    run_id: str
    path: Path
    protocol: str
    charge_frame: pd.DataFrame
    messages_frame: pd.DataFrame
    status_frame: pd.DataFrame


REQUIRED_COLUMNS = {
    "run_id",
    "request_msg_id",
    "uav_id",
    "ugv_id",
    "outcome",
    "failure_reason",
    "request_time",
    "decision_time",
    "dock_start_time",
    "decision_latency_ms",
    "waiting_time_ms",
    "charge_completed",
    "start_battery",
    "end_battery",
    "energy_recovered",
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


def read_metadata_protocol(run_path: Path) -> Optional[str]:
    candidates = [
        "metadata.json",
        "metadata.csv",
        "metadata.txt",
        "metadata.yaml",
        "metadata.yml",
    ]
    for name in candidates:
        meta_path = run_path / name
        if not meta_path.exists():
            continue
        if meta_path.suffix == ".json":
            try:
                with meta_path.open() as handle:
                    data = json.load(handle)
                for key in ("protocol", "decision_policy", "charging_protocol"):
                    value = data.get(key)
                    if value:
                        return str(value).strip()
            except (json.JSONDecodeError, OSError):
                continue
        elif meta_path.suffix == ".csv":
            try:
                with meta_path.open(newline="") as handle:
                    reader = csv.DictReader(handle)
                    for row in reader:
                        value = row.get("protocol") or row.get("decision_policy")
                        if value:
                            return str(value).strip()
                        break
            except OSError:
                continue
        else:
            try:
                for line in meta_path.read_text().splitlines():
                    if ":" not in line:
                        continue
                    key, value = line.split(":", 1)
                    key = key.strip()
                    if key in {"protocol", "decision_policy", "charging_protocol"}:
                        value = value.strip()
                        if value:
                            return value
            except OSError:
                continue
    return None


def read_charge_events(path: Path) -> pd.DataFrame:
    frame = pd.read_csv(path)
    for column in REQUIRED_COLUMNS:
        if column not in frame.columns:
            frame[column] = pd.NA
    return frame


def read_optional_csv(path: Path) -> pd.DataFrame:
    if not path.exists():
        return pd.DataFrame()
    return pd.read_csv(path)


def resolve_protocol(run_id: str, run_path: Path, protocol_map: Dict[str, str]) -> str:
    if run_id in protocol_map:
        return protocol_map[run_id]
    meta_protocol = read_metadata_protocol(run_path)
    if meta_protocol:
        return meta_protocol
    return run_id


def collect_runs(log_root: Path, run_ids: Optional[List[str]],
                 protocol_map: Dict[str, str]) -> List[RunData]:
    runs: List[RunData] = []
    for run_path in find_runs(log_root, run_ids):
        csv_path = run_path / "charge_events.csv"
        if not csv_path.exists():
            continue
        charge_frame = read_charge_events(csv_path)
        run_id = charge_frame["run_id"].iloc[0] if not charge_frame.empty else run_path.name
        protocol = resolve_protocol(run_id, run_path, protocol_map)

        messages_frame = read_optional_csv(run_path / "messages.csv")
        status_frame = read_optional_csv(run_path / "status_timeseries.csv")

        for frame in (charge_frame, messages_frame, status_frame):
            if not frame.empty and "run_id" not in frame.columns:
                frame["run_id"] = run_id
            if not frame.empty:
                frame["protocol"] = protocol

        runs.append(RunData(
            run_id=run_id,
            path=run_path,
            protocol=protocol,
            charge_frame=charge_frame,
            messages_frame=messages_frame,
            status_frame=status_frame,
        ))
    return runs


def filter_positive(series: pd.Series) -> pd.Series:
    numeric = pd.to_numeric(series, errors="coerce")
    return numeric[numeric >= 0]


def normalize_bool(series: pd.Series) -> pd.Series:
    return series.astype(str).str.lower().isin(["true", "1", "yes"])


def control_messages(frame: pd.DataFrame) -> pd.DataFrame:
    if frame.empty or "flow_type" not in frame.columns:
        return pd.DataFrame()
    flow = pd.to_numeric(frame["flow_type"], errors="coerce")
    return frame[flow == 1]


def bucket_time(series: pd.Series, bin_seconds: int) -> pd.Series:
    numeric = pd.to_numeric(series, errors="coerce")
    return (numeric // bin_seconds) * bin_seconds


def compute_delivery_rates(messages: pd.DataFrame, charge_frame: pd.DataFrame) -> Tuple[pd.Series, pd.Series]:
    if messages.empty:
        return pd.Series(dtype=float), pd.Series(dtype=float)
    control = control_messages(messages)
    if control.empty:
        return pd.Series(dtype=float), pd.Series(dtype=float)

    ugv_ids = set(charge_frame["ugv_id"].dropna().astype(str).unique())
    sink_mask = control["dst_id"].astype(str) == "sink_gateway"
    ugv_mask = control["dst_id"].astype(str).isin(ugv_ids) | control["dst_id"].astype(str).str.contains(
        "ugv", case=False, na=False)

    sink_rate = control[sink_mask].groupby("protocol").apply(lambda g: normalize_bool(g["delivered"]).mean())
    ugv_rate = control[ugv_mask].groupby("protocol").apply(lambda g: normalize_bool(g["delivered"]).mean())
    sink_rate = sink_rate.sort_index()
    ugv_rate = ugv_rate.sort_index()
    return sink_rate, ugv_rate


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
    if "decision_ctrl_pdr" not in frame.columns or "decision_ctrl_delay_mean_ms" not in frame.columns:
        return
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


def plot_control_delivery_rates(sink_rate: pd.Series, ugv_rate: pd.Series, output_dir: Path) -> None:
    if not sink_rate.empty:
        ax = sink_rate.plot(kind="bar", figsize=(8, 5), color="#4C78A8")
        ax.set_ylabel("Delivery rate")
        ax.set_xlabel("Charging protocol")
        ax.set_ylim(0, 1)
        ax.set_title("CONTROL delivery rate to sink gateway")
        ax.tick_params(axis="x", rotation=35)
        plt.tight_layout()
        plt.savefig(output_dir / "control_delivery_rate_to_sink.png", dpi=150)
        plt.close()

    if not ugv_rate.empty:
        ax = ugv_rate.plot(kind="bar", figsize=(8, 5), color="#F58518")
        ax.set_ylabel("Delivery rate")
        ax.set_xlabel("Charging protocol")
        ax.set_ylim(0, 1)
        ax.set_title("CONTROL delivery rate to UGV")
        ax.tick_params(axis="x", rotation=35)
        plt.tight_layout()
        plt.savefig(output_dir / "control_delivery_rate_to_ugv.png", dpi=150)
        plt.close()


def plot_alive_over_time(status_frame: pd.DataFrame, output_dir: Path, bin_seconds: int) -> pd.DataFrame:
    if status_frame.empty:
        return pd.DataFrame()

    frame = status_frame.copy()
    frame["time_bin"] = bucket_time(frame["time"], bin_seconds)
    frame["alive"] = pd.to_numeric(frame["battery_level"], errors="coerce") > 1e-3
    frame["is_ch"] = pd.to_numeric(frame["role"], errors="coerce") == 1
    frame["alive_members"] = frame["alive"] & ~frame["is_ch"]
    frame["alive_ch"] = frame["alive"] & frame["is_ch"]

    grouped = frame.groupby(["protocol", "time_bin"]).agg(
        alive_members=("alive_members", "sum"),
        alive_ch=("alive_ch", "sum"),
    ).reset_index()

    fig, axes = plt.subplots(2, 1, figsize=(10, 8), sharex=True)
    for protocol, group in grouped.groupby("protocol"):
        axes[0].plot(group["time_bin"], group["alive_members"], label=protocol)
        axes[1].plot(group["time_bin"], group["alive_ch"], label=protocol)

    axes[0].set_ylabel("Alive members")
    axes[1].set_ylabel("Alive CHs")
    axes[1].set_xlabel("Time (s)")
    axes[0].set_title("Alive UAVs over time")
    axes[0].legend(title="Protocol", bbox_to_anchor=(1.02, 1), loc="upper left")
    plt.tight_layout()
    plt.savefig(output_dir / "alive_uavs_over_time.png", dpi=150)
    plt.close()

    return grouped


def plot_battery_over_time(status_frame: pd.DataFrame, output_dir: Path, bin_seconds: int) -> None:
    if status_frame.empty:
        return

    frame = status_frame.copy()
    frame["time_bin"] = bucket_time(frame["time"], bin_seconds)
    frame["is_ch"] = pd.to_numeric(frame["role"], errors="coerce") == 1
    frame["battery_level"] = pd.to_numeric(frame["battery_level"], errors="coerce")

    members = frame[~frame["is_ch"]]
    chs = frame[frame["is_ch"]]

    if not members.empty:
        stats = members.groupby(["protocol", "time_bin"]).agg(
            mean_battery=("battery_level", "mean"),
            min_battery=("battery_level", "min"),
        ).reset_index()
        plt.figure(figsize=(10, 6))
        for protocol, group in stats.groupby("protocol"):
            plt.plot(group["time_bin"], group["mean_battery"], label=f"{protocol} mean")
            plt.plot(group["time_bin"], group["min_battery"], linestyle="--", label=f"{protocol} min")
        plt.ylabel("Battery level")
        plt.xlabel("Time (s)")
        plt.title("Member battery over time")
        plt.legend(bbox_to_anchor=(1.02, 1), loc="upper left")
        plt.tight_layout()
        plt.savefig(output_dir / "battery_over_time_members.png", dpi=150)
        plt.close()

    if not chs.empty:
        stats = chs.groupby(["protocol", "time_bin"]).agg(
            mean_battery=("battery_level", "mean"),
        ).reset_index()
        plt.figure(figsize=(10, 6))
        for protocol, group in stats.groupby("protocol"):
            plt.plot(group["time_bin"], group["mean_battery"], label=protocol)
        plt.ylabel("Battery level")
        plt.xlabel("Time (s)")
        plt.title("CH battery over time")
        plt.legend(bbox_to_anchor=(1.02, 1), loc="upper left")
        plt.tight_layout()
        plt.savefig(output_dir / "battery_over_time_ch.png", dpi=150)
        plt.close()


def plot_control_delay(messages: pd.DataFrame, output_dir: Path, min_samples: int) -> None:
    control = control_messages(messages)
    if control.empty or "e2e_delay_ms" not in control.columns:
        return
    fig, ax = plt.subplots(figsize=(8, 5))
    data = []
    labels = []
    for protocol, group in control.groupby("protocol"):
        values = filter_positive(group["e2e_delay_ms"])
        if len(values) < min_samples:
            continue
        data.append(values)
        labels.append(protocol)
    if data:
        ax.boxplot(data, labels=labels, showfliers=False)
        ax.set_ylabel("E2E delay (ms)")
        ax.set_xlabel("Charging protocol")
        ax.set_title("CONTROL message delay")
        ax.tick_params(axis="x", rotation=35)
        plt.tight_layout()
        plt.savefig(output_dir / "control_delay_boxplot.png", dpi=150)
    plt.close()


def plot_control_drop_rate(messages: pd.DataFrame, output_dir: Path) -> None:
    control = control_messages(messages)
    if control.empty or "dropped" not in control.columns:
        return
    rates = control.groupby("protocol").apply(lambda g: normalize_bool(g["dropped"]).mean())
    ax = rates.plot(kind="bar", figsize=(8, 5), color="#E45756")
    ax.set_ylabel("Drop rate")
    ax.set_xlabel("Charging protocol")
    ax.set_ylim(0, 1)
    ax.set_title("CONTROL drop rate")
    ax.tick_params(axis="x", rotation=35)
    plt.tight_layout()
    plt.savefig(output_dir / "control_drop_rate.png", dpi=150)
    plt.close()


def plot_control_hopcount(messages: pd.DataFrame, output_dir: Path) -> None:
    control = control_messages(messages)
    if control.empty or "hop_count" not in control.columns:
        return
    control["hop_count"] = pd.to_numeric(control["hop_count"], errors="coerce")
    max_hop = int(control["hop_count"].max()) if control["hop_count"].notna().any() else 0
    bins = list(range(0, max_hop + 2))
    plt.figure(figsize=(10, 6))
    for protocol, group in control.groupby("protocol"):
        values = group["hop_count"].dropna()
        if values.empty:
            continue
        plt.hist(values, bins=bins, alpha=0.5, label=protocol)
    plt.xlabel("Hop count")
    plt.ylabel("Message count")
    plt.title("CONTROL hop count distribution")
    plt.legend(bbox_to_anchor=(1.02, 1), loc="upper left")
    plt.tight_layout()
    plt.savefig(output_dir / "control_hopcount.png", dpi=150)
    plt.close()


def main() -> None:
    args = parse_args()
    output_dir: Path = args.output_dir
    output_dir.mkdir(parents=True, exist_ok=True)

    protocol_map = load_protocol_map(args.protocol_map)
    runs = collect_runs(args.log_root, args.run_ids, protocol_map)
    if not runs:
        raise SystemExit("No charge_events.csv files found.")

    combined_charge = pd.concat([run.charge_frame for run in runs], ignore_index=True)
    combined_messages = pd.concat([run.messages_frame for run in runs], ignore_index=True)
    combined_status = pd.concat([run.status_frame for run in runs], ignore_index=True)

    plot_outcome_distribution(combined_charge, output_dir)
    plot_latency_boxplots(combined_charge, output_dir, args.min_samples)
    plot_energy_recovered(combined_charge, output_dir, args.min_samples)
    plot_qos_context(combined_charge, output_dir)

    sink_rate, ugv_rate = compute_delivery_rates(combined_messages, combined_charge)
    plot_control_delivery_rates(sink_rate, ugv_rate, output_dir)

    alive_grouped = plot_alive_over_time(combined_status, output_dir, bin_seconds=5)
    plot_battery_over_time(combined_status, output_dir, bin_seconds=5)

    plot_control_delay(combined_messages, output_dir, args.min_samples)
    plot_control_drop_rate(combined_messages, output_dir)
    plot_control_hopcount(combined_messages, output_dir)

    summary_path = output_dir / "protocol_summary.csv"
    summary = combined_charge.groupby("protocol").agg(
        sessions=("request_msg_id", "count"),
        success_rate=("charge_completed", lambda x: x.astype(str).str.lower().isin(["true", "1"]).mean()),
        median_wait_ms=("waiting_time_ms", "median"),
        median_decision_ms=("decision_latency_ms", "median"),
        median_energy_recovered=("energy_recovered", "median"),
    )

    if not sink_rate.empty:
        summary = summary.join(sink_rate.rename("control_delivery_rate_to_sink"), how="left")
    if not ugv_rate.empty:
        summary = summary.join(ugv_rate.rename("control_delivery_rate_to_ugv"), how="left")

    if not alive_grouped.empty:
        avg_alive = alive_grouped.groupby("protocol").agg(
            avg_alive_members=("alive_members", "mean"),
        )
        summary = summary.join(avg_alive, how="left")

    summary.to_csv(summary_path)

    print(f"Wrote charts to {output_dir}")


if __name__ == "__main__":
    main()
