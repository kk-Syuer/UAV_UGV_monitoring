#!/usr/bin/env python3
"""Clean and enrich experiment_data_collection/test_round1 datasets.

This script is designed to be run locally where pandas/numpy are available.
It reads each policy folder under test_round1, applies data-quality rules,
and writes cleaned CSV/JSON artifacts to test_round1_clean.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Dict, Iterable, List, Optional

import numpy as np
import pandas as pd

NA_VALUES = [-1, "-1", "null", "", "None", "none", "nan", "NaN"]

CSV_DTYPES: Dict[str, Dict[str, str]] = {
    "messages.csv": {
        "run_id": "string",
        "msg_id": "Int64",
        "src_id": "Int64",
        "dst_id": "Int64",
        "delivered": "string",
        "dropped": "string",
    },
    "charge_events.csv": {
        "run_id": "string",
        "request_msg_id": "Int64",
        "uav_id": "Int64",
        "ugv_id": "Int64",
    },
    "status_timeseries.csv": {
        "run_id": "string",
        "uav_id": "Int64",
        "backbone_active": "string",
        "role": "string",
    },
    "death_events.csv": {
        "run_id": "string",
        "uav_id": "Int64",
        "role": "string",
    },
    "charge_queue_timeseries.csv": {
        "run_id": "string",
    },
    "network_timeseries.csv": {
        "run_id": "string",
    },
    "weather_timeseries.csv": {
        "run_id": "string",
    },
    "recovery_events.csv": {
        "run_id": "string",
    },
    "qos_metrics.csv": {
        "run_id": "string",
    },
}

BOOL_TRUE = {"1", "true", "t", "yes", "y"}
BOOL_FALSE = {"0", "false", "f", "no", "n"}
ROLE_MAP = {
    "0": "UAV",
    "1": "Cluster Head",
    "uav": "UAV",
    "member": "UAV",
    "ch": "Cluster Head",
    "cluster_head": "Cluster Head",
    "cluster head": "Cluster Head",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Clean test_round1 experiment data")
    parser.add_argument(
        "--input-root",
        type=Path,
        default=Path("experiment_data_collection/test_round1"),
        help="Folder containing policy subfolders",
    )
    parser.add_argument(
        "--output-root",
        type=Path,
        default=Path("experiment_data_collection/test_round1_clean"),
        help="Folder to write cleaned outputs",
    )
    parser.add_argument(
        "--timezone",
        default="Europe/Rome",
        help="Timezone used when converting relative seconds to timestamps",
    )
    parser.add_argument(
        "--mission-start",
        default=None,
        help="Optional mission start timestamp (ISO-8601), used for absolute datetimes",
    )
    return parser.parse_args()


def ensure_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def read_csv(path: Path) -> pd.DataFrame:
    dtype_map = CSV_DTYPES.get(path.name, None)
    return pd.read_csv(path, dtype=dtype_map, na_values=NA_VALUES)


def maybe_normalize_bool(series: pd.Series) -> pd.Series:
    lowered = series.astype(str).str.strip().str.lower()
    out = pd.Series(pd.NA, index=series.index, dtype="boolean")
    out[lowered.isin(BOOL_TRUE)] = True
    out[lowered.isin(BOOL_FALSE)] = False
    return out


def normalize_role(series: pd.Series) -> pd.Series:
    lowered = series.astype(str).str.strip().str.lower()
    return lowered.map(ROLE_MAP).fillna(series)


def add_policy_column(frame: pd.DataFrame, policy: str) -> pd.DataFrame:
    if frame.empty:
        return frame
    frame["policy"] = policy
    return frame


def add_datetime_columns(
    frame: pd.DataFrame,
    mission_start: Optional[pd.Timestamp],
    timezone: str,
) -> pd.DataFrame:
    if frame.empty:
        return frame
    for col in frame.columns:
        if not (col == "time" or col.endswith("_time") or col.endswith("_time_s") or col.endswith("_time_sec")):
            continue
        numeric = pd.to_numeric(frame[col], errors="coerce")
        frame[col] = numeric
        if mission_start is not None:
            frame[f"{col}_dt"] = mission_start + pd.to_timedelta(numeric, unit="s")
            frame[f"{col}_dt"] = frame[f"{col}_dt"].dt.tz_convert(timezone)
    return frame


def drop_non_monotonic(frame: pd.DataFrame, time_col: str = "time") -> pd.DataFrame:
    if frame.empty or time_col not in frame.columns:
        return frame
    frame = frame.sort_values(time_col).reset_index(drop=True)
    diffs = pd.to_numeric(frame[time_col], errors="coerce").diff()
    return frame[(diffs.isna()) | (diffs >= 0)].reset_index(drop=True)


def winsorize(series: pd.Series, low_q: float = 0.01, high_q: float = 0.99) -> pd.Series:
    numeric = pd.to_numeric(series, errors="coerce")
    if numeric.notna().sum() < 5:
        return numeric
    lo = numeric.quantile(low_q)
    hi = numeric.quantile(high_q)
    return numeric.clip(lower=lo, upper=hi)


def clip_ranges(frame: pd.DataFrame) -> pd.DataFrame:
    if frame.empty:
        return frame
    if "battery_level" in frame.columns:
        battery = pd.to_numeric(frame["battery_level"], errors="coerce")
        frame["battery_level"] = battery.where((battery >= 0) & (battery <= 100), np.nan)
        frame["battery_fraction"] = frame["battery_level"] / 100.0
    for col in frame.columns:
        if "queue_length" in col:
            series = pd.to_numeric(frame[col], errors="coerce")
            frame[col] = series.where(series >= 0, np.nan)
    for col in ["pdr", "window_pdr", "ctrl_charge_req_pdr", "ctrl_charge_dec_pdr"]:
        if col in frame.columns:
            series = pd.to_numeric(frame[col], errors="coerce")
            frame[col] = series.where((series >= 0) & (series <= 1), np.nan)
    for col in frame.columns:
        if "delay" in col:
            series = pd.to_numeric(frame[col], errors="coerce")
            frame[col] = series.where(series >= 0, np.nan)
    return frame


def clean_messages(frame: pd.DataFrame) -> pd.DataFrame:
    if frame.empty:
        return frame
    for col in ["creation_time_s", "delivered_time_s", "e2e_delay_ms"]:
        if col in frame.columns:
            frame[col] = pd.to_numeric(frame[col], errors="coerce")
    if "e2e_delay_ms" not in frame.columns and {"creation_time_s", "delivered_time_s"}.issubset(frame.columns):
        frame["e2e_delay_ms"] = (frame["delivered_time_s"] - frame["creation_time_s"]) * 1000.0
    frame["negative_delay_flag"] = pd.to_numeric(frame.get("e2e_delay_ms"), errors="coerce") < 0

    if {"run_id", "msg_id"}.issubset(frame.columns):
        frame = frame.sort_values(["run_id", "msg_id", "delivered_time_s"], na_position="last")
        frame = frame.drop_duplicates(subset=["run_id", "msg_id"], keep="first")

    frame = clip_ranges(frame)
    return frame.reset_index(drop=True)


def clean_charge_events(frame: pd.DataFrame) -> pd.DataFrame:
    if frame.empty:
        return frame
    for col in [
        "request_time",
        "decision_time",
        "dock_start_time",
        "charge_end_time",
        "terminal_time",
        "waiting_time_ms",
        "decision_latency_ms",
        "charge_duration_ms",
    ]:
        if col in frame.columns:
            frame[col] = pd.to_numeric(frame[col], errors="coerce")

    if {"run_id", "request_msg_id"}.issubset(frame.columns):
        frame = frame.sort_values(["run_id", "request_msg_id", "terminal_time"], na_position="last")
        frame = frame.drop_duplicates(subset=["run_id", "request_msg_id"], keep="last")

    if "waiting_time_ms" in frame.columns:
        frame["waiting_time_ms"] = winsorize(frame["waiting_time_ms"])
        frame["waiting_time_s"] = frame["waiting_time_ms"] / 1000.0
    if "decision_latency_ms" in frame.columns:
        frame["decision_latency_ms"] = winsorize(frame["decision_latency_ms"])
        frame["decision_latency_s"] = frame["decision_latency_ms"] / 1000.0
    if "charge_duration_ms" in frame.columns:
        frame["charge_duration_s"] = pd.to_numeric(frame["charge_duration_ms"], errors="coerce") / 1000.0

    if {"dock_start_time", "request_time"}.issubset(frame.columns):
        frame["waiting_time_calc_s"] = (frame["dock_start_time"] - frame["request_time"]).where(
            (frame["dock_start_time"] >= frame["request_time"]), np.nan
        )

    frame = clip_ranges(frame)
    return frame.reset_index(drop=True)


def clean_death_events(frame: pd.DataFrame) -> pd.DataFrame:
    if frame.empty:
        return frame
    if {"time", "uav_id"}.issubset(frame.columns):
        frame["time"] = pd.to_numeric(frame["time"], errors="coerce")
        frame = frame.sort_values(["time", "uav_id"]).drop_duplicates(["time", "uav_id"], keep="first")
    frame = clip_ranges(frame)
    return frame.reset_index(drop=True)


def clean_status(frame: pd.DataFrame) -> pd.DataFrame:
    if frame.empty:
        return frame
    if "time" in frame.columns:
        frame["time"] = pd.to_numeric(frame["time"], errors="coerce")
    if "backbone_active" in frame.columns:
        frame["backbone_active"] = maybe_normalize_bool(frame["backbone_active"])
    if "role" in frame.columns:
        frame["role"] = normalize_role(frame["role"])
    if "energy_consumption_rate" in frame.columns:
        ecr = pd.to_numeric(frame["energy_consumption_rate"], errors="coerce")
        frame["energy_consumption_rate"] = ecr.where(ecr > 0, np.nan)
    frame = drop_non_monotonic(frame, "time")
    frame = clip_ranges(frame)
    return frame


def queue_backlog_metrics(queue_df: pd.DataFrame) -> pd.DataFrame:
    if queue_df.empty or "time" not in queue_df.columns:
        return pd.DataFrame()
    frame = queue_df.copy()
    frame["time"] = pd.to_numeric(frame["time"], errors="coerce")
    frame = frame.sort_values("time").dropna(subset=["time"])
    queue_col = "queue_length_ch" if "queue_length_ch" in frame.columns else None
    if queue_col is None:
        return pd.DataFrame()
    frame[queue_col] = pd.to_numeric(frame[queue_col], errors="coerce").fillna(0)
    dt = frame["time"].diff().fillna(0)
    integral = float((frame[queue_col] * dt).sum())
    return pd.DataFrame(
        {
            "queue_backlog_integral": [integral],
            "queue_len_mean": [frame[queue_col].mean()],
            "queue_len_p95": [frame[queue_col].quantile(0.95)],
        }
    )


def compute_uav_lifetime(status_df: pd.DataFrame, death_df: pd.DataFrame) -> pd.DataFrame:
    if status_df.empty or "uav_id" not in status_df.columns or "time" not in status_df.columns:
        return pd.DataFrame()
    s = status_df.copy()
    s["time"] = pd.to_numeric(s["time"], errors="coerce")
    spawn = s.groupby(["run_id", "uav_id"], dropna=False)["time"].min().rename("spawn_time")
    end = s.groupby(["run_id", "uav_id"], dropna=False)["time"].max().rename("mission_end_time")

    life = pd.concat([spawn, end], axis=1).reset_index()
    if not death_df.empty and {"run_id", "uav_id", "time"}.issubset(death_df.columns):
        d = death_df.copy()
        d["time"] = pd.to_numeric(d["time"], errors="coerce")
        death = d.groupby(["run_id", "uav_id"], dropna=False)["time"].min().rename("death_time")
        life = life.merge(death.reset_index(), on=["run_id", "uav_id"], how="left")
    else:
        life["death_time"] = np.nan

    life["effective_end_time"] = life["death_time"].fillna(life["mission_end_time"])
    life["uav_lifetime_s"] = life["effective_end_time"] - life["spawn_time"]
    return life


def compute_ch_downtime_ratio(status_df: pd.DataFrame) -> pd.DataFrame:
    if status_df.empty or "time" not in status_df.columns:
        return pd.DataFrame()
    ch = status_df.copy()
    if "role" in ch.columns:
        ch = ch[ch["role"].astype(str).str.lower().str.contains("cluster head")]
    if ch.empty or "backbone_active" not in ch.columns:
        return pd.DataFrame()

    ch = ch.sort_values(["run_id", "uav_id", "time"])
    ch["backbone_active"] = ch["backbone_active"].fillna(False)
    ch["dt"] = ch.groupby(["run_id", "uav_id"], dropna=False)["time"].diff().fillna(0)
    ch["downtime_dt"] = np.where(ch["backbone_active"], 0, ch["dt"])

    out = ch.groupby(["run_id", "uav_id"], dropna=False).agg(
        mission_duration_s=("dt", "sum"),
        downtime_s=("downtime_dt", "sum"),
    ).reset_index()
    out["ch_downtime_ratio"] = out["downtime_s"] / out["mission_duration_s"].replace(0, np.nan)
    return out


def compute_recovery_latency(recovery_df: pd.DataFrame, status_df: pd.DataFrame) -> pd.DataFrame:
    if recovery_df.empty or status_df.empty:
        return pd.DataFrame()
    required = {"run_id", "creation_time"}
    if not required.issubset(recovery_df.columns):
        return pd.DataFrame()

    rec = recovery_df.copy()
    rec["creation_time"] = pd.to_numeric(rec["creation_time"], errors="coerce")
    if "control_type" in rec.columns:
        rec = rec[rec["control_type"].astype(str).str.contains("EMERGENCY_RETURN_TRIGGERED", na=False)]
    if rec.empty:
        return pd.DataFrame()

    st = status_df.copy()
    st["time"] = pd.to_numeric(st["time"], errors="coerce")
    if "role" in st.columns:
        st = st[st["role"].astype(str).str.lower().str.contains("cluster head")]
    if "backbone_active" in st.columns:
        st = st[st["backbone_active"] == True]  # noqa: E712

    rows: List[Dict[str, object]] = []
    for _, event in rec.iterrows():
        run_id = event.get("run_id")
        t0 = event.get("creation_time")
        if pd.isna(run_id) or pd.isna(t0):
            continue
        candidate = st[(st["run_id"] == run_id) & (st["time"] >= t0)]
        replacement_time = candidate["time"].min() if not candidate.empty else np.nan
        rows.append(
            {
                "run_id": run_id,
                "trigger_time": t0,
                "replacement_time": replacement_time,
                "recovery_latency_s": replacement_time - t0 if pd.notna(replacement_time) else np.nan,
            }
        )
    return pd.DataFrame(rows)


def align_timeseries(
    status_df: pd.DataFrame,
    queue_df: pd.DataFrame,
    network_df: pd.DataFrame,
    weather_df: pd.DataFrame,
) -> pd.DataFrame:
    if status_df.empty or "time" not in status_df.columns:
        return pd.DataFrame()

    base = status_df.sort_values(["run_id", "time"]).copy()

    def merge_optional(left: pd.DataFrame, right: pd.DataFrame, suffix: str) -> pd.DataFrame:
        if right.empty or "time" not in right.columns:
            return left
        right = right.sort_values(["run_id", "time"]).copy()
        keep_cols = [c for c in right.columns if c not in {"policy"}]
        right = right[keep_cols]
        return pd.merge_asof(
            left,
            right,
            on="time",
            by="run_id",
            direction="nearest",
            suffixes=("", suffix),
        )

    aligned = merge_optional(base, queue_df, "_queue")
    aligned = merge_optional(aligned, network_df, "_net")
    aligned = merge_optional(aligned, weather_df, "_weather")
    return aligned


def assign_to_nearest_window(events_df: pd.DataFrame, aligned_df: pd.DataFrame, time_col: str) -> pd.DataFrame:
    if events_df.empty or aligned_df.empty or time_col not in events_df.columns:
        return pd.DataFrame()
    events = events_df.copy()
    events[time_col] = pd.to_numeric(events[time_col], errors="coerce")
    events = events.sort_values(["run_id", time_col])

    anchor = aligned_df[["run_id", "time"]].dropna().drop_duplicates().sort_values(["run_id", "time"])
    mapped = pd.merge_asof(
        events,
        anchor,
        left_on=time_col,
        right_on="time",
        by="run_id",
        direction="nearest",
    )
    mapped = mapped.rename(columns={"time": "aligned_time"})
    return mapped


def flatten_summary(summary_path: Path, policy: str) -> pd.DataFrame:
    with summary_path.open() as handle:
        data = json.load(handle)
    frame = pd.json_normalize(data, sep=".")
    frame["policy"] = policy
    return frame


def write_frame(path: Path, frame: pd.DataFrame) -> None:
    if frame.empty:
        return
    frame.to_csv(path, index=False)


def clean_policy_folder(
    policy_dir: Path,
    out_dir: Path,
    mission_start: Optional[pd.Timestamp],
    timezone: str,
) -> Dict[str, pd.DataFrame]:
    ensure_dir(out_dir)
    policy = policy_dir.name

    loaded: Dict[str, pd.DataFrame] = {}
    for csv_path in sorted(policy_dir.glob("*.csv")):
        frame = read_csv(csv_path)
        frame = add_policy_column(frame, policy)
        frame = add_datetime_columns(frame, mission_start, timezone)
        loaded[csv_path.name] = frame

    if "messages.csv" in loaded:
        loaded["messages.csv"] = clean_messages(loaded["messages.csv"])
    if "charge_events.csv" in loaded:
        loaded["charge_events.csv"] = clean_charge_events(loaded["charge_events.csv"])
    if "death_events.csv" in loaded:
        loaded["death_events.csv"] = clean_death_events(loaded["death_events.csv"])
    if "status_timeseries.csv" in loaded:
        loaded["status_timeseries.csv"] = clean_status(loaded["status_timeseries.csv"])

    for key in ["charge_queue_timeseries.csv", "network_timeseries.csv", "weather_timeseries.csv"]:
        if key in loaded:
            loaded[key] = drop_non_monotonic(clip_ranges(loaded[key]), "time")

    for name, frame in loaded.items():
        write_frame(out_dir / name, frame)

    summary_path = policy_dir / "summary.json"
    if summary_path.exists():
        summary_df = flatten_summary(summary_path, policy)
        write_frame(out_dir / "summary_flat.csv", summary_df)

    status_df = loaded.get("status_timeseries.csv", pd.DataFrame())
    death_df = loaded.get("death_events.csv", pd.DataFrame())
    queue_df = loaded.get("charge_queue_timeseries.csv", pd.DataFrame())
    network_df = loaded.get("network_timeseries.csv", pd.DataFrame())
    weather_df = loaded.get("weather_timeseries.csv", pd.DataFrame())
    recovery_df = loaded.get("recovery_events.csv", pd.DataFrame())
    messages_df = loaded.get("messages.csv", pd.DataFrame())
    charge_df = loaded.get("charge_events.csv", pd.DataFrame())

    write_frame(out_dir / "uav_lifetime.csv", compute_uav_lifetime(status_df, death_df))
    write_frame(out_dir / "ch_downtime_ratio.csv", compute_ch_downtime_ratio(status_df))
    write_frame(out_dir / "queue_backlog_metrics.csv", queue_backlog_metrics(queue_df))
    write_frame(out_dir / "recovery_latency.csv", compute_recovery_latency(recovery_df, status_df))

    aligned = align_timeseries(status_df, queue_df, network_df, weather_df)
    write_frame(out_dir / "aligned_timeseries.csv", aligned)
    write_frame(out_dir / "messages_aligned.csv", assign_to_nearest_window(messages_df, aligned, "creation_time_s"))
    write_frame(out_dir / "charge_events_aligned.csv", assign_to_nearest_window(charge_df, aligned, "request_time"))

    if not network_df.empty:
        net_summary = network_df.groupby("run_id", dropna=False).agg(
            overall_pdr=("window_pdr", "mean"),
            mean_delay_ms=("window_delay_mean_ms", "mean"),
            p95_delay_ms=("window_delay_p95_ms", "mean"),
            mean_jitter_ms=("window_jitter_ms", "mean"),
        ).reset_index()
        write_frame(out_dir / "network_summary.csv", net_summary)

    if "qos_metrics.csv" in loaded:
        qos = loaded["qos_metrics.csv"].copy()
        qos = clip_ranges(qos)
        write_frame(out_dir / "qos_metrics_clean.csv", qos)

    return loaded


def collect_policy_dirs(input_root: Path) -> Iterable[Path]:
    for path in sorted(input_root.iterdir()):
        if path.is_dir():
            yield path


def main() -> None:
    args = parse_args()
    ensure_dir(args.output_root)

    mission_start = None
    if args.mission_start:
        mission_start = pd.Timestamp(args.mission_start)
        if mission_start.tz is None:
            mission_start = mission_start.tz_localize(args.timezone)

    summary_frames: List[pd.DataFrame] = []
    for policy_dir in collect_policy_dirs(args.input_root):
        out_dir = args.output_root / policy_dir.name
        clean_policy_folder(policy_dir, out_dir, mission_start, args.timezone)
        flat_summary = out_dir / "summary_flat.csv"
        if flat_summary.exists():
            summary_frames.append(pd.read_csv(flat_summary))

    if summary_frames:
        pd.concat(summary_frames, ignore_index=True).to_csv(args.output_root / "all_policies_summary.csv", index=False)

    print(f"Cleaning complete. Output written to: {args.output_root}")


if __name__ == "__main__":
    main()
