#!/usr/bin/env python3
"""
run_all_plots_test_round2.py
-----------------------------
End-to-end plotting pipeline for UAV/UGV experiment results.

Produces figures across 8 thematic groups, derived CSV tables, a
protocol KPI summary table, and a missing-data report.

Group 08 (cross-layer analysis) is run automatically unless
``--skip-cross-layer`` is passed.  It requires the four add-on scripts:
  build_cross_layer_windows.py, cross_layer_lagged_correlation.py,
  plot_cross_layer_overlays.py, compute_charging_fairness.py.
Use ``--max-lag-min`` (default 10) to control the lag window.

Usage
-----
    python scripts/run_all_plots_test_round2.py \\
        [--input  experiment_data_collection/test_round2] \\
        [--output analysis/test_round2] \\
        [--bin-seconds 60]

Run folder naming convention
-----------------------------
    <protocol_name>_<replicate_id>   e.g.  ugv_dynamic_1
    If no trailing numeric suffix is present, replicate_id defaults to 1.

Old-schema compatibility
------------------------
    Columns named ``time`` are transparently renamed to ``time_s``; ``t_rel_s``
    is computed as ``time_s - min(time_s)`` when absent.  New event tables
    (packet_generated_events.csv etc.) are listed as missing but do not abort
    execution.
"""

import argparse
import json
import subprocess
import sys
import warnings
from pathlib import Path

import matplotlib
matplotlib.use("Agg")  # non-interactive backend — must be set before pyplot
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

# Ensure scripts/ directory is on the import path when run directly.
sys.path.insert(0, str(Path(__file__).parent))

from plotting_utils import (
    discover_runs,
    load_csv_if_exists,
    ensure_t_rel,
    savefig,
    ecdf,
    merge_asof_weather,
    protocol_color_map,
    label_bars,
    deduplicate_legend,
    boxplot_multi,
)

# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _parse_args():
    p = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    p.add_argument(
        "--input",
        default="experiment_data_collection/test_round2",
        help="Root directory that contains one sub-folder per run "
             "(default: experiment_data_collection/test_round2)",
    )
    p.add_argument(
        "--output",
        default="analysis/test_round2",
        help="Root output directory for figures, derived tables, and reports "
             "(default: analysis/test_round2)",
    )
    p.add_argument(
        "--bin-seconds",
        type=float,
        default=60.0,
        help="Time-bin width in seconds used for resampled timeseries plots "
             "(default: 60)",
    )
    p.add_argument(
        "--skip-cross-layer",
        action="store_true",
        default=False,
        help="Skip Group 08 cross-layer analysis (window tables, lagged "
             "correlation, event overlays, charging fairness).",
    )
    p.add_argument(
        "--max-lag-min",
        type=float,
        default=10.0,
        help="Maximum lag in minutes for cross-layer lagged correlation "
             "(default: 10)",
    )
    return p.parse_args()


# ---------------------------------------------------------------------------
# Per-run data loader
# ---------------------------------------------------------------------------

_FILE_MAP: dict = {
    "messages":                "messages.csv",
    "charge_events":           "charge_events.csv",
    "status_timeseries":       "status_timeseries.csv",
    "qos_metrics":             "qos_metrics.csv",
    "network_timeseries":      "network_timeseries.csv",
    "charge_queue_timeseries": "charge_queue_timeseries.csv",
    "weather_timeseries":      "weather_timeseries.csv",
    "recovery_events":         "recovery_events.csv",
    "death_events":            "death_events.csv",
    "preemption_events":       "preemption_events.csv",
    "packet_generated_events": "packet_generated_events.csv",
    "packet_delivered_events": "packet_delivered_events.csv",
    "charge_request_events":   "charge_request_events.csv",
    "charge_decision_events":  "charge_decision_events.csv",
    "charge_session_events":   "charge_session_events.csv",
}


def _load(run: dict, key: str, missing: list) -> pd.DataFrame | None:
    """Load *key* CSV for *run*, compute ``t_rel_s`` if absent."""
    filename = _FILE_MAP.get(key)
    if filename is None:
        return None
    path = run["folder"] / filename
    df = load_csv_if_exists(path, missing_report=missing)
    if df is not None:
        ensure_t_rel(df)
    return df


def _load_summary(run: dict) -> dict | None:
    """
    Load ``summary.json`` for *run*.  Falls back to the last *valid* line of
    ``summary_snapshots.jsonl`` (new schema, append-only JSONL).

    Handles truncated/empty trailing lines from unclean node shutdowns by
    scanning JSONL lines in reverse and returning the first that parses
    successfully.  Returns ``None`` if neither file is present or parseable.
    """
    path = run["folder"] / "summary.json"
    if path.exists() and path.stat().st_size > 0:
        try:
            with open(path) as fh:
                return json.load(fh)
        except json.JSONDecodeError as exc:
            warnings.warn(f"Malformed {path}: {exc} — skipping")

    jsonl_path = run["folder"] / "summary_snapshots.jsonl"
    if jsonl_path.exists() and jsonl_path.stat().st_size > 0:
        # The C++ writer emits pretty-printed (multi-line) JSON objects, so we
        # cannot split by newline.  Use raw_decode() to extract complete objects
        # from the concatenated file content and return the last valid one.
        content = jsonl_path.read_text()
        decoder = json.JSONDecoder()
        last_obj = None
        idx = 0
        while idx < len(content):
            # Skip inter-object whitespace / blank lines.
            while idx < len(content) and content[idx] in " \t\n\r":
                idx += 1
            if idx >= len(content):
                break
            try:
                obj, idx = decoder.raw_decode(content, idx)
                last_obj = obj
            except json.JSONDecodeError:
                # Skip to the next newline and retry.
                nl = content.find("\n", idx)
                if nl == -1:
                    break
                idx = nl + 1
        if last_obj is not None:
            return last_obj
        warnings.warn(f"No valid JSON objects found in {jsonl_path}")

    return None


# ---------------------------------------------------------------------------
# Private plot helpers
# ---------------------------------------------------------------------------

def _overall_pdr_from_csvs(run: dict, missing: list) -> float:
    df = _load(run, "qos_metrics", missing)
    if df is None or "generated" not in df.columns or "delivered" not in df.columns:
        return float("nan")
    total_gen = df["generated"].sum()
    total_del = df["delivered"].sum()
    return total_del / total_gen if total_gen > 0 else float("nan")


def _success_rate_from_csvs(run: dict, missing: list) -> float:
    df = _load(run, "charge_events", missing)
    if df is None or "outcome" not in df.columns:
        return float("nan")
    total = len(df)
    return int((df["outcome"] == "STARTED").sum()) / total if total > 0 else float("nan")


def _mean_latency_from_csvs(run: dict, missing: list) -> float:
    df = _load(run, "charge_events", missing)
    if df is None or "decision_latency_ms" not in df.columns:
        return float("nan")
    return float(df["decision_latency_ms"].replace(-1, float("nan")).mean())


def _energy_recovered_series(run: dict, missing: list) -> "np.ndarray | None":
    """Return per-session energy-recovered values in **Wh** for completed charge sessions.

    Source priority (highest accuracy first):
    1. charge_session_events.csv DOCK_END — ``energy_charged_wh`` column
       (already computed as (end−start) × battery_capacity_wh / 100; accounts
       for CH vs member capacity differences).
    2. charge_session_events.csv DOCK_END — ``(battery_after − battery_before)
       × battery_capacity_wh / 100`` when ``energy_charged_wh`` is all -1 but
       capacity is known.
    3. charge_events.csv — ``energy_recovered_pct`` (percentage points, last
       resort when charge_session_events is absent).

    Returns None only when no file is present; returns an empty array when
    files exist but no completed sessions have valid data.
    """
    # --- preferred: charge_session_events DOCK_END (Wh, capacity-aware) ---
    _probe: list = []
    df2 = _load(run, "charge_session_events", _probe)
    if df2 is not None and "event_type" in df2.columns:
        ended = df2[df2["event_type"] == "DOCK_END"].copy()

        # Path 1: direct energy_charged_wh column
        if "energy_charged_wh" in ended.columns:
            vals = ended["energy_charged_wh"].replace(-1, float("nan")).dropna().values
            if len(vals) > 0:
                return vals

        # Path 2: compute from battery delta × capacity
        if ("battery_before" in ended.columns and "battery_after" in ended.columns
                and "battery_capacity_wh" in ended.columns):
            cap = ended["battery_capacity_wh"].replace(0, float("nan"))
            delta = (ended["battery_after"] - ended["battery_before"]).replace(-1, float("nan"))
            wh = (delta * cap / 100.0).dropna()
            wh = wh[wh >= 0]
            if len(wh) > 0:
                return wh.values

    # --- fallback: charge_events.csv percentage points (old schema) ---
    df = _load(run, "charge_events", _probe)
    if df is not None:
        col = next((c for c in ("energy_recovered_pct", "energy_recovered") if c in df.columns), None)
        if col is not None:
            if "outcome" in df.columns:
                df = df[df["outcome"].isin(["STARTED", "ENERGY_DEPLETED"])].copy()
            vals = df[col].replace(-1, float("nan")).dropna().values
            if len(vals) > 0:
                return vals

    # No usable data found.
    missing.extend(_probe)
    return None


def _mean_energy_from_csvs(run: dict, missing: list) -> float:
    vals = _energy_recovered_series(run, missing)
    if vals is None or len(vals) == 0:
        return float("nan")
    return float(np.nanmean(vals))


# ---------------------------------------------------------------------------
# Merged-replicate helpers
# ---------------------------------------------------------------------------

def _group_by_protocol(runs: list) -> dict:
    """Return OrderedDict mapping protocol_name → list[run], preserving discovery order."""
    groups: dict = {}
    for run in runs:
        groups.setdefault(run["protocol"], []).append(run)
    return groups


def _ts_to_grid(t_arr, y_arr, t_grid):
    """Interpolate (t_arr, y_arr) onto *t_grid*; returns NaN outside the data range."""
    mask = ~np.isnan(y_arr)
    if mask.sum() < 2:
        return np.full(len(t_grid), float("nan"))
    return np.interp(t_grid, t_arr[mask], y_arr[mask],
                     left=float("nan"), right=float("nan"))


def _merged_ts(run_list, load_key, ycol, missing, bin_sec):
    """
    Resample *ycol* from each run in *run_list* to a common time grid of width
    *bin_sec*, then return *(t_grid, mean_array, std_array)*.
    Returns *(None, None, None)* if no data are found.
    """
    series = []
    t_max = 0.0
    for run in run_list:
        df = _load(run, load_key, missing)
        if df is None or ycol not in df.columns or "t_rel_s" not in df.columns:
            continue
        df = df[df[ycol] >= 0].dropna(subset=[ycol, "t_rel_s"])
        if df.empty:
            continue
        t = df["t_rel_s"].values
        y = df[ycol].values
        t_max = max(t_max, t.max())
        series.append((t, y))
    if not series:
        return None, None, None
    t_grid = np.arange(0, t_max + bin_sec, bin_sec)
    interpolated = np.array([_ts_to_grid(t, y, t_grid) for t, y in series])
    with warnings.catch_warnings():
        warnings.simplefilter("ignore", RuntimeWarning)
        mean = np.nanmean(interpolated, axis=0)
        std  = np.nan_to_num(np.nanstd(interpolated, axis=0), nan=0.0)
    return t_grid, mean, std


# ===========================================================================
# GROUP 01 — Validation
# ===========================================================================

def _plot_01_network_pdr_over_time(runs, fig_dir, missing):
    """G1/P1 — Rolling-window PDR timeseries for all protocols on one axes."""
    fig, ax = plt.subplots(figsize=(12, 5))
    colors = protocol_color_map([r["protocol"] for r in runs])
    any_data = False

    for run in runs:
        df = _load(run, "network_timeseries", missing)
        if df is None or "window_pdr" not in df.columns or "t_rel_s" not in df.columns:
            continue
        df = df[df["window_pdr"] >= 0].copy()
        if df.empty:
            continue
        ax.plot(
            df["t_rel_s"] / 60,
            df["window_pdr"],
            alpha=0.75,
            linewidth=1.2,
            color=colors[run["protocol"]],
            label=run["label"],
        )
        any_data = True

    if not any_data:
        missing.append("PLOT 01_network_pdr_over_time: no valid window_pdr data")
        plt.close(fig)
        return

    ax.axhline(0.95, color="red", linestyle="--", linewidth=1, label="PDR target 0.95")
    ax.set_xlabel("Experiment time (min)")
    ax.set_ylabel("Window PDR")
    ax.set_title("Network PDR Over Time — All Protocols")
    ax.set_ylim(-0.05, 1.05)
    deduplicate_legend(ax)
    savefig(fig, fig_dir, "01_network_pdr_over_time")
    print("  [OK] 01_network_pdr_over_time")


def _plot_01_battery_ecdf(runs, fig_dir, missing):
    """G1/P2 — ECDF of per-sample UAV battery levels, one curve per run."""
    fig, ax = plt.subplots(figsize=(9, 6))
    colors = protocol_color_map([r["protocol"] for r in runs])
    any_data = False

    for run in runs:
        df = _load(run, "status_timeseries", missing)
        if df is None or "battery_level" not in df.columns:
            continue
        # Exclude the infrastructure sink_gateway node.
        if "uav_id" in df.columns:
            df = df[df["uav_id"] != "sink_gateway"]
        vals = df["battery_level"].dropna().values
        if len(vals) == 0:
            continue
        xs, ys = ecdf(vals)
        ax.plot(xs, ys, linewidth=1.5, color=colors[run["protocol"]], label=run["label"])
        any_data = True

    if not any_data:
        missing.append("PLOT 01_battery_ecdf: no battery_level data")
        plt.close(fig)
        return

    ax.set_xlabel("Battery level (%)")
    ax.set_ylabel("Cumulative probability")
    ax.set_title("UAV Battery Level ECDF — All Protocols")
    deduplicate_legend(ax)
    savefig(fig, fig_dir, "01_battery_ecdf")
    print("  [OK] 01_battery_ecdf")


# Packet types kept as individual slices; everything else → "Other"
_PKT_MAJOR_TYPES = [
    "HEARTBEAT", "SEARCH_TELEMETRY", "STATUS_CH",
    "CHARGE_REQUEST", "CHARGE_DECISION",
    "EMERGENCY_RETURN_TRIGGERED", "EMERGENCY_RECOVERY_COMPLETE",
]


def _packet_type_counts(df: pd.DataFrame) -> dict:
    """Return {type: count} with minor types collapsed into 'Other'."""
    counts = df["control_type"].value_counts().to_dict()
    result = {t: counts.pop(t, 0) for t in _PKT_MAJOR_TYPES}
    result["Other"] = sum(counts.values())
    # Drop zero-count types
    return {k: v for k, v in result.items() if v > 0}


def _plot_01_packet_generated_per_run(runs, fig_dir, missing):
    """G1/P3 — Stacked bar: total packets generated per run, broken down by type."""
    run_labels = []
    type_data: dict = {}   # type → list of counts (one per run)
    any_data = False

    for run in runs:
        df = _load(run, "packet_generated_events", missing)
        if df is None or "control_type" not in df.columns:
            continue
        counts = _packet_type_counts(df)
        run_labels.append(run["label"])
        for t in list(_PKT_MAJOR_TYPES) + ["Other"]:
            type_data.setdefault(t, []).append(counts.get(t, 0))
        any_data = True

    if not any_data:
        missing.append("PLOT 01_packet_generated_per_run: no packet_generated_events data")
        return

    # Only keep types that appear at all
    active_types = [t for t in list(_PKT_MAJOR_TYPES) + ["Other"]
                    if t in type_data and sum(type_data[t]) > 0]

    cmap = plt.get_cmap("tab10")
    type_colors = {t: cmap(i % 10) for i, t in enumerate(active_types)}

    fig, ax = plt.subplots(figsize=(max(10, len(run_labels) * 1.1), 6))
    x = np.arange(len(run_labels))
    bottom = np.zeros(len(run_labels))

    for t in active_types:
        vals = np.array(type_data[t])
        ax.bar(x, vals, bottom=bottom, label=t, color=type_colors[t],
               edgecolor="white", linewidth=0.4)
        bottom += vals

    ax.set_xticks(x)
    ax.set_xticklabels(run_labels, rotation=30, ha="right", fontsize=8)
    ax.set_ylabel("Packet count")
    ax.set_title("Total Packets Generated per Run — by Type")
    ax.legend(loc="upper right", fontsize=8, ncol=2)
    fig.tight_layout()
    savefig(fig, fig_dir, "01_packet_generated_per_run")
    print("  [OK] 01_packet_generated_per_run")


# ===========================================================================
# GROUP 02 — Per-Protocol Stats
# ===========================================================================

def _plot_02_charge_success_rate(runs, fig_dir, missing):
    """G2/P1 — Bar chart: charge success rate per protocol."""
    labels, rates = [], []
    label_to_protocol = {}

    for run in runs:
        summary = _load_summary(run)
        if summary is not None:
            ch = summary.get("charging", {})
            total = ch.get("requests_total", 0)
            started = ch.get("started", 0)
            rate = started / total if total > 0 else float("nan")
        else:
            df = _load(run, "charge_events", missing)
            if df is None or "outcome" not in df.columns:
                continue
            total = len(df)
            rate = int((df["outcome"] == "STARTED").sum()) / total if total > 0 else float("nan")

        labels.append(run["label"])
        rates.append(rate)
        label_to_protocol[run["label"]] = run["protocol"]

    if not labels:
        missing.append("PLOT 02_charge_success_rate: no data")
        return

    colors = protocol_color_map(list(label_to_protocol.values()))
    bar_colors = [colors[label_to_protocol[lbl]] for lbl in labels]

    fig, ax = plt.subplots(figsize=(max(6, len(labels) * 1.3), 5))
    x = np.arange(len(labels))
    bars = ax.bar(x, rates, color=bar_colors, edgecolor="white", linewidth=0.5)
    label_bars(ax, bars, fmt="{:.1%}")
    ax.set_ylabel("Success rate  (STARTED / total requests)")
    ax.set_title("Charge Success Rate per Protocol")
    ax.set_ylim(0, 1.2)
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=20, ha="right")
    savefig(fig, fig_dir, "02_charge_success_rate")
    print("  [OK] 02_charge_success_rate")


def _plot_02_decision_latency(runs, fig_dir, missing):
    """G2/P2 — Box plot: charge decision latency per protocol."""
    data: dict = {}
    for run in runs:
        df = _load(run, "charge_events", missing)
        if df is None or "decision_latency_ms" not in df.columns:
            continue
        vals = df["decision_latency_ms"].replace(-1, float("nan")).dropna().values
        if len(vals) > 0:
            data[run["label"]] = vals.tolist()

    if not data:
        missing.append("PLOT 02_decision_latency: no decision_latency_ms data")
        return

    boxplot_multi(
        data,
        "Decision latency (ms)",
        "Charge Decision Latency Distribution per Protocol",
        fig_dir,
        "02_decision_latency",
    )
    print("  [OK] 02_decision_latency")


def _plot_02_effective_wait(runs, fig_dir, missing):
    """G2/P3 — Box plot: effective wait time per protocol."""
    data: dict = {}
    for run in runs:
        df = _load(run, "charge_events", missing)
        if df is None or "effective_wait_ms" not in df.columns:
            continue
        vals = df["effective_wait_ms"].replace(-1, float("nan")).dropna().values
        if len(vals) > 0:
            data[run["label"]] = vals.tolist()

    if not data:
        missing.append("PLOT 02_effective_wait: no effective_wait_ms data")
        return

    boxplot_multi(
        data,
        "Effective wait (ms)",
        "Effective Wait Time per Protocol",
        fig_dir,
        "02_effective_wait",
    )
    print("  [OK] 02_effective_wait")


def _plot_02_energy_recovered(runs, fig_dir, missing):
    """G2/P4 — Box plot: energy recovered per completed charge session."""
    data: dict = {}
    for run in runs:
        vals = _energy_recovered_series(run, missing)
        if vals is not None and len(vals) > 0:
            data[run["label"]] = vals.tolist()

    if not data:
        missing.append("PLOT 02_energy_recovered: no energy_recovered data")
        return

    boxplot_multi(
        data,
        "Energy recovered (Wh)",
        "Energy Recovered per Charge Session",
        fig_dir,
        "02_energy_recovered",
    )
    print("  [OK] 02_energy_recovered")


# ===========================================================================
# GROUP 03 — Cross-Protocol Charging
# ===========================================================================

def _plot_03_charge_queue_length(runs, fig_dir, missing):
    """G3/P1 — Total charge queue length timeseries, all protocols overlaid."""
    fig, ax = plt.subplots(figsize=(12, 5))
    colors = protocol_color_map([r["protocol"] for r in runs])
    any_data = False

    for run in runs:
        df = _load(run, "charge_queue_timeseries", missing)
        if df is None or "t_rel_s" not in df.columns:
            continue
        role_cols = [c for c in ("queue_length_ugv", "queue_length_ch", "queue_length_member")
                     if c in df.columns]
        if not role_cols:
            continue
        df = df.sort_values("t_rel_s")
        total_q = df[role_cols].clip(lower=0).sum(axis=1)
        ax.plot(
            df["t_rel_s"] / 60,
            total_q,
            color=colors[run["protocol"]],
            linewidth=1.2,
            alpha=0.8,
            label=run["label"],
        )
        any_data = True

    if not any_data:
        missing.append("PLOT 03_charge_queue_length: no charge_queue_timeseries data")
        plt.close(fig)
        return

    ax.set_xlabel("Experiment time (min)")
    ax.set_ylabel("Total charge queue length")
    ax.set_title("Charge Queue Length Over Time — All Protocols")
    deduplicate_legend(ax)
    savefig(fig, fig_dir, "03_charge_queue_length")
    print("  [OK] 03_charge_queue_length")


def _plot_03_dock_utilization(runs, fig_dir, missing):
    """G3/P2 — UGV dock utilisation timeseries, all protocols overlaid."""
    fig, ax = plt.subplots(figsize=(12, 5))
    colors = protocol_color_map([r["protocol"] for r in runs])
    any_data = False

    for run in runs:
        df = _load(run, "charge_queue_timeseries", missing)
        if (
            df is None
            or "ugv_dock_utilization" not in df.columns
            or "t_rel_s" not in df.columns
        ):
            continue
        df = df[df["ugv_dock_utilization"] >= 0].sort_values("t_rel_s")
        ax.plot(
            df["t_rel_s"] / 60,
            df["ugv_dock_utilization"],
            color=colors[run["protocol"]],
            linewidth=1.2,
            alpha=0.8,
            label=run["label"],
        )
        any_data = True

    if not any_data:
        missing.append("PLOT 03_dock_utilization: no ugv_dock_utilization data")
        plt.close(fig)
        return

    ax.set_xlabel("Experiment time (min)")
    ax.set_ylabel("Dock utilisation (fraction)")
    ax.set_title("UGV Dock Utilisation Over Time — All Protocols")
    ax.set_ylim(-0.05, 1.05)
    deduplicate_legend(ax)
    savefig(fig, fig_dir, "03_dock_utilization")
    print("  [OK] 03_dock_utilization")


def _is_preemptive(protocol_name: str) -> bool:
    """Return True when the protocol uses preemption (name contains '_p_')."""
    return "_p_" in protocol_name.lower()


def _plot_03_charge_outcome_breakdown(runs, fig_dir, missing):
    """G3/P3 — Stacked bar of charge request outcomes per protocol.

    PREEMPTED is silenced (forced to 0) for protocols whose name does not
    contain ``_p_``, because preemption is only meaningful in preemptive
    scheduling variants.  The PREEMPTED legend entry is shown only when at
    least one protocol is preemptive.
    """
    ALL_OUTCOMES = ["STARTED", "REJECTED", "DROPPED", "TIMEOUT", "PREEMPTED", "ENERGY_DEPLETED"]
    PALETTE = {
        "STARTED":        "#2ca02c",
        "REJECTED":       "#d62728",
        "DROPPED":        "#ff7f0e",
        "TIMEOUT":        "#9467bd",
        "PREEMPTED":      "#8c564b",
        "ENERGY_DEPLETED":"#1f77b4",
    }

    rows = []
    for run in runs:
        df = _load(run, "charge_events", missing)
        if df is None or "outcome" not in df.columns:
            continue
        counts = df["outcome"].value_counts()
        row = {"label": run["label"], "_preemptive": _is_preemptive(run["protocol"])}
        for o in ALL_OUTCOMES:
            val = int(counts.get(o, 0))
            # Suppress PREEMPTED counts for non-preemptive protocols
            row[o] = val if (o != "PREEMPTED" or row["_preemptive"]) else 0
        rows.append(row)

    if not rows:
        missing.append("PLOT 03_charge_outcome_breakdown: no outcome data")
        return

    summary_df = pd.DataFrame(rows).set_index("label")
    any_preemptive = summary_df["_preemptive"].any()
    summary_df = summary_df.drop(columns=["_preemptive"])
    # Only include PREEMPTED in the draw loop when at least one protocol uses it
    outcomes_to_draw = [o for o in ALL_OUTCOMES if o != "PREEMPTED" or any_preemptive]
    labels = summary_df.index.tolist()

    fig, ax = plt.subplots(figsize=(max(8, len(labels) * 1.5), 6))
    bottom = np.zeros(len(labels))
    for outcome in outcomes_to_draw:
        if outcome not in summary_df.columns:
            continue
        vals = summary_df[outcome].values
        ax.bar(
            labels,
            vals,
            bottom=bottom,
            color=PALETTE[outcome],
            label=outcome,
            edgecolor="white",
            linewidth=0.3,
        )
        bottom += vals

    ax.set_ylabel("Number of charge requests")
    ax.set_title("Charge Request Outcomes per Protocol")
    ax.set_xticks(np.arange(len(labels)))
    ax.set_xticklabels(labels, rotation=20, ha="right")
    ax.legend(loc="upper right", fontsize=8)
    savefig(fig, fig_dir, "03_charge_outcome_breakdown")
    print("  [OK] 03_charge_outcome_breakdown")
    return summary_df  # available for caller to persist as derived table


def _plot_03_dead_uav_cumulative(runs, fig_dir, missing):
    """G3/P4 — Cumulative UAV death events over time, all protocols."""
    fig, ax = plt.subplots(figsize=(12, 5))
    colors = protocol_color_map([r["protocol"] for r in runs])
    any_data = False

    for run in runs:
        df = _load(run, "death_events", missing)
        if df is not None and "t_rel_s" in df.columns:
            df = df.sort_values("t_rel_s")
            ax.step(
                df["t_rel_s"] / 60,
                np.arange(1, len(df) + 1),
                where="post",
                color=colors[run["protocol"]],
                linewidth=1.5,
                label=run["label"],
            )
            any_data = True
        else:
            # Fallback: read cumulative count from charge_queue_timeseries.
            dfq = _load(run, "charge_queue_timeseries", missing)
            if (
                dfq is not None
                and "dead_event_count" in dfq.columns
                and "t_rel_s" in dfq.columns
            ):
                dfq = dfq.sort_values("t_rel_s")
                ax.plot(
                    dfq["t_rel_s"] / 60,
                    dfq["dead_event_count"],
                    color=colors[run["protocol"]],
                    linewidth=1.5,
                    label=run["label"],
                )
                any_data = True

    if not any_data:
        missing.append("PLOT 03_dead_uav_cumulative: no death data")
        plt.close(fig)
        return

    ax.set_xlabel("Experiment time (min)")
    ax.set_ylabel("Cumulative dead UAVs")
    ax.set_title("Cumulative UAV Deaths Over Time — All Protocols")
    deduplicate_legend(ax)
    savefig(fig, fig_dir, "03_dead_uav_cumulative")
    print("  [OK] 03_dead_uav_cumulative")


def _cumulative_energy_series(run: dict, missing: list):
    """Return *(t_min_array, cumulative_wh_array, unit_str)* for *run*, or *(None, None, None)*.

    Source priority:
    1. charge_session_events.csv  DOCK_END  ->  energy_charged_wh
    2. charge_session_events.csv  DOCK_END  ->  (battery_after - battery_before)
                                               x battery_capacity_wh / 100
    3. charge_events.csv  STARTED/ENERGY_DEPLETED  ->  energy_recovered_pct
       (units become %-pts, not Wh, only used as last resort)
    """
    _probe: list = []
    df2 = _load(run, "charge_session_events", _probe)
    if df2 is not None and "event_type" in df2.columns and "t_rel_s" in df2.columns:
        ended = df2[df2["event_type"] == "DOCK_END"].sort_values("t_rel_s")

        if "energy_charged_wh" in ended.columns:
            wh   = ended["energy_charged_wh"].replace(-1, float("nan"))
            mask = wh.notna() & (wh >= 0)
            if mask.sum() > 0:
                t = ended.loc[mask, "t_rel_s"].values / 60.0
                y = np.cumsum(wh[mask].values)
                return t, y, "Wh"

        if ("battery_before" in ended.columns and "battery_after" in ended.columns
                and "battery_capacity_wh" in ended.columns):
            cap   = ended["battery_capacity_wh"].replace(0, float("nan"))
            delta = (ended["battery_after"] - ended["battery_before"]).replace(-1, float("nan"))
            wh    = (delta * cap / 100.0)
            mask  = wh.notna() & (wh >= 0)
            if mask.sum() > 0:
                t = ended.loc[mask, "t_rel_s"].values / 60.0
                y = np.cumsum(wh[mask].values)
                return t, y, "Wh"

    # Fallback: charge_events.csv energy_recovered_pct (%-pts)
    df = _load(run, "charge_events", _probe)
    if df is not None and "t_rel_s" in df.columns:
        col = next((c for c in ("energy_recovered_pct", "energy_recovered") if c in df.columns), None)
        if col is not None:
            if "outcome" in df.columns:
                df = df[df["outcome"].isin(["STARTED", "ENERGY_DEPLETED"])].copy()
            df = df.sort_values("t_rel_s")
            vals = df[col].replace(-1, float("nan"))
            mask = vals.notna() & (vals >= 0)
            if mask.sum() > 0:
                t = df.loc[mask, "t_rel_s"].values / 60.0
                y = np.cumsum(vals[mask].values)
                return t, y, "%-pts"

    missing.extend(_probe)
    return None, None, None


def _plot_03_cumulative_energy_charged(runs, fig_dir, missing):
    """G3/P5 — Cumulative energy charged per run over time (step plot)."""
    fig, ax = plt.subplots(figsize=(12, 5))
    colors = protocol_color_map([r["protocol"] for r in runs])
    any_data = False
    unit_label = "Wh"

    for run in runs:
        t, y, unit = _cumulative_energy_series(run, missing)
        if t is None or len(t) == 0:
            continue
        unit_label = unit
        ax.step(t, y, where="post",
                color=colors[run["protocol"]],
                linewidth=1.5, alpha=0.85,
                label=run["label"])
        any_data = True

    if not any_data:
        missing.append("PLOT 03_cumulative_energy_charged: no energy data")
        plt.close(fig)
        return

    ax.set_xlabel("Experiment time (min)")
    ax.set_ylabel(f"Cumulative energy charged ({unit_label})")
    ax.set_title("Cumulative Energy Charged Over Time — All Protocols")
    deduplicate_legend(ax)
    savefig(fig, fig_dir, "03_cumulative_energy_charged")
    print("  [OK] 03_cumulative_energy_charged")


# ===========================================================================
# GROUP 04 — Policy Radar
# ===========================================================================

def _plot_04_policy_radar(runs, fig_dir, missing):
    """G4/P1 — Radar/spider chart comparing protocols on 5 normalised KPIs."""
    # (label, display_name, max_value, higher_is_better)
    AXES = [
        ("pdr",      "PDR",            1.0,    True),
        ("success",  "Success rate",   1.0,    True),
        ("latency",  "Low latency",    100_000, False),  # ms → invert
        ("energy",   "Energy recov.",  100.0,  True),
        ("survival", "Survival",       1.0,    True),
    ]

    def _norm(val, max_val, higher_is_better):
        if np.isnan(val):
            return 0.0
        n = float(val) / max_val
        if not higher_is_better:
            n = 1.0 - min(n, 1.0)
        return float(np.clip(n, 0.0, 1.0))

    protocol_scores: dict = {}

    for run in runs:
        summary = _load_summary(run)
        if summary is not None:
            net = summary.get("network", {})
            by_cat = net.get("by_category", [])
            total_gen = sum(c.get("generated", 0) for c in by_cat)
            total_del = sum(c.get("delivered", 0) for c in by_cat)
            pdr = total_del / total_gen if total_gen > 0 else float("nan")

            ch = summary.get("charging", {})
            success = ch.get("success_rate", float("nan"))
            latency = ch.get("decision_latency_ms", {}).get("mean", float("nan"))
            energy = ch.get("energy_recovered", {}).get("mean", float("nan"))
            survival = summary.get("fleet", {}).get("survival_rate", float("nan"))
        else:
            pdr = _overall_pdr_from_csvs(run, missing)
            success = _success_rate_from_csvs(run, missing)
            latency = _mean_latency_from_csvs(run, missing)
            energy = _mean_energy_from_csvs(run, missing)
            survival = float("nan")

        raw = [pdr, success, latency, energy, survival]
        protocol_scores[run["label"]] = [
            _norm(v, AXES[i][2], AXES[i][3]) for i, v in enumerate(raw)
        ]

    if not protocol_scores:
        missing.append("PLOT 04_policy_radar: no summary data available")
        return

    axis_labels = [a[1] for a in AXES]
    N = len(axis_labels)
    angles = np.linspace(0, 2 * np.pi, N, endpoint=False).tolist()
    angles += angles[:1]

    fig, ax = plt.subplots(figsize=(7, 7), subplot_kw={"polar": True})
    colors = protocol_color_map(list(protocol_scores.keys()))

    for label, scores in protocol_scores.items():
        vals = scores + scores[:1]
        ax.plot(angles, vals, linewidth=1.5, label=label, color=colors[label])
        ax.fill(angles, vals, alpha=0.08, color=colors[label])

    ax.set_xticks(angles[:-1])
    ax.set_xticklabels(axis_labels, fontsize=9)
    ax.set_ylim(0, 1)
    ax.set_yticks([0.25, 0.5, 0.75, 1.0])
    ax.set_yticklabels(["0.25", "0.5", "0.75", "1.0"], fontsize=7)
    ax.set_title("Policy Radar — Normalised KPIs", pad=20)
    ax.legend(loc="upper right", bbox_to_anchor=(1.35, 1.1), fontsize=8)
    savefig(fig, fig_dir, "04_policy_radar")
    print("  [OK] 04_policy_radar")


# ===========================================================================
# GROUP 05 — Weather
# ===========================================================================

def _plot_05_pdr_vs_weather_regime(runs, fig_dir, missing):
    """G5/P1 — Box plot: network window PDR grouped by weather regime."""
    regime_pdr: dict = {}

    for run in runs:
        net_df = _load(run, "network_timeseries", missing)
        w_df = _load(run, "weather_timeseries", missing)
        if net_df is None or w_df is None:
            continue
        if "window_pdr" not in net_df.columns or "t_rel_s" not in net_df.columns:
            continue
        if "regime" not in w_df.columns or "t_rel_s" not in w_df.columns:
            continue

        net_df = net_df[net_df["window_pdr"] >= 0].copy()
        merged = merge_asof_weather(
            net_df, w_df, time_col="t_rel_s", weather_time_col="t_rel_s"
        )
        if "regime" not in merged.columns:
            continue
        for regime, grp in merged.groupby("regime"):
            regime_pdr.setdefault(str(regime), []).extend(
                grp["window_pdr"].dropna().tolist()
            )

    if not regime_pdr:
        missing.append("PLOT 05_pdr_vs_weather_regime: no merged PDR+weather data")
        return

    regimes = sorted(regime_pdr.keys())
    data = [regime_pdr[r] for r in regimes]

    fig, ax = plt.subplots(figsize=(max(6, len(regimes) * 1.5), 5))
    bp = ax.boxplot(data, patch_artist=True, showfliers=False)
    for patch in bp["boxes"]:
        patch.set_facecolor("#4878d0")
        patch.set_alpha(0.6)
    ax.set_xticks(range(1, len(regimes) + 1))
    ax.set_xticklabels(regimes)
    ax.set_ylabel("Window PDR")
    ax.set_title("Network PDR vs Weather Regime (all protocols)")
    ax.set_ylim(-0.05, 1.05)
    savefig(fig, fig_dir, "05_pdr_vs_weather_regime")
    print("  [OK] 05_pdr_vs_weather_regime")


def _plot_05_weather_timeseries(runs, fig_dir, missing):
    """G5/P2 — Rain intensity + wind speed over time (one representative run)."""
    for run in runs:
        df = _load(run, "weather_timeseries", missing)
        if df is None or "t_rel_s" not in df.columns:
            continue
        df = df.sort_values("t_rel_s")

        n_panels = sum(
            [
                int("rain_intensity" in df.columns),
                int("wind_speed" in df.columns),
                int("temperature_c" in df.columns),
            ]
        )
        if n_panels == 0:
            continue

        fig, axes = plt.subplots(n_panels, 1, figsize=(12, 3 * n_panels), sharex=True)
        if n_panels == 1:
            axes = [axes]

        panel = 0
        if "rain_intensity" in df.columns:
            axes[panel].plot(df["t_rel_s"] / 60, df["rain_intensity"], color="#1f77b4", linewidth=1)
            axes[panel].set_ylabel("Rain intensity")
            panel += 1
        if "wind_speed" in df.columns:
            axes[panel].plot(df["t_rel_s"] / 60, df["wind_speed"], color="#ff7f0e", linewidth=1)
            axes[panel].set_ylabel("Wind speed (m/s)")
            panel += 1
        if "temperature_c" in df.columns:
            axes[panel].plot(df["t_rel_s"] / 60, df["temperature_c"], color="#2ca02c", linewidth=1)
            axes[panel].set_ylabel("Temperature (°C)")
            panel += 1

        axes[-1].set_xlabel("Experiment time (min)")
        fig.suptitle(f"Weather Timeseries — {run['label']}", fontsize=11)
        fig.tight_layout()

        name = f"05_weather_timeseries_{run['protocol']}_r{run['replicate']}"
        savefig(fig, fig_dir, name)
        print(f"  [OK] {name}")
        # One representative weather plot is enough (weather is shared
        # across protocols in the simulation).
        break


# ===========================================================================
# MERGED PLOTS — one entry per protocol, replicates pooled
# ===========================================================================

# --- Group 01 merged --------------------------------------------------------

def _plot_01_network_pdr_over_time_merged(runs, fig_dir, missing, bin_sec):
    """G1/P1-M — PDR timeseries: mean ± 1σ band per protocol (replicates merged)."""
    groups = _group_by_protocol(runs)
    colors = protocol_color_map(list(groups.keys()))
    fig, ax = plt.subplots(figsize=(12, 5))
    any_data = False

    for proto, run_list in groups.items():
        t_grid, mean, std = _merged_ts(
            run_list, "network_timeseries", "window_pdr", missing, bin_sec
        )
        if t_grid is None:
            continue
        valid = ~np.isnan(mean)
        ax.plot(t_grid[valid] / 60, mean[valid],
                color=colors[proto], linewidth=1.8, label=proto)
        any_data = True

    if not any_data:
        missing.append("PLOT 01_network_pdr_over_time_merged: no valid window_pdr data")
        plt.close(fig)
        return

    ax.axhline(0.95, color="red", linestyle="--", linewidth=1, label="PDR target 0.95")
    ax.set_xlabel("Experiment time (min)")
    ax.set_ylabel("Window PDR  (mean across replicates)")
    ax.set_title("Network PDR Over Time — Merged Replicates")
    ax.set_ylim(-0.05, 1.05)
    deduplicate_legend(ax)
    savefig(fig, fig_dir, "01_network_pdr_over_time_merged")
    print("  [OK] 01_network_pdr_over_time_merged")


def _plot_01_battery_ecdf_merged(runs, fig_dir, missing):
    """G1/P2-M — Battery ECDF: one curve per protocol, all replicates pooled."""
    groups = _group_by_protocol(runs)
    colors = protocol_color_map(list(groups.keys()))
    fig, ax = plt.subplots(figsize=(9, 6))
    any_data = False

    for proto, run_list in groups.items():
        all_vals = []
        for run in run_list:
            df = _load(run, "status_timeseries", missing)
            if df is None or "battery_level" not in df.columns:
                continue
            if "uav_id" in df.columns:
                df = df[df["uav_id"] != "sink_gateway"]
            all_vals.extend(df["battery_level"].dropna().tolist())
        if not all_vals:
            continue
        xs, ys = ecdf(np.array(all_vals))
        ax.plot(xs, ys, linewidth=1.8, color=colors[proto], label=proto)
        any_data = True

    if not any_data:
        missing.append("PLOT 01_battery_ecdf_merged: no battery_level data")
        plt.close(fig)
        return

    ax.set_xlabel("Battery level (%)")
    ax.set_ylabel("Cumulative probability")
    ax.set_title("UAV Battery Level ECDF — Merged Replicates")
    deduplicate_legend(ax)
    savefig(fig, fig_dir, "01_battery_ecdf_merged")
    print("  [OK] 01_battery_ecdf_merged")


def _plot_01_pdr_cdf_merged(runs, fig_dir, missing, pdr_min: float = 0.5):
    """G1/P3-M — CDF of window PDR (≥ pdr_min) per protocol, all replicates pooled."""
    groups = _group_by_protocol(runs)
    colors = protocol_color_map(list(groups.keys()))
    fig, ax = plt.subplots(figsize=(9, 6))
    any_data = False

    for proto, run_list in groups.items():
        all_vals = []
        for run in run_list:
            df = _load(run, "network_timeseries", missing)
            if df is None or "window_pdr" not in df.columns:
                continue
            vals = df["window_pdr"].dropna()
            vals = vals[vals >= pdr_min]
            all_vals.extend(vals.tolist())
        if not all_vals:
            continue
        xs, ys = ecdf(np.array(all_vals))
        ax.plot(xs, ys, linewidth=1.8, color=colors[proto], label=proto)
        any_data = True

    if not any_data:
        missing.append("PLOT 01_pdr_cdf_merged: no window_pdr data")
        plt.close(fig)
        return

    ax.axvline(0.95, color="red", linestyle="--", linewidth=1, label="PDR target 0.95")
    ax.set_xlabel(f"Window PDR  (samples ≥ {pdr_min})")
    ax.set_ylabel("Cumulative probability")
    ax.set_title(f"Network PDR CDF — Merged Replicates (PDR ≥ {pdr_min})")
    ax.set_xlim(pdr_min - 0.02, 1.02)
    ax.set_ylim(-0.02, 1.05)
    deduplicate_legend(ax)
    savefig(fig, fig_dir, "01_pdr_cdf_merged")
    print("  [OK] 01_pdr_cdf_merged")


def _plot_01_packet_generated_merged(runs, fig_dir, missing):
    """G1/P4-M — Stacked bar: mean total packets generated per protocol (replicates
    merged), with packet-type ratio breakdown.  Error bars show std across replicates
    on the total count."""
    groups = _group_by_protocol(runs)
    colors = protocol_color_map(list(groups.keys()))

    active_types = list(_PKT_MAJOR_TYPES) + ["Other"]
    # proto → {type: [count_per_replicate]}
    proto_type_counts: dict = {}
    any_data = False

    for proto, run_list in groups.items():
        per_type: dict = {t: [] for t in active_types}
        for run in run_list:
            df = _load(run, "packet_generated_events", missing)
            if df is None or "control_type" not in df.columns:
                continue
            counts = _packet_type_counts(df)
            for t in active_types:
                per_type[t].append(counts.get(t, 0))
            any_data = True
        if any(per_type[t] for t in active_types):
            proto_type_counts[proto] = per_type

    if not any_data:
        missing.append("PLOT 01_packet_generated_merged: no packet_generated_events data")
        return

    proto_labels = list(proto_type_counts.keys())
    used_types = [t for t in active_types
                  if any(sum(proto_type_counts[p].get(t, [])) > 0
                         for p in proto_labels)]

    cmap = plt.get_cmap("tab10")
    type_colors = {t: cmap(i % 10) for i, t in enumerate(used_types)}

    fig, (ax_bar, ax_ratio) = plt.subplots(
        1, 2, figsize=(max(12, len(proto_labels) * 2.2), 6),
        gridspec_kw={"width_ratios": [3, 2]}
    )

    x = np.arange(len(proto_labels))
    # --- left: stacked bar of mean counts with std error on total ---
    totals_mean = np.zeros(len(proto_labels))
    totals_std  = np.zeros(len(proto_labels))
    bottom = np.zeros(len(proto_labels))

    for t in used_types:
        vals = np.array([
            np.mean(proto_type_counts[p].get(t, [0])) for p in proto_labels
        ])
        ax_bar.bar(x, vals, bottom=bottom, label=t, color=type_colors[t],
                   edgecolor="white", linewidth=0.4)
        bottom += vals

    for i, p in enumerate(proto_labels):
        rep_totals = [
            sum(proto_type_counts[p][t][j] if j < len(proto_type_counts[p][t]) else 0
                for t in used_types)
            for j in range(max(len(proto_type_counts[p].get(t, [])) for t in used_types))
        ]
        totals_mean[i] = np.mean(rep_totals) if rep_totals else bottom[i]
        totals_std[i]  = np.std(rep_totals) if len(rep_totals) > 1 else 0

    ax_bar.errorbar(x, totals_mean, yerr=totals_std, fmt="none",
                    ecolor="black", capsize=4, linewidth=1.2)
    ax_bar.set_xticks(x)
    ax_bar.set_xticklabels(proto_labels, rotation=25, ha="right", fontsize=8)
    ax_bar.set_ylabel("Mean packet count  (replicates averaged)")
    ax_bar.set_title("Packets Generated — Mean per Protocol")
    ax_bar.legend(loc="upper right", fontsize=7, ncol=2)

    # --- right: 100 % stacked ratio bar ---
    bottoms_r = np.zeros(len(proto_labels))
    for t in used_types:
        means = np.array([
            np.mean(proto_type_counts[p].get(t, [0])) for p in proto_labels
        ])
        totals = np.array([
            np.mean([sum(proto_type_counts[p][tt][j]
                         for tt in used_types
                         if j < len(proto_type_counts[p].get(tt, [])))
                     for j in range(max(1, max(
                         len(proto_type_counts[p].get(tt, [])) for tt in used_types
                     )))])
            for p in proto_labels
        ])
        ratios = np.where(totals > 0, means / totals, 0.0)
        ax_ratio.bar(x, ratios, bottom=bottoms_r, label=t, color=type_colors[t],
                     edgecolor="white", linewidth=0.4)
        bottoms_r += ratios

    ax_ratio.set_xticks(x)
    ax_ratio.set_xticklabels(proto_labels, rotation=25, ha="right", fontsize=8)
    ax_ratio.set_ylabel("Fraction of total packets")
    ax_ratio.set_ylim(0, 1.05)
    ax_ratio.set_title("Packet Type Ratio — Merged Protocols")

    fig.tight_layout()
    savefig(fig, fig_dir, "01_packet_generated_merged")
    print("  [OK] 01_packet_generated_merged")


def _plot_01_mean_pdr_merged(runs, fig_dir, missing):
    """G1/P5-M — Bar chart: overall mean PDR per protocol (replicates merged),
    error bars = std across replicates.  PDR is computed from qos_metrics.csv as
    sum(delivered) / sum(generated) per replicate."""
    groups = _group_by_protocol(runs)
    colors = protocol_color_map(list(groups.keys()))

    proto_labels, means, errs = [], [], []

    for proto, run_list in groups.items():
        pdrs = []
        for run in run_list:
            df = _load(run, "qos_metrics", missing)
            if df is None or "generated" not in df.columns or "delivered" not in df.columns:
                continue
            valid = df[(df["pdr"] >= 0) & (df["generated"] > 0)]
            if valid.empty:
                continue
            total_gen = valid["generated"].sum()
            total_del = valid["delivered"].sum()
            if total_gen > 0:
                pdrs.append(total_del / total_gen)
        if not pdrs:
            continue
        proto_labels.append(proto)
        means.append(float(np.mean(pdrs)))
        errs.append(float(np.std(pdrs)) if len(pdrs) > 1 else 0.0)

    if not proto_labels:
        missing.append("PLOT 01_mean_pdr_merged: no valid PDR data in qos_metrics")
        return

    bar_colors = [colors[p] for p in proto_labels]
    fig, ax = plt.subplots(figsize=(max(6, len(proto_labels) * 1.5), 5))
    x = np.arange(len(proto_labels))
    bars = ax.bar(x, means, yerr=errs, capsize=5,
                  color=bar_colors, edgecolor="white", linewidth=0.5,
                  error_kw={"elinewidth": 1.4, "ecolor": "black"})
    label_bars(ax, bars, fmt="{:.3f}")
    ax.axhline(0.95, color="red", linestyle="--", linewidth=1, label="PDR target 0.95")
    ax.set_ylabel("Mean overall PDR  (mean across replicates ± std)")
    ax.set_title("Mean PDR Comparison — Merged Replicates")
    ax.set_ylim(0, 1.1)
    ax.set_xticks(x)
    ax.set_xticklabels(proto_labels, rotation=25, ha="right")
    ax.legend(fontsize=8)
    fig.tight_layout()
    savefig(fig, fig_dir, "01_mean_pdr_merged")
    print("  [OK] 01_mean_pdr_merged")


# --- Group 02 merged --------------------------------------------------------

def _plot_02_charge_success_rate_merged(runs, fig_dir, missing):
    """G2/P1-M — Success rate bar: mean ± std across replicates, one bar per protocol."""
    groups = _group_by_protocol(runs)
    labels, means, errs = [], [], []
    colors_map = protocol_color_map(list(groups.keys()))

    for proto, run_list in groups.items():
        rates = []
        for run in run_list:
            summary = _load_summary(run)
            if summary is not None:
                ch = summary.get("charging", {})
                total = ch.get("requests_total", 0)
                started = ch.get("started", 0)
                rate = started / total if total > 0 else float("nan")
            else:
                df = _load(run, "charge_events", missing)
                if df is None or "outcome" not in df.columns:
                    continue
                total = len(df)
                rate = int((df["outcome"] == "STARTED").sum()) / total if total > 0 else float("nan")
            rates.append(rate)
        if not rates:
            continue
        labels.append(proto)
        means.append(float(np.nanmean(rates)))
        errs.append(float(np.nanstd(rates)))

    if not labels:
        missing.append("PLOT 02_charge_success_rate_merged: no data")
        return

    bar_colors = [colors_map[lbl] for lbl in labels]
    fig, ax = plt.subplots(figsize=(max(6, len(labels) * 1.5), 5))
    x = np.arange(len(labels))
    bars = ax.bar(x, means, color=bar_colors, edgecolor="white", linewidth=0.5)
    label_bars(ax, bars, fmt="{:.1%}")
    ax.set_ylabel("Success rate  (mean across replicates)")
    ax.set_title("Charge Success Rate — Merged Replicates")
    ax.set_ylim(0, 1.2)
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=20, ha="right")
    savefig(fig, fig_dir, "02_charge_success_rate_merged")
    print("  [OK] 02_charge_success_rate_merged")


def _plot_02_decision_latency_merged(runs, fig_dir, missing):
    """G2/P2-M — Decision latency boxplot, one box per protocol (replicates pooled)."""
    groups = _group_by_protocol(runs)
    data: dict = {}
    for proto, run_list in groups.items():
        all_vals = []
        for run in run_list:
            df = _load(run, "charge_events", missing)
            if df is None or "decision_latency_ms" not in df.columns:
                continue
            all_vals.extend(
                df["decision_latency_ms"].replace(-1, float("nan")).dropna().tolist()
            )
        if all_vals:
            data[proto] = all_vals

    if not data:
        missing.append("PLOT 02_decision_latency_merged: no decision_latency_ms data")
        return

    boxplot_multi(
        data, "Decision latency (ms)",
        "Charge Decision Latency — Merged Replicates",
        fig_dir, "02_decision_latency_merged",
    )
    print("  [OK] 02_decision_latency_merged")


def _plot_02_effective_wait_merged(runs, fig_dir, missing):
    """G2/P3-M — Effective wait boxplot, one box per protocol (replicates pooled)."""
    groups = _group_by_protocol(runs)
    data: dict = {}
    for proto, run_list in groups.items():
        all_vals = []
        for run in run_list:
            df = _load(run, "charge_events", missing)
            if df is None or "effective_wait_ms" not in df.columns:
                continue
            all_vals.extend(
                df["effective_wait_ms"].replace(-1, float("nan")).dropna().tolist()
            )
        if all_vals:
            data[proto] = all_vals

    if not data:
        missing.append("PLOT 02_effective_wait_merged: no effective_wait_ms data")
        return

    boxplot_multi(
        data, "Effective wait (ms)",
        "Effective Wait Time — Merged Replicates",
        fig_dir, "02_effective_wait_merged",
    )
    print("  [OK] 02_effective_wait_merged")


def _plot_02_energy_recovered_merged(runs, fig_dir, missing):
    """G2/P4-M — Energy recovered boxplot, one box per protocol (replicates pooled)."""
    groups = _group_by_protocol(runs)
    data: dict = {}
    for proto, run_list in groups.items():
        all_vals = []
        for run in run_list:
            vals = _energy_recovered_series(run, missing)
            if vals is not None:
                all_vals.extend(vals.tolist())
        if all_vals:
            data[proto] = all_vals

    if not data:
        missing.append("PLOT 02_energy_recovered_merged: no energy_recovered data")
        return

    boxplot_multi(
        data, "Energy recovered (Wh)",
        "Energy Recovered per Charge Session — Merged Replicates",
        fig_dir, "02_energy_recovered_merged",
    )
    print("  [OK] 02_energy_recovered_merged")


# --- Group 03 merged --------------------------------------------------------

def _plot_03_charge_queue_length_merged(runs, fig_dir, missing, bin_sec):
    """G3/P1-M — Queue length: mean ± 1σ per protocol (replicates merged)."""
    groups = _group_by_protocol(runs)
    colors = protocol_color_map(list(groups.keys()))
    fig, ax = plt.subplots(figsize=(12, 5))
    any_data = False

    for proto, run_list in groups.items():
        series = []
        t_max = 0.0
        for run in run_list:
            df = _load(run, "charge_queue_timeseries", missing)
            if df is None or "t_rel_s" not in df.columns:
                continue
            role_cols = [c for c in ("queue_length_ugv", "queue_length_ch", "queue_length_member")
                         if c in df.columns]
            if not role_cols:
                continue
            df = df.sort_values("t_rel_s")
            t = df["t_rel_s"].values
            y = df[role_cols].clip(lower=0).sum(axis=1).values
            t_max = max(t_max, t.max())
            series.append((t, y))
        if not series:
            continue
        t_grid = np.arange(0, t_max + bin_sec, bin_sec)
        interpolated = np.array([_ts_to_grid(t, y, t_grid) for t, y in series])
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", RuntimeWarning)
            mean = np.nanmean(interpolated, axis=0)
            std  = np.nan_to_num(np.nanstd(interpolated, axis=0), nan=0.0)
        valid = ~np.isnan(mean)
        ax.plot(t_grid[valid] / 60, mean[valid],
                color=colors[proto], linewidth=1.8, label=proto)
        any_data = True

    if not any_data:
        missing.append("PLOT 03_charge_queue_length_merged: no charge_queue_timeseries data")
        plt.close(fig)
        return

    ax.set_xlabel("Experiment time (min)")
    ax.set_ylabel("Total charge queue length  (mean across replicates)")
    ax.set_title("Charge Queue Length — Merged Replicates")
    deduplicate_legend(ax)
    savefig(fig, fig_dir, "03_charge_queue_length_merged")
    print("  [OK] 03_charge_queue_length_merged")


def _plot_03_dock_utilization_merged(runs, fig_dir, missing, bin_sec):
    """G3/P2-M — Dock utilisation: mean ± 1σ per protocol (replicates merged)."""
    groups = _group_by_protocol(runs)
    colors = protocol_color_map(list(groups.keys()))
    fig, ax = plt.subplots(figsize=(12, 5))
    any_data = False

    for proto, run_list in groups.items():
        t_grid, mean, std = _merged_ts(
            run_list, "charge_queue_timeseries", "ugv_dock_utilization", missing, bin_sec
        )
        if t_grid is None:
            continue
        valid = ~np.isnan(mean)
        ax.plot(t_grid[valid] / 60, mean[valid],
                color=colors[proto], linewidth=1.8, label=proto)
        any_data = True

    if not any_data:
        missing.append("PLOT 03_dock_utilization_merged: no ugv_dock_utilization data")
        plt.close(fig)
        return

    ax.set_xlabel("Experiment time (min)")
    ax.set_ylabel("Dock utilisation  (mean across replicates)")
    ax.set_title("UGV Dock Utilisation — Merged Replicates")
    ax.set_ylim(-0.05, 1.05)
    deduplicate_legend(ax)
    savefig(fig, fig_dir, "03_dock_utilization_merged")
    print("  [OK] 03_dock_utilization_merged")


def _plot_03_charge_outcome_breakdown_merged(runs, fig_dir, missing):
    """G3/P3-M — Stacked bar of outcomes per protocol (counts summed across replicates).

    PREEMPTED is suppressed for protocols without ``_p_`` in their name.
    The PREEMPTED legend entry appears only when at least one protocol is
    preemptive, keeping non-preemptive bars clean.
    """
    ALL_OUTCOMES = ["STARTED", "REJECTED", "DROPPED", "TIMEOUT", "PREEMPTED", "ENERGY_DEPLETED"]
    PALETTE = {
        "STARTED":         "#2ca02c",
        "REJECTED":        "#d62728",
        "DROPPED":         "#ff7f0e",
        "TIMEOUT":         "#9467bd",
        "PREEMPTED":       "#8c564b",
        "ENERGY_DEPLETED": "#1f77b4",
    }

    groups = _group_by_protocol(runs)
    rows = []
    any_preemptive = False
    for proto, run_list in groups.items():
        is_pre = _is_preemptive(proto)
        if is_pre:
            any_preemptive = True
        row = {o: 0 for o in ALL_OUTCOMES}
        row["label"] = proto
        has_data = False
        for run in run_list:
            df = _load(run, "charge_events", missing)
            if df is None or "outcome" not in df.columns:
                continue
            counts = df["outcome"].value_counts()
            for o in ALL_OUTCOMES:
                val = int(counts.get(o, 0))
                row[o] += val if (o != "PREEMPTED" or is_pre) else 0
            has_data = True
        if has_data:
            rows.append(row)

    if not rows:
        missing.append("PLOT 03_charge_outcome_breakdown_merged: no outcome data")
        return

    outcomes_to_draw = [o for o in ALL_OUTCOMES if o != "PREEMPTED" or any_preemptive]
    summary_df = pd.DataFrame(rows).set_index("label")
    labels = summary_df.index.tolist()
    fig, ax = plt.subplots(figsize=(max(8, len(labels) * 1.5), 6))
    bottom = np.zeros(len(labels))
    for outcome in outcomes_to_draw:
        if outcome not in summary_df.columns:
            continue
        vals = summary_df[outcome].values
        ax.bar(labels, vals, bottom=bottom, color=PALETTE[outcome],
               label=outcome, edgecolor="white", linewidth=0.3)
        bottom += vals

    ax.set_ylabel("Number of charge requests  (all replicates summed)")
    ax.set_title("Charge Request Outcomes — Merged Replicates")
    ax.set_xticks(np.arange(len(labels)))
    ax.set_xticklabels(labels, rotation=20, ha="right")
    ax.legend(loc="upper right", fontsize=8)
    savefig(fig, fig_dir, "03_charge_outcome_breakdown_merged")
    print("  [OK] 03_charge_outcome_breakdown_merged")


def _plot_03_dead_uav_cumulative_merged(runs, fig_dir, missing, bin_sec):
    """G3/P4-M — Cumulative deaths: mean ± 1σ per protocol (replicates merged)."""
    groups = _group_by_protocol(runs)
    colors = protocol_color_map(list(groups.keys()))
    fig, ax = plt.subplots(figsize=(12, 5))
    any_data = False

    for proto, run_list in groups.items():
        series = []
        t_max = 0.0
        for run in run_list:
            df = _load(run, "death_events", missing)
            if df is not None and "t_rel_s" in df.columns and len(df) > 0:
                df = df.sort_values("t_rel_s")
                t = df["t_rel_s"].values
                y = np.arange(1, len(df) + 1, dtype=float)
                t_max = max(t_max, t[-1])
                series.append((t, y))
            else:
                dfq = _load(run, "charge_queue_timeseries", missing)
                if (dfq is not None and "dead_event_count" in dfq.columns
                        and "t_rel_s" in dfq.columns):
                    dfq = dfq.sort_values("t_rel_s")
                    t = dfq["t_rel_s"].values
                    y = dfq["dead_event_count"].values.astype(float)
                    t_max = max(t_max, t.max())
                    series.append((t, y))

        if not series:
            continue

        t_grid = np.arange(0, t_max + bin_sec, bin_sec)
        # Cumulative step series → forward-fill via searchsorted
        interped = []
        for t, y in series:
            idx = np.searchsorted(t, t_grid, side="right")
            y_ext = np.where(idx == 0, 0.0, y[np.minimum(idx - 1, len(y) - 1)])
            interped.append(y_ext)
        arr = np.array(interped, dtype=float)
        mean = np.mean(arr, axis=0)
        std  = np.std(arr, axis=0)

        ax.plot(t_grid / 60, mean, color=colors[proto], linewidth=1.8, label=proto)
        any_data = True

    if not any_data:
        missing.append("PLOT 03_dead_uav_cumulative_merged: no death data")
        plt.close(fig)
        return

    ax.set_xlabel("Experiment time (min)")
    ax.set_ylabel("Cumulative dead UAVs  (mean across replicates)")
    ax.set_title("Cumulative UAV Deaths — Merged Replicates")
    deduplicate_legend(ax)
    savefig(fig, fig_dir, "03_dead_uav_cumulative_merged")
    print("  [OK] 03_dead_uav_cumulative_merged")


def _plot_03_cumulative_energy_charged_merged(runs, fig_dir, missing, bin_sec):
    """G3/P5-M — Cumulative energy charged: mean per protocol (replicates merged)."""
    groups = _group_by_protocol(runs)
    colors = protocol_color_map(list(groups.keys()))
    fig, ax = plt.subplots(figsize=(12, 5))
    any_data = False
    unit_label = "Wh"

    for proto, run_list in groups.items():
        series = []
        t_max = 0.0
        for run in run_list:
            t, y, unit = _cumulative_energy_series(run, missing)
            if t is None or len(t) == 0:
                continue
            unit_label = unit
            t_max = max(t_max, float(t[-1]))
            series.append((t, y))

        if not series:
            continue

        t_grid = np.arange(0, t_max + bin_sec / 60.0, bin_sec / 60.0)
        interped = []
        for t, y in series:
            idx = np.searchsorted(t, t_grid, side="right")
            # forward-fill: value is last cumulative before or at grid point
            y_ff = np.where(idx == 0, 0.0, y[np.minimum(idx - 1, len(y) - 1)])
            interped.append(y_ff)
        arr = np.array(interped, dtype=float)
        mean = np.mean(arr, axis=0)

        ax.plot(t_grid, mean, color=colors[proto], linewidth=1.8, label=proto)
        any_data = True

    if not any_data:
        missing.append("PLOT 03_cumulative_energy_charged_merged: no energy data")
        plt.close(fig)
        return

    ax.set_xlabel("Experiment time (min)")
    ax.set_ylabel(f"Cumulative energy charged ({unit_label})  [mean across replicates]")
    ax.set_title("Cumulative Energy Charged — Merged Replicates")
    deduplicate_legend(ax)
    savefig(fig, fig_dir, "03_cumulative_energy_charged_merged")
    print("  [OK] 03_cumulative_energy_charged_merged")


# --- Group 04 merged --------------------------------------------------------

def _plot_04_policy_radar_merged(runs, fig_dir, missing):
    """G4/P1-M — Radar chart: KPIs averaged across replicates, one polygon per protocol."""
    AXES = [
        ("pdr",      "PDR",           1.0,     True),
        ("success",  "Success rate",  1.0,     True),
        ("latency",  "Low latency",   100_000, False),
        ("energy",   "Energy recov.", 100.0,   True),
        ("survival", "Survival",      1.0,     True),
    ]

    def _norm(val, max_val, higher_is_better):
        if np.isnan(val):
            return 0.0
        n = float(val) / max_val
        if not higher_is_better:
            n = 1.0 - min(n, 1.0)
        return float(np.clip(n, 0.0, 1.0))

    groups = _group_by_protocol(runs)
    protocol_scores: dict = {}

    for proto, run_list in groups.items():
        replicate_scores = []
        for run in run_list:
            summary = _load_summary(run)
            if summary is not None:
                net = summary.get("network", {})
                by_cat = net.get("by_category", [])
                total_gen = sum(c.get("generated", 0) for c in by_cat)
                total_del = sum(c.get("delivered", 0) for c in by_cat)
                pdr = total_del / total_gen if total_gen > 0 else float("nan")
                ch = summary.get("charging", {})
                success  = ch.get("success_rate", float("nan"))
                latency  = ch.get("decision_latency_ms", {}).get("mean", float("nan"))
                energy   = ch.get("energy_recovered", {}).get("mean", float("nan"))
                survival = summary.get("fleet", {}).get("survival_rate", float("nan"))
            else:
                pdr      = _overall_pdr_from_csvs(run, missing)
                success  = _success_rate_from_csvs(run, missing)
                latency  = _mean_latency_from_csvs(run, missing)
                energy   = _mean_energy_from_csvs(run, missing)
                survival = float("nan")

            raw = [pdr, success, latency, energy, survival]
            replicate_scores.append(
                [_norm(v, AXES[i][2], AXES[i][3]) for i, v in enumerate(raw)]
            )

        if not replicate_scores:
            continue
        protocol_scores[proto] = list(np.nanmean(replicate_scores, axis=0))

    if not protocol_scores:
        missing.append("PLOT 04_policy_radar_merged: no summary data available")
        return

    axis_labels = [a[1] for a in AXES]
    N = len(axis_labels)
    angles = np.linspace(0, 2 * np.pi, N, endpoint=False).tolist()
    angles += angles[:1]

    fig, ax = plt.subplots(figsize=(7, 7), subplot_kw={"polar": True})
    colors = protocol_color_map(list(protocol_scores.keys()))

    for label, scores in protocol_scores.items():
        vals = scores + scores[:1]
        ax.plot(angles, vals, linewidth=1.8, label=label, color=colors[label])
        ax.fill(angles, vals, alpha=0.10, color=colors[label])

    ax.set_xticks(angles[:-1])
    ax.set_xticklabels(axis_labels, fontsize=9)
    ax.set_ylim(0, 1)
    ax.set_yticks([0.25, 0.5, 0.75, 1.0])
    ax.set_yticklabels(["0.25", "0.5", "0.75", "1.0"], fontsize=7)
    ax.set_title("Policy Radar — Merged Replicates", pad=20)
    ax.legend(loc="upper right", bbox_to_anchor=(1.35, 1.1), fontsize=8)
    savefig(fig, fig_dir, "04_policy_radar_merged")
    print("  [OK] 04_policy_radar_merged")


# --- Group 05 merged --------------------------------------------------------

def _plot_05_pdr_vs_weather_regime_merged(runs, fig_dir, missing):
    """G5/P1-M — Grouped bar: mean PDR per (protocol, regime), replicates pooled."""
    groups = _group_by_protocol(runs)
    # proto → regime → [pdr values pooled across replicates]
    proto_regime_pdr: dict = {}

    for proto, run_list in groups.items():
        for run in run_list:
            net_df = _load(run, "network_timeseries", missing)
            w_df   = _load(run, "weather_timeseries", missing)
            if net_df is None or w_df is None:
                continue
            if "window_pdr" not in net_df.columns or "t_rel_s" not in net_df.columns:
                continue
            if "regime" not in w_df.columns or "t_rel_s" not in w_df.columns:
                continue
            net_df = net_df[net_df["window_pdr"] >= 0].copy()
            merged = merge_asof_weather(
                net_df, w_df, time_col="t_rel_s", weather_time_col="t_rel_s"
            )
            if "regime" not in merged.columns:
                continue
            for regime, grp in merged.groupby("regime"):
                (proto_regime_pdr
                 .setdefault(proto, {})
                 .setdefault(str(regime), [])
                 .extend(grp["window_pdr"].dropna().tolist()))

    if not proto_regime_pdr:
        missing.append("PLOT 05_pdr_vs_weather_regime_merged: no merged PDR+weather data")
        return

    all_regimes = sorted({r for p in proto_regime_pdr.values() for r in p})
    protocols   = list(proto_regime_pdr.keys())
    colors      = protocol_color_map(protocols)

    n_proto  = len(protocols)
    n_regime = len(all_regimes)
    bar_w    = 0.8 / n_proto

    fig, ax = plt.subplots(figsize=(max(8, n_regime * n_proto * 0.8), 5))
    for pi, proto in enumerate(protocols):
        regime_data = proto_regime_pdr.get(proto, {})
        offsets = np.arange(n_regime) + (pi - (n_proto - 1) / 2.0) * bar_w
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", RuntimeWarning)
            means = [float(np.nanmean(regime_data.get(r, [float("nan")]))) for r in all_regimes]
            stds  = [float(np.nan_to_num(np.nanstd(regime_data.get(r, [0.0])), nan=0.0))
                     for r in all_regimes]
        ax.bar(offsets, means, width=bar_w * 0.9, color=colors[proto], label=proto,
               edgecolor="white", linewidth=0.3)

    ax.set_xticks(np.arange(n_regime))
    ax.set_xticklabels(all_regimes, rotation=15, ha="right")
    ax.set_ylabel("Mean window PDR")
    ax.set_title("Network PDR vs Weather Regime — Merged Replicates")
    ax.set_ylim(0, 1.15)
    ax.legend(fontsize=8, loc="upper right")
    savefig(fig, fig_dir, "05_pdr_vs_weather_regime_merged")
    print("  [OK] 05_pdr_vs_weather_regime_merged")


# ===========================================================================
# Group 06 — Network QoS & End-to-End Delay
# ===========================================================================

def _qos_category_label(flow_type, control_type) -> str:
    """Human-readable category label for a (flow_type, control_type) pair.

    control_type takes priority: if non-empty it is the label regardless of
    flow_type.  This ensures e.g. TELEMETRY packets with flow_type=0 but
    control_type='TELEMETRY' are not incorrectly collapsed into 'DATA'.
    """
    ct = str(control_type).strip()
    if ct:
        return ct
    ft = str(flow_type).strip()
    if ft == "0" or ft.upper() == "DATA":
        return "DATA"
    return "CONTROL"


def _load_e2e_delays(run: dict, missing: list) -> "pd.DataFrame | None":
    """Return a DataFrame with at least e2e_delay_ms (ms) and t_rel_s columns.

    Priority:
    1. Join packet_generated_events.csv + packet_delivered_events.csv on msg_id:
       e2e_delay_ms = (delivered_time_s − creation_time_s) × 1000.
       Also preserves flow_type, control_type for per-category analysis.
    2. messages.csv  e2e_delay_ms  column (fallback for old-schema data).

    Returns None when no source yields data.
    """
    # --- primary: packet event join (matches lineage doc §5.1/§5.2) ---
    gen = _load(run, "packet_generated_events", [])
    dlv = _load(run, "packet_delivered_events", [])
    if (gen is not None and dlv is not None
            and "msg_id" in gen.columns and "creation_time_s" in gen.columns
            and "msg_id" in dlv.columns and "delivered_time_s" in dlv.columns):
        keep_gen = [c for c in
                    ["msg_id", "creation_time_s", "flow_type", "control_type", "t_rel_s"]
                    if c in gen.columns]
        merged = dlv[["msg_id", "delivered_time_s"]].merge(
            gen[keep_gen], on="msg_id", how="inner"
        )
        merged["e2e_delay_ms"] = (
            (merged["delivered_time_s"] - merged["creation_time_s"]) * 1000.0
        )
        merged = merged[merged["e2e_delay_ms"] >= 0].copy()
        if not merged.empty:
            return merged

    # --- fallback: messages.csv ---
    df = _load(run, "messages", missing)
    if df is not None and "e2e_delay_ms" in df.columns:
        if "delivered" in df.columns:
            df = df[df["delivered"] == "true"].copy()
        df = df[df["e2e_delay_ms"] >= 0].copy()
        if not df.empty:
            return df

    return None


def _pdr_by_category_from_events(run: dict) -> "dict | None":
    """Compute PDR per message category from raw packet event files.

    Joins packet_generated_events.csv (all packets) with
    packet_delivered_events.csv (delivered subset) on msg_id to derive
    per-(flow_type, control_type) delivery rates.  This is the only source
    that includes telemetry packets, which are tracked internally by the ROS
    node but never written to qos_metrics.csv (known issue §13 #7).

    Returns dict mapping category_label → pdr, or None if data unavailable.
    """
    gen = _load(run, "packet_generated_events", [])
    dlv = _load(run, "packet_delivered_events", [])
    if gen is None or dlv is None or "msg_id" not in gen.columns:
        return None

    key_cols = [c for c in ("flow_type", "control_type") if c in gen.columns]
    if not key_cols:
        return None

    gen_counts = gen.groupby(key_cols)["msg_id"].count().reset_index(name="generated")

    if "msg_id" not in dlv.columns:
        return None
    dlv_typed = dlv[["msg_id"]].merge(gen[["msg_id"] + key_cols], on="msg_id", how="inner")
    dlv_counts = dlv_typed.groupby(key_cols)["msg_id"].count().reset_index(name="delivered")

    combined = gen_counts.merge(dlv_counts, on=key_cols, how="left")
    combined["delivered"] = combined["delivered"].fillna(0)
    combined["pdr"] = combined["delivered"] / combined["generated"].clip(lower=1)

    result = {}
    for _, row in combined.iterrows():
        cat = _qos_category_label(
            row.get("flow_type", ""), row.get("control_type", "")
        )
        result[cat] = float(row["pdr"])
    return result or None


def _last_qos_per_category(df: "pd.DataFrame") -> "pd.DataFrame":
    """Return the last cumulative snapshot row per (flow_type, control_type).

    qos_metrics.csv is a snapshot timeseries; every flush appends one row per
    known category.  The final row contains the most-complete cumulative stats.
    """
    key_cols = [c for c in ("flow_type", "control_type") if c in df.columns]
    if not key_cols:
        return df
    return df.groupby(key_cols, as_index=False).last()


def _plot_06_qos_heatmap(runs, fig_dir, missing):
    """G6/P1 — Heatmap: PDR per message category × protocol.

    Data source priority:
    1. packet_generated_events.csv + packet_delivered_events.csv join — covers
       ALL packet types including telemetry (qos_metrics.csv silently discards
       telemetry, see lineage doc known issue §13 #7).
    2. qos_metrics.csv last-snapshot fallback if packet events are absent.
    """
    # proto → category → [pdr values from individual runs]
    data: dict = {}
    for run in runs:
        proto = run["protocol"]
        # Try packet events first (includes telemetry)
        cat_pdr = _pdr_by_category_from_events(run)
        if cat_pdr is None:
            # Fallback: qos_metrics.csv (misses telemetry but better than nothing)
            df = _load(run, "qos_metrics", missing)
            if df is None or "pdr" not in df.columns:
                continue
            df = _last_qos_per_category(df)
            cat_pdr = {}
            for _, row in df.iterrows():
                cat = _qos_category_label(
                    row.get("flow_type", ""), row.get("control_type", "")
                )
                cat_pdr[cat] = float(row["pdr"])
        data.setdefault(proto, {})
        for cat, pdr in cat_pdr.items():
            data[proto].setdefault(cat, []).append(pdr)

    if not data:
        missing.append("PLOT 06_qos_heatmap: no qos_metrics or packet event data")
        return

    protocols = list(data.keys())
    all_cats = sorted({c for p in data.values() for c in p})
    # Build matrix: rows = categories, cols = protocols
    matrix = np.full((len(all_cats), len(protocols)), float("nan"))
    for pi, proto in enumerate(protocols):
        for ci, cat in enumerate(all_cats):
            vals = data[proto].get(cat, [])
            if vals:
                matrix[ci, pi] = float(np.nanmean(vals))

    fig, ax = plt.subplots(figsize=(max(6, len(protocols) * 1.2), max(4, len(all_cats) * 0.7)))
    im = ax.imshow(matrix, aspect="auto", cmap="RdYlGn", vmin=0, vmax=1,
                   interpolation="nearest")
    cbar = fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)
    cbar.set_label("PDR", fontsize=9)

    ax.set_xticks(range(len(protocols)))
    ax.set_xticklabels(protocols, rotation=25, ha="right", fontsize=8)
    ax.set_yticks(range(len(all_cats)))
    ax.set_yticklabels(all_cats, fontsize=8)

    # Annotate cells
    for ci in range(len(all_cats)):
        for pi in range(len(protocols)):
            v = matrix[ci, pi]
            if not np.isnan(v):
                ax.text(pi, ci, f"{v:.2f}", ha="center", va="center",
                        fontsize=7, color="black" if 0.25 < v < 0.85 else "white")

    ax.set_title("Network PDR by Message Category × Protocol")
    ax.set_xlabel("Protocol")
    ax.set_ylabel("Message category")
    fig.tight_layout()
    savefig(fig, fig_dir, "06_qos_heatmap")
    print("  [OK] 06_qos_heatmap")


def _plot_06_qos_heatmap_merged(runs, fig_dir, missing):
    """G6/P1-M — Heatmap: PDR per category × protocol, replicates averaged.

    Same data priority as _plot_06_qos_heatmap: packet events first (includes
    telemetry), qos_metrics.csv as fallback.
    """
    groups = _group_by_protocol(runs)
    # proto → category → [pdr values pooled across replicates]
    data: dict = {}
    for proto, run_list in groups.items():
        for run in run_list:
            cat_pdr = _pdr_by_category_from_events(run)
            if cat_pdr is None:
                df = _load(run, "qos_metrics", missing)
                if df is None or "pdr" not in df.columns:
                    continue
                df = _last_qos_per_category(df)
                cat_pdr = {}
                for _, row in df.iterrows():
                    cat = _qos_category_label(
                        row.get("flow_type", ""), row.get("control_type", "")
                    )
                    cat_pdr[cat] = float(row["pdr"])
            for cat, pdr in cat_pdr.items():
                data.setdefault(proto, {}).setdefault(cat, []).append(pdr)

    if not data:
        missing.append("PLOT 06_qos_heatmap_merged: no qos_metrics or packet event data")
        return

    protocols = list(data.keys())
    all_cats = sorted({c for p in data.values() for c in p})
    matrix = np.full((len(all_cats), len(protocols)), float("nan"))
    for pi, proto in enumerate(protocols):
        for ci, cat in enumerate(all_cats):
            vals = data[proto].get(cat, [])
            if vals:
                matrix[ci, pi] = float(np.nanmean(vals))

    fig, ax = plt.subplots(figsize=(max(6, len(protocols) * 1.2), max(4, len(all_cats) * 0.7)))
    im = ax.imshow(matrix, aspect="auto", cmap="RdYlGn", vmin=0, vmax=1,
                   interpolation="nearest")
    cbar = fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)
    cbar.set_label("PDR", fontsize=9)

    ax.set_xticks(range(len(protocols)))
    ax.set_xticklabels(protocols, rotation=25, ha="right", fontsize=8)
    ax.set_yticks(range(len(all_cats)))
    ax.set_yticklabels(all_cats, fontsize=8)

    for ci in range(len(all_cats)):
        for pi in range(len(protocols)):
            v = matrix[ci, pi]
            if not np.isnan(v):
                ax.text(pi, ci, f"{v:.2f}", ha="center", va="center",
                        fontsize=7, color="black" if 0.25 < v < 0.85 else "white")

    ax.set_title("Network PDR by Message Category × Protocol — Merged Replicates")
    ax.set_xlabel("Protocol")
    ax.set_ylabel("Message category")
    fig.tight_layout()
    savefig(fig, fig_dir, "06_qos_heatmap_merged")
    print("  [OK] 06_qos_heatmap_merged")


def _e2e_delay_vals_for_run(run: dict, missing: list) -> "list[float]":
    """Return a list of E2E delay values (ms) for one run.

    Source priority:
    1. network_timeseries.csv  window_delay_mean_ms  (≥ 0 rows) — computed by
       the ROS node from internal rclcpp::Time objects; immune to CSV timestamp
       precision / clock-synchronisation issues.
    2. packet_generated_events + packet_delivered_events join on msg_id —
       per-packet delay, used when network_timeseries is absent.
    3. messages.csv  e2e_delay_ms — old-schema fallback.
    """
    # --- primary: ROS-node-computed window means ---
    net_df = _load(run, "network_timeseries", [])
    if net_df is not None and "window_delay_mean_ms" in net_df.columns:
        vals = net_df["window_delay_mean_ms"]
        vals = vals[vals >= 0].dropna()
        if not vals.empty:
            return vals.tolist()

    # --- secondary: per-packet join ---
    pkt_df = _load_e2e_delays(run, [])
    if pkt_df is not None and not pkt_df.empty:
        return pkt_df["e2e_delay_ms"].tolist()

    # --- fallback: messages.csv ---
    msg_df = _load(run, "messages", missing)
    if msg_df is not None and "e2e_delay_ms" in msg_df.columns:
        if "delivered" in msg_df.columns:
            msg_df = msg_df[msg_df["delivered"] == "true"]
        vals = msg_df["e2e_delay_ms"]
        vals = vals[vals >= 0].dropna()
        if not vals.empty:
            return vals.tolist()

    return []


def _plot_06_e2e_delay(runs, fig_dir, missing):
    """G6/P2 — Boxplot: E2E delay distribution per protocol.

    Values are window mean delays from network_timeseries.csv (one point per
    10-second window), which is the ROS node's own authoritative computation.
    Falls back to per-packet delay from packet event join or messages.csv.
    """
    data: dict = {}
    for run in runs:
        vals = _e2e_delay_vals_for_run(run, missing)
        if vals:
            data[run["label"]] = vals

    if not data:
        missing.append("PLOT 06_e2e_delay: no e2e_delay_ms data")
        return

    boxplot_multi(
        data,
        "E2E delay (ms)",
        "End-to-End Message Delay per Protocol",
        fig_dir,
        "06_e2e_delay",
    )
    print("  [OK] 06_e2e_delay")


def _plot_06_e2e_delay_merged(runs, fig_dir, missing):
    """G6/P2-M — Boxplot: E2E delay, replicates pooled per protocol."""
    groups = _group_by_protocol(runs)
    data: dict = {}
    for proto, run_list in groups.items():
        all_vals = []
        for run in run_list:
            all_vals.extend(_e2e_delay_vals_for_run(run, missing))
        if all_vals:
            data[proto] = all_vals

    if not data:
        missing.append("PLOT 06_e2e_delay_merged: no e2e_delay_ms data")
        return

    boxplot_multi(
        data,
        "E2E delay (ms)",
        "End-to-End Message Delay — Merged Replicates",
        fig_dir,
        "06_e2e_delay_merged",
    )
    print("  [OK] 06_e2e_delay_merged")


def _plot_06_mean_e2e_delay_merged(runs, fig_dir, missing):
    """G6/P3-M — Bar chart: mean E2E delay per protocol (mean ± std across replicates)."""
    groups = _group_by_protocol(runs)
    colors = protocol_color_map(list(groups.keys()))

    proto_labels, means, errs = [], [], []

    for proto, run_list in groups.items():
        rep_means = []
        for run in run_list:
            vals = _e2e_delay_vals_for_run(run, missing)
            if vals:
                rep_means.append(float(np.mean(vals)))
        if not rep_means:
            continue
        proto_labels.append(proto)
        means.append(float(np.mean(rep_means)))
        errs.append(float(np.std(rep_means)) if len(rep_means) > 1 else 0.0)

    if not proto_labels:
        missing.append("PLOT 06_mean_e2e_delay_merged: no e2e delay data")
        return

    bar_colors = [colors[p] for p in proto_labels]
    fig, ax = plt.subplots(figsize=(max(6, len(proto_labels) * 1.5), 5))
    x = np.arange(len(proto_labels))
    bars = ax.bar(x, means, yerr=errs, capsize=5,
                  color=bar_colors, edgecolor="white", linewidth=0.5,
                  error_kw={"elinewidth": 1.4, "ecolor": "black"})
    label_bars(ax, bars, fmt="{:.1f}")
    ax.set_ylabel("Mean E2E delay (ms)  (mean across replicates ± std)")
    ax.set_title("Mean End-to-End Delay — Merged Replicates")
    ax.set_xticks(x)
    ax.set_xticklabels(proto_labels, rotation=25, ha="right")
    fig.tight_layout()
    savefig(fig, fig_dir, "06_mean_e2e_delay_merged")
    print("  [OK] 06_mean_e2e_delay_merged")


# ---------------------------------------------------------------------------
# Cross-metric KPI builder (used by correlation heatmaps)
# ---------------------------------------------------------------------------

def _build_kpi_df(runs: list, missing: list) -> "pd.DataFrame | None":
    """Return a DataFrame with one row per run and the following columns:

    protocol, mean_pdr, mean_e2e_delay_ms, total_deaths,
    mean_queue_length, mean_decision_latency_ms, charge_success_rate,
    mean_effective_wait_ms, mean_energy_recovered_wh
    """
    rows = []
    for run in runs:
        row: dict = {"protocol": run["protocol"], "label": run["label"]}

        # mean PDR
        df_qos = _load(run, "qos_metrics", [])
        if df_qos is not None and "generated" in df_qos.columns and "delivered" in df_qos.columns:
            valid = df_qos[(df_qos["pdr"] >= 0) & (df_qos["generated"] > 0)]
            g = valid["generated"].sum()
            row["mean_pdr"] = (valid["delivered"].sum() / g) if g > 0 else float("nan")
        else:
            row["mean_pdr"] = float("nan")

        # mean E2E delay
        delay_vals = _e2e_delay_vals_for_run(run, [])
        row["mean_e2e_delay_ms"] = float(np.mean(delay_vals)) if delay_vals else float("nan")

        # total deaths
        df_death = _load(run, "death_events", [])
        row["total_deaths"] = len(df_death) if df_death is not None else float("nan")

        # mean waiting queue length
        df_q = _load(run, "charge_queue_timeseries", [])
        if df_q is not None and "queue_length_total" in df_q.columns:
            vals = df_q["queue_length_total"].replace(-1, float("nan")).dropna()
            row["mean_queue_length"] = float(vals.mean()) if len(vals) > 0 else float("nan")
        else:
            row["mean_queue_length"] = float("nan")

        # charging metrics from charge_events
        df_ce = _load(run, "charge_events", [])
        if df_ce is not None:
            # success rate
            if "outcome" in df_ce.columns:
                total = len(df_ce)
                row["charge_success_rate"] = (
                    int((df_ce["outcome"] == "STARTED").sum()) / total
                    if total > 0 else float("nan")
                )
            else:
                row["charge_success_rate"] = float("nan")

            # decision latency
            if "decision_latency_ms" in df_ce.columns:
                vals = df_ce["decision_latency_ms"].replace(-1, float("nan")).dropna()
                row["mean_decision_latency_ms"] = float(vals.mean()) if len(vals) > 0 else float("nan")
            else:
                row["mean_decision_latency_ms"] = float("nan")

            # effective wait
            if "effective_wait_ms" in df_ce.columns:
                vals = df_ce["effective_wait_ms"].replace(-1, float("nan")).dropna()
                row["mean_effective_wait_ms"] = float(vals.mean()) if len(vals) > 0 else float("nan")
            else:
                row["mean_effective_wait_ms"] = float("nan")
        else:
            row["charge_success_rate"] = float("nan")
            row["mean_decision_latency_ms"] = float("nan")
            row["mean_effective_wait_ms"] = float("nan")

        # total energy recovered (Wh)
        energy_vals = _energy_recovered_series(run, [])
        row["mean_energy_recovered_wh"] = (
            float(np.nansum(energy_vals)) if energy_vals is not None and len(energy_vals) > 0
            else float("nan")
        )

        rows.append(row)

    if not rows:
        return None
    return pd.DataFrame(rows)


def _draw_corr_heatmap(corr: "np.ndarray", labels: list, title: str,
                       fig_dir: Path, fname: str) -> None:
    """Draw a symmetric correlation heatmap with annotated cells."""
    n = len(labels)
    fig, ax = plt.subplots(figsize=(max(7, n * 0.9), max(6, n * 0.8)))
    im = ax.imshow(corr, vmin=-1, vmax=1, cmap="RdYlGn", aspect="auto")
    plt.colorbar(im, ax=ax, fraction=0.046, pad=0.04, label="Pearson r")

    ax.set_xticks(range(n))
    ax.set_yticks(range(n))
    ax.set_xticklabels(labels, rotation=40, ha="right", fontsize=8)
    ax.set_yticklabels(labels, fontsize=8)

    for i in range(n):
        for j in range(n):
            val = corr[i, j]
            if not np.isnan(val):
                txt_color = "black" if abs(val) < 0.6 else "white"
                ax.text(j, i, f"{val:.2f}", ha="center", va="center",
                        fontsize=7, color=txt_color)

    ax.set_title(title, pad=12)
    fig.tight_layout()
    savefig(fig, fig_dir, fname)


def _plot_correlation_heatmap_pdr(runs, fig_dir, missing):
    """Correlation heatmap: mean PDR × charging KPIs (per-run scalars, all protocols)."""
    df = _build_kpi_df(runs, missing)
    if df is None:
        missing.append("PLOT corr_heatmap_pdr: could not build KPI dataframe")
        return

    metric_cols = [
        "mean_pdr", "total_deaths", "mean_queue_length",
        "mean_decision_latency_ms", "charge_success_rate",
        "mean_effective_wait_ms", "mean_energy_recovered_wh",
    ]
    col_labels = [
        "Mean PDR", "Total Deaths", "Mean Queue Length",
        "Decision Latency (ms)", "Success Rate",
        "Effective Wait (ms)", "Energy Recovered (Wh)",
    ]

    sub = df[metric_cols].apply(pd.to_numeric, errors="coerce").dropna()
    if len(sub) < 3:
        missing.append("PLOT corr_heatmap_pdr: too few complete rows for correlation")
        return

    corr = np.corrcoef(sub.values.T)
    _draw_corr_heatmap(
        corr, col_labels,
        "Pearson Correlation — Mean PDR & Charging KPIs (per run)",
        fig_dir, "06_corr_heatmap_pdr",
    )
    print("  [OK] 06_corr_heatmap_pdr")


def _plot_correlation_heatmap_e2e(runs, fig_dir, missing):
    """Correlation heatmap: mean E2E delay × charging KPIs (per-run scalars)."""
    df = _build_kpi_df(runs, missing)
    if df is None:
        missing.append("PLOT corr_heatmap_e2e: could not build KPI dataframe")
        return

    metric_cols = [
        "mean_e2e_delay_ms", "total_deaths", "mean_queue_length",
        "mean_decision_latency_ms", "charge_success_rate",
        "mean_effective_wait_ms", "mean_energy_recovered_wh",
    ]
    col_labels = [
        "Mean E2E Delay (ms)", "Total Deaths", "Mean Queue Length",
        "Decision Latency (ms)", "Success Rate",
        "Effective Wait (ms)", "Energy Recovered (Wh)",
    ]

    sub = df[metric_cols].apply(pd.to_numeric, errors="coerce").dropna()
    if len(sub) < 3:
        missing.append("PLOT corr_heatmap_e2e: too few complete rows for correlation")
        return

    corr = np.corrcoef(sub.values.T)
    _draw_corr_heatmap(
        corr, col_labels,
        "Pearson Correlation — Mean E2E Delay & Charging KPIs (per run)",
        fig_dir, "06_corr_heatmap_e2e",
    )
    print("  [OK] 06_corr_heatmap_e2e")


def _plot_06_pdr_vs_e2e_scatter(runs, fig_dir, missing):
    """Scatter: mean PDR vs mean E2E delay, one point per run coloured by protocol.

    Pearson r and p-value are annotated.  A least-squares regression line is
    drawn across all points regardless of protocol.

    Data source: same scalars as _build_kpi_df (qos_metrics.csv for PDR,
    packet_events.csv / messages.csv for E2E delay).
    """
    from scipy import stats as _stats

    df = _build_kpi_df(runs, missing)
    if df is None:
        missing.append("PLOT 06_pdr_vs_e2e_scatter: could not build KPI dataframe")
        return

    sub = df[["protocol", "label", "mean_pdr", "mean_e2e_delay_ms"]].copy()
    sub = sub.apply(lambda c: pd.to_numeric(c, errors="ignore") if c.name not in ("protocol", "label") else c)
    sub = sub.dropna(subset=["mean_pdr", "mean_e2e_delay_ms"])

    if len(sub) < 3:
        missing.append("PLOT 06_pdr_vs_e2e_scatter: too few complete rows")
        return

    protocols = sorted(sub["protocol"].unique())
    cmap = plt.get_cmap("tab10")
    colors = {p: cmap(i % 10) for i, p in enumerate(protocols)}

    fig, ax = plt.subplots(figsize=(7, 5))

    for proto in protocols:
        mask = sub["protocol"] == proto
        ax.scatter(
            sub.loc[mask, "mean_e2e_delay_ms"],
            sub.loc[mask, "mean_pdr"],
            label=proto, color=colors[proto], s=60, zorder=3,
        )

    # regression line across all points
    x_all = sub["mean_e2e_delay_ms"].values.astype(float)
    y_all = sub["mean_pdr"].values.astype(float)
    slope, intercept, r_val, p_val, _ = _stats.linregress(x_all, y_all)
    x_line = np.array([x_all.min(), x_all.max()])
    ax.plot(x_line, slope * x_line + intercept, color="black",
            linewidth=1.2, linestyle="--", zorder=2)

    p_str = f"p = {p_val:.3f}" if p_val >= 0.001 else "p < 0.001"
    ax.annotate(
        f"Pearson r = {r_val:.3f}\n{p_str}",
        xy=(0.97, 0.97), xycoords="axes fraction",
        ha="right", va="top", fontsize=9,
        bbox=dict(boxstyle="round,pad=0.3", facecolor="white", edgecolor="grey", alpha=0.8),
    )

    ax.set_xlabel("Mean E2E Delay (ms)")
    ax.set_ylabel("Mean PDR")
    ax.set_title("PDR vs E2E Delay — per run, all protocols")
    ax.legend(title="Protocol", fontsize=7, title_fontsize=8,
               bbox_to_anchor=(1.01, 1), loc="upper left")
    fig.tight_layout()
    savefig(fig, fig_dir, "06_pdr_vs_e2e_scatter")
    print("  [OK] 06_pdr_vs_e2e_scatter")


def _plot_05_delay_vs_weather_regime(runs, fig_dir, missing):
    """G5/P3 — Boxplot: window mean E2E delay grouped by weather regime (all protocols).

    Source: network_timeseries.csv  window_delay_mean_ms — computed by the ROS
    node from internal rclcpp::Time objects, so it is immune to CSV timestamp
    precision or clock-synchronisation artefacts.  Pattern mirrors
    05_pdr_vs_weather_regime exactly (same file, same join, different column).
    """
    regime_delay: dict = {}

    for run in runs:
        net_df = _load(run, "network_timeseries", missing)
        w_df   = _load(run, "weather_timeseries", missing)
        if net_df is None or w_df is None:
            continue
        if "window_delay_mean_ms" not in net_df.columns or "t_rel_s" not in net_df.columns:
            continue
        if "regime" not in w_df.columns or "t_rel_s" not in w_df.columns:
            continue

        net_df = net_df[net_df["window_delay_mean_ms"] >= 0].copy()
        merged = merge_asof_weather(
            net_df, w_df, time_col="t_rel_s", weather_time_col="t_rel_s"
        )
        if "regime" not in merged.columns:
            continue
        for regime, grp in merged.groupby("regime"):
            regime_delay.setdefault(str(regime), []).extend(
                grp["window_delay_mean_ms"].dropna().tolist()
            )

    if not regime_delay:
        missing.append("PLOT 05_delay_vs_weather_regime: no merged delay+weather data")
        return

    regimes = sorted(regime_delay.keys())
    data = [regime_delay[r] for r in regimes]

    fig, ax = plt.subplots(figsize=(max(6, len(regimes) * 1.5), 5))
    bp = ax.boxplot(data, patch_artist=True, showfliers=False)
    for patch in bp["boxes"]:
        patch.set_facecolor("#e8812a")
        patch.set_alpha(0.65)
    ax.set_xticks(range(1, len(regimes) + 1))
    ax.set_xticklabels(regimes)
    ax.set_ylabel("Window mean E2E delay (ms)")
    ax.set_title("Message E2E Delay vs Weather Regime (all protocols)")
    savefig(fig, fig_dir, "05_delay_vs_weather_regime")
    print("  [OK] 05_delay_vs_weather_regime")


def _plot_05_delay_vs_weather_regime_merged(runs, fig_dir, missing):
    """G5/P3-M — Grouped bar: mean E2E delay per (protocol, weather regime), replicates pooled.

    Source: network_timeseries.csv  window_delay_mean_ms — same authoritative
    source as the non-merged variant; mirrors 05_pdr_vs_weather_regime_merged.
    """
    groups = _group_by_protocol(runs)
    proto_regime_delay: dict = {}

    for proto, run_list in groups.items():
        for run in run_list:
            net_df = _load(run, "network_timeseries", missing)
            w_df   = _load(run, "weather_timeseries", missing)
            if net_df is None or w_df is None:
                continue
            if "window_delay_mean_ms" not in net_df.columns or "t_rel_s" not in net_df.columns:
                continue
            if "regime" not in w_df.columns or "t_rel_s" not in w_df.columns:
                continue

            net_df = net_df[net_df["window_delay_mean_ms"] >= 0].copy()
            merged = merge_asof_weather(
                net_df, w_df, time_col="t_rel_s", weather_time_col="t_rel_s"
            )
            if "regime" not in merged.columns:
                continue
            for regime, grp in merged.groupby("regime"):
                (proto_regime_delay
                 .setdefault(proto, {})
                 .setdefault(str(regime), [])
                 .extend(grp["window_delay_mean_ms"].dropna().tolist()))

    if not proto_regime_delay:
        missing.append("PLOT 05_delay_vs_weather_regime_merged: no merged delay+weather data")
        return

    all_regimes = sorted({r for p in proto_regime_delay.values() for r in p})
    protocols   = list(proto_regime_delay.keys())
    colors      = protocol_color_map(protocols)

    n_proto  = len(protocols)
    n_regime = len(all_regimes)
    bar_w    = 0.8 / n_proto

    fig, ax = plt.subplots(figsize=(max(8, n_regime * n_proto * 0.8), 5))
    for pi, proto in enumerate(protocols):
        regime_data = proto_regime_delay.get(proto, {})
        offsets = np.arange(n_regime) + (pi - (n_proto - 1) / 2.0) * bar_w
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", RuntimeWarning)
            means = [
                float(np.nanmean(regime_data.get(r, [float("nan")])))
                for r in all_regimes
            ]
        ax.bar(offsets, means, width=bar_w * 0.9, color=colors[proto], label=proto,
               edgecolor="white", linewidth=0.3)

    ax.set_xticks(np.arange(n_regime))
    ax.set_xticklabels(all_regimes, rotation=15, ha="right")
    ax.set_ylabel("Mean window E2E delay (ms)")
    ax.set_title("Message E2E Delay vs Weather Regime — Merged Replicates")
    ax.legend(fontsize=8, loc="upper right")
    savefig(fig, fig_dir, "05_delay_vs_weather_regime_merged")
    print("  [OK] 05_delay_vs_weather_regime_merged")


# ===========================================================================
# GROUP 07 — Causal Analysis  (event overlays + lagged correlation)
# ===========================================================================

# ---------------------------------------------------------------------------
# 07 helpers
# ---------------------------------------------------------------------------

def _load_event_times_s(run: dict, missing: list) -> dict:
    """Return a dict with lists of t_rel_s values for key event types.

    Keys returned:
        all_deaths  – all UAV death events
        ch_deaths   – CH-only deaths (role == 1)
        ch_started  – charge STARTED events for CH UAVs only (role == 1)
        started     – charge STARTED events for all UAVs (kept for other uses)
    """
    result = {
        "all_deaths": [],
        "ch_deaths":  [],
        "ch_started": [],
        "started":    [],
        "timeouts":   [],
    }

    # Deaths
    de = _load(run, "death_events", missing)
    if de is not None and "t_rel_s" in de.columns:
        result["all_deaths"] = _clean_event_times(de["t_rel_s"].dropna().tolist())
        if "role" in de.columns:
            ch_mask = de["role"].astype(str).isin(["1", "CH"])
            result["ch_deaths"] = _clean_event_times(
                de.loc[ch_mask, "t_rel_s"].dropna().tolist())
        else:
            result["ch_deaths"] = []

    # Charge outcomes — use role column to isolate CH requests
    ce = _load(run, "charge_events", missing)
    if ce is not None and "outcome" in ce.columns and "t_rel_s" in ce.columns:
        started_mask = ce["outcome"] == "STARTED"
        result["started"]  = _clean_event_times(
            ce.loc[started_mask, "t_rel_s"].dropna().tolist())
        result["timeouts"] = _clean_event_times(
            ce.loc[ce["outcome"] == "TIMEOUT", "t_rel_s"].dropna().tolist())
        if "role" in ce.columns:
            ch_mask = ce["role"].astype(str).isin(["1", "CH"])
            result["ch_started"] = _clean_event_times(
                ce.loc[started_mask & ch_mask, "t_rel_s"].dropna().tolist())
        else:
            result["ch_started"] = result["started"]

    return result


def _pdr_dip_intervals(t_arr, pdr_arr, threshold=None):
    """Detect contiguous intervals where PDR falls below *threshold*.

    Parameters
    ----------
    t_arr, pdr_arr : 1-D array-like
        Time (seconds) and PDR values.
    threshold : float or None
        If None, use p10 of non-negative PDR values (adaptive).

    Returns
    -------
    intervals : list of (t_start, t_end) tuples
    threshold : float  (the value actually used)
    """
    t   = np.asarray(t_arr,   dtype=float)
    pdr = np.asarray(pdr_arr, dtype=float)

    valid = pdr[pdr >= 0]
    if len(valid) == 0:
        return [], float("nan")

    if threshold is None:
        threshold = float(np.nanpercentile(valid, 10))

    # Boolean mask of dip samples
    dip_mask = (pdr >= 0) & (pdr < threshold)

    intervals = []
    in_dip = False
    t_start = 0.0
    for i, flag in enumerate(dip_mask):
        if flag and not in_dip:
            t_start = float(t[i])
            in_dip = True
        elif not flag and in_dip:
            intervals.append((t_start, float(t[i - 1])))
            in_dip = False
    if in_dip:
        intervals.append((t_start, float(t[-1])))

    return intervals, threshold


def _bin_events_to_grid(event_times_s, t_grid_edges):
    """Count events per bin defined by *t_grid_edges* using np.histogram.

    Returns an array of length ``len(t_grid_edges) - 1``.
    """
    if not event_times_s:
        return np.zeros(len(t_grid_edges) - 1, dtype=float)
    counts, _ = np.histogram(event_times_s, bins=t_grid_edges)
    return counts.astype(float)


def _lagged_pearson(x, y, max_lag_bins: int):
    """Compute Pearson r between x and y at integer lags [-max_lag, +max_lag].

    Positive lag means y lags behind x (x leads).  Returns (lags, corrs)
    where lags is a 1-D integer array and corrs is float (nan when
    insufficient data).
    """
    x = np.asarray(x, dtype=float)
    y = np.asarray(y, dtype=float)
    lags = np.arange(-max_lag_bins, max_lag_bins + 1)
    corrs = np.full(len(lags), float("nan"))
    n = len(x)
    for idx, lag in enumerate(lags):
        if lag == 0:
            xi, yi = x, y
        elif lag > 0:
            xi = x[: n - lag]
            yi = y[lag:]
        else:
            xi = x[-lag:]
            yi = y[: n + lag]
        if len(xi) < 3:
            continue
        valid = ~(np.isnan(xi) | np.isnan(yi))
        if valid.sum() < 3:
            continue
        xi, yi = xi[valid], yi[valid]
        mx, my = xi.mean(), yi.mean()
        num = ((xi - mx) * (yi - my)).sum()
        den = np.sqrt(((xi - mx) ** 2).sum() * ((yi - my) ** 2).sum())
        corrs[idx] = float(num / den) if den > 0 else float("nan")
    return lags, corrs


def _rolling_slope(y, window: int = 5):
    """Estimate local slope of *y* via np.gradient, smoothed with a rolling mean."""
    y = np.asarray(y, dtype=float)
    grad = np.gradient(y)
    kernel = np.ones(window) / window
    return np.convolve(grad, kernel, mode="same")


def _clean_event_times(times_s, t_max_s=None):
    """Return a sorted list of valid event times (seconds).

    Removes: NaN, negative values, and values beyond *t_max_s* (if given).
    Duplicate times are kept intentionally (same-second events on different
    UAVs are real) but callers that merge across replicates should use
    _add_binned_event_rug instead of _add_event_vlines.
    """
    out = [float(t) for t in times_s
           if t == t and not (t != t) and float(t) >= 0.0]   # NaN-safe
    if t_max_s is not None:
        out = [t for t in out if t <= float(t_max_s)]
    return sorted(out)


def _add_binned_event_rug(ax, times_s, t_grid_edges, n_replicates,
                          color, label, y_base=0.01, bar_scale=0.07):
    """Draw a density rug at the bottom of *ax* for merged-replicate event plots.

    Instead of one vline per raw event (which stacks N near-identical lines
    for N replicates), this bins events into *t_grid_edges* and draws a tick
    at each bin centre whose height is proportional to the mean number of
    events per replicate per bin.  Bins with zero events are silent.

    Parameters
    ----------
    times_s       : iterable of float – event times in seconds (already cleaned)
    t_grid_edges  : 1-D array         – bin edges in seconds
    n_replicates  : int               – number of replicates (used to normalise)
    y_base        : float             – baseline y position (axes fraction of 0-1)
    bar_scale     : float             – max bar height in data units
    """
    if not len(times_s) or n_replicates < 1:
        return

    counts, _ = np.histogram(times_s, bins=t_grid_edges)
    mean_per_rep = counts / float(n_replicates)
    max_val = mean_per_rep.max()
    if max_val == 0:
        return

    bin_centers_min = ((t_grid_edges[:-1] + t_grid_edges[1:]) / 2.0) / 60.0
    heights = mean_per_rep / max_val * bar_scale     # normalised to bar_scale

    first = True
    for x, h in zip(bin_centers_min, heights):
        if h <= 0:
            continue
        ax.plot([x, x], [y_base, y_base + h],
                color=color, linewidth=1.4, alpha=0.75,
                solid_capstyle="butt",
                label=label if first else "_nolegend_")
        first = False


def _add_event_vlines(ax, events_s, color, label, lw=0.8, alpha=0.7,
                      linestyle="-", max_lines=200):
    """Add vertical line markers to *ax* for each event time (seconds → minutes).

    The first line carries the legend *label*; subsequent ones are unlabelled.
    Capped at *max_lines* to avoid over-cluttering.
    """
    times = [t / 60.0 for t in events_s[:max_lines]]
    for i, t_min in enumerate(times):
        ax.axvline(
            t_min,
            color=color,
            linewidth=lw,
            alpha=alpha,
            linestyle=linestyle,
            label=label if i == 0 else "_nolegend_",
        )


# ---------------------------------------------------------------------------
# 07-A1  PDR + event markers — per-protocol single figure
# ---------------------------------------------------------------------------

def _plot_07_pdr_with_events_per_protocol(runs, fig_dir, missing):
    """G7/A1 — Per-protocol: PDR timeseries + vertical event markers + dip shading."""
    groups = _group_by_protocol(runs)

    for proto, run_list in groups.items():
        fig, ax = plt.subplots(figsize=(14, 5))
        any_pdr = False
        dip_threshold = None

        for run in run_list:
            net_df = _load(run, "network_timeseries", missing)
            if net_df is None or "window_pdr" not in net_df.columns or "t_rel_s" not in net_df.columns:
                continue
            net_df = net_df[net_df["window_pdr"] >= 0].sort_values("t_rel_s")
            if net_df.empty:
                continue

            t_s   = net_df["t_rel_s"].values
            pdr   = net_df["window_pdr"].values

            ax.plot(t_s / 60.0, pdr, linewidth=1.3, alpha=0.8, label=run["label"])
            any_pdr = True

            intervals, thr = _pdr_dip_intervals(t_s, pdr)
            if dip_threshold is None and not np.isnan(thr):
                dip_threshold = thr
            for (ts, te) in intervals:
                ax.axvspan(ts / 60.0, te / 60.0,
                           color="gold", alpha=0.25, linewidth=0)

        if not any_pdr:
            plt.close(fig)
            continue

        all_events: dict = {k: [] for k in ("all_deaths", "ch_deaths", "ch_started")}
        for run in run_list:
            ev = _load_event_times_s(run, missing)
            for k in all_events:
                all_events[k].extend(ev[k])

        _add_event_vlines(ax, all_events["ch_deaths"],
                          "darkred",  "CH death",    lw=1.2, linestyle="-")
        _add_event_vlines(ax, all_events["all_deaths"],
                          "red",      "UAV death",   lw=0.7, linestyle="--", alpha=0.5)
        _add_event_vlines(ax, all_events["ch_started"],
                          "steelblue", "CH charging started", lw=0.8, linestyle="-.", alpha=0.6)

        ax.set_xlabel("Experiment time (min)")
        ax.set_ylabel("Window PDR")
        proto_safe = proto.replace("/", "_")
        ax.set_title(f"PDR + Events — {proto}")
        ax.set_ylim(-0.05, 1.05)
        if dip_threshold is not None:
            ax.axhline(dip_threshold, color="goldenrod", linestyle="--",
                       linewidth=0.8, label=f"Dip threshold ({dip_threshold:.2f})")
        deduplicate_legend(ax, fontsize=7)
        savefig(fig, fig_dir, f"07_pdr_events_{proto_safe}")
        print(f"  [OK] 07_pdr_events_{proto_safe}")


# ---------------------------------------------------------------------------
# 07-A1  PDR + event markers — all-protocols panel (shared x-axis)
# ---------------------------------------------------------------------------

def _plot_07_pdr_with_events_panel(runs, fig_dir, missing):
    """G7/A1-panel — All protocols in sub-panels, shared x-axis, event markers."""
    groups = _group_by_protocol(runs)
    protos = list(groups.keys())
    n = len(protos)
    if n == 0:
        return

    fig, axes = plt.subplots(n, 1, figsize=(14, 3 * n), sharex=True)
    if n == 1:
        axes = [axes]

    colors = protocol_color_map(protos)
    any_plot = False

    for ax, proto in zip(axes, protos):
        run_list = groups[proto]
        for run in run_list:
            net_df = _load(run, "network_timeseries", missing)
            if net_df is None or "window_pdr" not in net_df.columns or "t_rel_s" not in net_df.columns:
                continue
            net_df = net_df[net_df["window_pdr"] >= 0].sort_values("t_rel_s")
            if net_df.empty:
                continue
            t_s, pdr = net_df["t_rel_s"].values, net_df["window_pdr"].values
            ax.plot(t_s / 60.0, pdr, linewidth=1.2, alpha=0.8,
                    color=colors[proto], label=run["label"])
            intervals, _ = _pdr_dip_intervals(t_s, pdr)
            for (ts, te) in intervals:
                ax.axvspan(ts / 60.0, te / 60.0, color="gold", alpha=0.2, linewidth=0)
            any_plot = True

        all_events: dict = {k: [] for k in ("all_deaths", "ch_deaths", "ch_started")}
        for run in run_list:
            ev = _load_event_times_s(run, missing)
            for k in all_events:
                all_events[k].extend(ev[k])

        _add_event_vlines(ax, all_events["ch_deaths"],
                          "darkred",   "CH death",            lw=1.2, linestyle="-")
        _add_event_vlines(ax, all_events["all_deaths"],
                          "red",       "UAV death",           lw=0.7, linestyle="--", alpha=0.5)
        _add_event_vlines(ax, all_events["ch_started"],
                          "steelblue", "CH charging started", lw=0.8, linestyle="-.", alpha=0.6)

        ax.set_ylabel("PDR", fontsize=8)
        ax.set_ylim(-0.05, 1.05)
        ax.set_title(proto, fontsize=9, pad=2)
        deduplicate_legend(ax, fontsize=6)

    if not any_plot:
        plt.close(fig)
        missing.append("PLOT 07_pdr_events_all_protocols: no window_pdr data")
        return

    axes[-1].set_xlabel("Experiment time (min)")
    fig.suptitle("PDR + Event Markers — All Protocols", fontsize=11, y=1.01)
    fig.tight_layout()
    savefig(fig, fig_dir, "07_pdr_events_all_protocols")
    print("  [OK] 07_pdr_events_all_protocols")


# ---------------------------------------------------------------------------
# 07-A1-MP  PDR + event markers — merged replicates, ALL protocols panel
# ---------------------------------------------------------------------------

def _plot_07_pdr_with_events_merged_panel(runs, fig_dir, missing, bin_sec: float = 60.0):
    """G7/A1-MP — All protocols stacked in sub-panels; each panel shows the mean
    PDR averaged across replicates (same grid approach as the merged variant)
    plus pooled event markers.
    """
    groups = _group_by_protocol(runs)
    protos = list(groups.keys())
    n = len(protos)
    if n == 0:
        return

    fig, axes = plt.subplots(n, 1, figsize=(14, 3 * n), sharex=True)
    if n == 1:
        axes = [axes]

    colors = protocol_color_map(protos)
    any_plot = False

    for ax, proto in zip(axes, protos):
        run_list = groups[proto]

        # Build mean PDR on a common grid
        series = []
        t_max = 0.0
        for run in run_list:
            net_df = _load(run, "network_timeseries", missing)
            if net_df is None or "window_pdr" not in net_df.columns or "t_rel_s" not in net_df.columns:
                continue
            net_df = net_df[net_df["window_pdr"] >= 0].sort_values("t_rel_s")
            if net_df.empty:
                continue
            t_max = max(t_max, float(net_df["t_rel_s"].iloc[-1]))
            series.append((net_df["t_rel_s"].values, net_df["window_pdr"].values))

        if not series:
            continue

        t_grid = np.arange(0, t_max + bin_sec, bin_sec)
        interped = [_ts_to_grid(t, y, t_grid) for t, y in series]
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", RuntimeWarning)
            pdr_mean = np.nanmean(np.vstack(interped), axis=0)

        ax.plot(t_grid / 60.0, pdr_mean,
                color=colors[proto], linewidth=1.8, label=f"{proto} (mean)")

        intervals, dip_threshold = _pdr_dip_intervals(t_grid, pdr_mean)
        for (ts, te) in intervals:
            ax.axvspan(ts / 60.0, te / 60.0, color="gold", alpha=0.2, linewidth=0)

        # Pooled + cleaned event markers — binned across replicates
        n_rep = len(run_list)
        all_events: dict = {k: [] for k in ("all_deaths", "ch_deaths", "ch_started")}
        for run in run_list:
            ev = _load_event_times_s(run, missing)
            for k in all_events:
                all_events[k].extend(_clean_event_times(ev[k], t_max_s=t_max))

        t_edges = np.arange(0, t_max + bin_sec, bin_sec)

        _add_binned_event_rug(ax, all_events["ch_deaths"],  t_edges, n_rep,
                              "darkred",   "CH death (binned, mean/rep)")
        _add_binned_event_rug(ax, all_events["all_deaths"], t_edges, n_rep,
                              "red",       "UAV death (binned, mean/rep)")
        _add_binned_event_rug(ax, all_events["ch_started"], t_edges, n_rep,
                              "steelblue", "CH charging started (binned, mean/rep)")

        if not np.isnan(dip_threshold):
            ax.axhline(dip_threshold, color="goldenrod", linestyle="--",
                       linewidth=0.7, label=f"Dip thr. ({dip_threshold:.2f})")

        ax.set_ylabel("PDR (mean)", fontsize=8)
        ax.set_ylim(-0.05, 1.05)
        ax.set_title(proto, fontsize=9, pad=2)
        deduplicate_legend(ax, fontsize=6)
        any_plot = True

    if not any_plot:
        plt.close(fig)
        missing.append("PLOT 07_pdr_events_merged_all_protocols: no window_pdr data")
        return

    axes[-1].set_xlabel("Experiment time (min)")
    fig.suptitle("PDR + Event Markers — Merged Replicates — All Protocols", fontsize=11, y=1.01)
    fig.tight_layout()
    savefig(fig, fig_dir, "07_pdr_events_merged_all_protocols")
    print("  [OK] 07_pdr_events_merged_all_protocols")


# ---------------------------------------------------------------------------
# 07-A1-M  PDR + event markers — merged replicates, one figure per protocol
# ---------------------------------------------------------------------------

def _plot_07_pdr_with_events_merged(runs, fig_dir, missing, bin_sec: float = 60.0):
    """G7/A1-M — Per-protocol merged PDR (mean across replicates) + event markers.

    PDR timeseries: replicates are interpolated onto a common time grid and
    averaged (same approach as _plot_01_network_pdr_over_time_merged).
    Event markers and dip shading are derived from all replicates pooled.
    """
    groups = _group_by_protocol(runs)
    colors = protocol_color_map(list(groups.keys()))

    for proto, run_list in groups.items():
        # --- build mean PDR timeseries ----------------------------------------
        series = []
        t_max = 0.0
        for run in run_list:
            net_df = _load(run, "network_timeseries", missing)
            if net_df is None or "window_pdr" not in net_df.columns or "t_rel_s" not in net_df.columns:
                continue
            net_df = net_df[net_df["window_pdr"] >= 0].sort_values("t_rel_s")
            if net_df.empty:
                continue
            t_max = max(t_max, float(net_df["t_rel_s"].iloc[-1]))
            series.append((net_df["t_rel_s"].values, net_df["window_pdr"].values))

        if not series:
            continue

        t_grid = np.arange(0, t_max + bin_sec, bin_sec)
        interped = [_ts_to_grid(t, y, t_grid) for t, y in series]
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", RuntimeWarning)
            pdr_mean = np.nanmean(np.vstack(interped), axis=0)

        # --- dip intervals on the averaged curve --------------------------------
        intervals, dip_threshold = _pdr_dip_intervals(t_grid, pdr_mean)

        # --- pooled + cleaned event times ---------------------------------------
        n_rep = len(run_list)
        all_events: dict = {k: [] for k in ("all_deaths", "ch_deaths", "ch_started")}
        for run in run_list:
            ev = _load_event_times_s(run, missing)
            for k in all_events:
                all_events[k].extend(_clean_event_times(ev[k], t_max_s=t_max))

        t_edges = np.arange(0, t_max + bin_sec, bin_sec)

        # --- plot ---------------------------------------------------------------
        fig, ax = plt.subplots(figsize=(14, 5))
        ax.plot(t_grid / 60.0, pdr_mean,
                color=colors[proto], linewidth=1.8, label=f"{proto} (mean)")

        for (ts, te) in intervals:
            ax.axvspan(ts / 60.0, te / 60.0, color="gold", alpha=0.25, linewidth=0)

        _add_binned_event_rug(ax, all_events["ch_deaths"],  t_edges, n_rep,
                              "darkred",   "CH death (binned, mean/rep)")
        _add_binned_event_rug(ax, all_events["all_deaths"], t_edges, n_rep,
                              "red",       "UAV death (binned, mean/rep)")
        _add_binned_event_rug(ax, all_events["ch_started"], t_edges, n_rep,
                              "steelblue", "CH charging started (binned, mean/rep)")

        if not np.isnan(dip_threshold):
            ax.axhline(dip_threshold, color="goldenrod", linestyle="--",
                       linewidth=0.8, label=f"Dip threshold p10 = {dip_threshold:.2f}")

        ax.set_xlabel("Experiment time (min)")
        ax.set_ylabel("Window PDR (mean across replicates)")
        proto_safe = proto.replace("/", "_")
        ax.set_title(f"PDR + Events — {proto} — Merged Replicates")
        ax.set_ylim(-0.05, 1.05)
        deduplicate_legend(ax, fontsize=7)
        savefig(fig, fig_dir, f"07_pdr_events_merged_{proto_safe}")
        print(f"  [OK] 07_pdr_events_merged_{proto_safe}")


def _plot_07_pdr_with_events_merged_variance(runs, fig_dir, missing, bin_sec: float = 60.0):
    """G7/A1-MV — Same as _plot_07_pdr_with_events_merged but with ±1σ variance band.

    The shaded band shows the across-replicate standard deviation at every
    time bin so the reader can judge how consistent the PDR trajectory is.
    The dip threshold (p10 of the mean curve) and all event markers are kept.

    Dip threshold: 10th percentile of all non-negative window_pdr values on
    the mean curve.  A point is flagged as a "dip" when mean PDR < p10.
    """
    groups = _group_by_protocol(runs)
    colors = protocol_color_map(list(groups.keys()))

    for proto, run_list in groups.items():
        series = []
        t_max = 0.0
        for run in run_list:
            net_df = _load(run, "network_timeseries", missing)
            if net_df is None or "window_pdr" not in net_df.columns or "t_rel_s" not in net_df.columns:
                continue
            net_df = net_df[net_df["window_pdr"] >= 0].sort_values("t_rel_s")
            if net_df.empty:
                continue
            t_max = max(t_max, float(net_df["t_rel_s"].iloc[-1]))
            series.append((net_df["t_rel_s"].values, net_df["window_pdr"].values))

        if not series:
            continue

        t_grid  = np.arange(0, t_max + bin_sec, bin_sec)
        interped = [_ts_to_grid(t, y, t_grid) for t, y in series]
        stack    = np.vstack(interped)
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", RuntimeWarning)
            pdr_mean = np.nanmean(stack, axis=0)
            pdr_std  = np.nanstd(stack,  axis=0)

        intervals, dip_threshold = _pdr_dip_intervals(t_grid, pdr_mean)

        n_rep = len(run_list)
        all_events: dict = {k: [] for k in ("all_deaths", "ch_deaths", "ch_started")}
        for run in run_list:
            ev = _load_event_times_s(run, missing)
            for k in all_events:
                all_events[k].extend(_clean_event_times(ev[k], t_max_s=t_max))

        t_edges = np.arange(0, t_max + bin_sec, bin_sec)

        fig, ax = plt.subplots(figsize=(14, 5))
        t_min_arr = t_grid / 60.0
        color  = colors[proto]

        ax.plot(t_min_arr, pdr_mean, color=color, linewidth=1.8,
                label=f"{proto} (mean)")
        ax.fill_between(t_min_arr, pdr_mean - pdr_std, pdr_mean + pdr_std,
                        alpha=0.25, color=color, label="±1σ variance")

        for (ts, te) in intervals:
            ax.axvspan(ts / 60.0, te / 60.0, color="gold", alpha=0.25, linewidth=0)

        _add_binned_event_rug(ax, all_events["ch_deaths"],  t_edges, n_rep,
                              "darkred",   "CH death (binned, mean/rep)")
        _add_binned_event_rug(ax, all_events["all_deaths"], t_edges, n_rep,
                              "red",       "UAV death (binned, mean/rep)")
        _add_binned_event_rug(ax, all_events["ch_started"], t_edges, n_rep,
                              "steelblue", "CH charging started (binned, mean/rep)")

        if not np.isnan(dip_threshold):
            ax.axhline(dip_threshold, color="goldenrod", linestyle="--", linewidth=0.8,
                       label=f"Dip threshold p10 = {dip_threshold:.2f}")

        ax.set_xlabel("Experiment time (min)")
        ax.set_ylabel("Window PDR (mean ± 1σ across replicates)")
        proto_safe = proto.replace("/", "_")
        ax.set_title(f"PDR + Events + Variance — {proto} — Merged Replicates")
        ax.set_ylim(-0.05, 1.05)
        deduplicate_legend(ax, fontsize=7)
        savefig(fig, fig_dir, f"07_pdr_events_merged_variance_{proto_safe}")
        print(f"  [OK] 07_pdr_events_merged_variance_{proto_safe}")


def _plot_07_window_pdr_distribution(runs, fig_dir, missing):
    """G7/A3 — Window PDR distribution per protocol.

    For each protocol a single figure is produced with two panels:
      Left:  Histogram of all window_pdr observations pooled across replicates,
             with a KDE overlay.
      Right: Box-and-whisker per replicate so run-to-run variance is visible.

    The vertical dashed line marks the dip threshold (p10 pooled across all
    windows of that protocol) so the reader can relate the distribution to the
    dip shading used in the PDR+events plots.

    Source: network_timeseries.csv -> window_pdr.
    """
    groups = _group_by_protocol(runs)
    colors = protocol_color_map(list(groups.keys()))

    for proto, run_list in groups.items():
        all_vals: list = []          # every window_pdr value, all replicates
        per_run_vals: list = []      # list of arrays, one per replicate
        run_labels:   list = []

        for run in run_list:
            net_df = _load(run, "network_timeseries", missing)
            if net_df is None or "window_pdr" not in net_df.columns:
                continue
            vals = net_df.loc[net_df["window_pdr"] >= 0, "window_pdr"].dropna().values
            if len(vals) == 0:
                continue
            all_vals.extend(vals.tolist())
            per_run_vals.append(vals)
            run_labels.append(run["label"])

        if not all_vals:
            continue

        arr = np.asarray(all_vals)
        dip_thr = float(np.nanpercentile(arr, 10))
        color   = colors[proto]

        fig, (ax_hist, ax_box) = plt.subplots(1, 2, figsize=(13, 5),
                                               gridspec_kw={"width_ratios": [2, 1]})

        # --- Left: histogram + KDE ---
        ax_hist.hist(arr, bins=40, density=True, color=color, alpha=0.45,
                     edgecolor="white", linewidth=0.4, label="All replicates")

        if len(arr) > 5:
            from scipy.stats import gaussian_kde
            kde = gaussian_kde(arr, bw_method="scott")
            x_kde = np.linspace(max(0, arr.min() - 0.02), min(1, arr.max() + 0.02), 300)
            ax_hist.plot(x_kde, kde(x_kde), color=color, linewidth=2.0, label="KDE")

        ax_hist.axvline(dip_thr, color="goldenrod", linestyle="--", linewidth=1.2,
                        label=f"Dip threshold p10 = {dip_thr:.3f}")
        ax_hist.set_xlabel("Window PDR")
        ax_hist.set_ylabel("Density")
        ax_hist.set_title(f"Distribution — {proto}")
        ax_hist.set_xlim(-0.02, 1.02)
        ax_hist.legend(fontsize=8)

        # annotate mean ± std
        ax_hist.annotate(
            f"mean = {arr.mean():.3f}\nstd  = {arr.std():.3f}\nn = {len(arr):,}",
            xy=(0.03, 0.97), xycoords="axes fraction",
            ha="left", va="top", fontsize=8,
            bbox=dict(boxstyle="round,pad=0.3", facecolor="white",
                      edgecolor="grey", alpha=0.8),
        )

        # --- Right: box per replicate ---
        if per_run_vals:
            bp = ax_box.boxplot(per_run_vals, vert=True, patch_artist=True,
                                labels=run_labels, showfliers=True,
                                flierprops=dict(marker=".", markersize=3, alpha=0.4))
            for patch in bp["boxes"]:
                patch.set_facecolor(color)
                patch.set_alpha(0.55)
            ax_box.axhline(dip_thr, color="goldenrod", linestyle="--",
                           linewidth=1.0, label=f"p10 = {dip_thr:.3f}")
            ax_box.set_ylabel("Window PDR")
            ax_box.set_title("Per-replicate boxplot")
            ax_box.set_ylim(-0.05, 1.05)
            ax_box.tick_params(axis="x", rotation=30, labelsize=7)
            ax_box.legend(fontsize=7)

        fig.suptitle(f"Window PDR Distribution — {proto}", fontsize=11)
        fig.tight_layout()
        proto_safe = proto.replace("/", "_")
        savefig(fig, fig_dir, f"07_window_pdr_dist_{proto_safe}")
        print(f"  [OK] 07_window_pdr_dist_{proto_safe}")


def _plot_07_pdr_with_events_merged_variance_all(runs, fig_dir, missing,
                                                  bin_sec: float = 60.0):
    """G7/A1-MVA — All protocols stacked; each panel shows mean PDR ±1σ across
    replicates with binned event rugs (no raw per-replicate vlines).

    Combines the all-protocols panel layout of _plot_07_pdr_with_events_merged_panel
    with the variance band of _plot_07_pdr_with_events_merged_variance.
    Event markers are binned (mean events per replicate per bin) so N replicates
    do not produce N overlapping lines.
    """
    groups = _group_by_protocol(runs)
    protos = list(groups.keys())
    n = len(protos)
    if n == 0:
        return

    fig, axes = plt.subplots(n, 1, figsize=(14, 3.5 * n), sharex=True)
    if n == 1:
        axes = [axes]

    colors = protocol_color_map(protos)
    any_plot = False

    for ax, proto in zip(axes, protos):
        run_list = groups[proto]

        series = []
        t_max = 0.0
        for run in run_list:
            net_df = _load(run, "network_timeseries", missing)
            if net_df is None or "window_pdr" not in net_df.columns or "t_rel_s" not in net_df.columns:
                continue
            net_df = net_df[net_df["window_pdr"] >= 0].sort_values("t_rel_s")
            if net_df.empty:
                continue
            t_max = max(t_max, float(net_df["t_rel_s"].iloc[-1]))
            series.append((net_df["t_rel_s"].values, net_df["window_pdr"].values))

        if not series:
            continue

        t_grid  = np.arange(0, t_max + bin_sec, bin_sec)
        interped = [_ts_to_grid(t, y, t_grid) for t, y in series]
        stack    = np.vstack(interped)
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", RuntimeWarning)
            pdr_mean = np.nanmean(stack, axis=0)
            pdr_std  = np.nanstd(stack,  axis=0)

        intervals, dip_threshold = _pdr_dip_intervals(t_grid, pdr_mean)

        n_rep = len(run_list)
        all_events: dict = {k: [] for k in ("all_deaths", "ch_deaths", "ch_started")}
        for run in run_list:
            ev = _load_event_times_s(run, missing)
            for k in all_events:
                all_events[k].extend(_clean_event_times(ev[k], t_max_s=t_max))

        t_edges = np.arange(0, t_max + bin_sec, bin_sec)
        color   = colors[proto]
        t_min_arr = t_grid / 60.0

        ax.plot(t_min_arr, pdr_mean, color=color, linewidth=1.8,
                label=f"{proto} (mean)")
        ax.fill_between(t_min_arr, pdr_mean - pdr_std, pdr_mean + pdr_std,
                        alpha=0.22, color=color, label="±1σ")

        for (ts, te) in intervals:
            ax.axvspan(ts / 60.0, te / 60.0, color="gold", alpha=0.2, linewidth=0)

        _add_binned_event_rug(ax, all_events["ch_deaths"],  t_edges, n_rep,
                              "darkred",   "CH death (mean/rep)")
        _add_binned_event_rug(ax, all_events["all_deaths"], t_edges, n_rep,
                              "red",       "UAV death (mean/rep)")
        _add_binned_event_rug(ax, all_events["ch_started"], t_edges, n_rep,
                              "steelblue", "CH charging started (mean/rep)")

        if not np.isnan(dip_threshold):
            ax.axhline(dip_threshold, color="goldenrod", linestyle="--",
                       linewidth=0.7, label=f"p10 = {dip_threshold:.2f}")

        ax.set_ylabel("PDR (mean ± 1σ)", fontsize=8)
        ax.set_ylim(-0.05, 1.05)
        ax.set_title(proto, fontsize=9, pad=2)
        deduplicate_legend(ax, fontsize=6)
        any_plot = True

    if not any_plot:
        plt.close(fig)
        missing.append("PLOT 07_pdr_events_merged_variance_all: no window_pdr data")
        return

    axes[-1].set_xlabel("Experiment time (min)")
    fig.suptitle("PDR ± 1σ + Binned Events — Merged Replicates — All Protocols",
                 fontsize=11, y=1.01)
    fig.tight_layout()
    savefig(fig, fig_dir, "07_pdr_events_merged_variance_all")
    print("  [OK] 07_pdr_events_merged_variance_all")


# ---------------------------------------------------------------------------
# 07-A2  Dock utilisation + TIMEOUT markers — all protocols panel
# ---------------------------------------------------------------------------

def _plot_07_dock_util_with_timeouts(runs, fig_dir, missing):
    """G7/A2 — Dock utilisation timeseries + TIMEOUT event markers, per protocol.

    Source: charge_queue_timeseries.csv -> ugv_dock_utilization
    (same column used by _plot_03_dock_utilization).
    """
    groups = _group_by_protocol(runs)
    protos = list(groups.keys())
    n = len(protos)
    if n == 0:
        return

    fig, axes = plt.subplots(n, 1, figsize=(14, 3 * n), sharex=True)
    if n == 1:
        axes = [axes]

    colors = protocol_color_map(protos)
    any_plot = False

    for ax, proto in zip(axes, protos):
        run_list = groups[proto]
        for run in run_list:
            cq_df = _load(run, "charge_queue_timeseries", missing)
            if (cq_df is None
                    or "ugv_dock_utilization" not in cq_df.columns
                    or "t_rel_s" not in cq_df.columns):
                continue
            cq_df = cq_df[cq_df["ugv_dock_utilization"] >= 0].sort_values("t_rel_s")
            if cq_df.empty:
                continue
            ax.plot(cq_df["t_rel_s"] / 60.0, cq_df["ugv_dock_utilization"],
                    linewidth=1.2, alpha=0.8, color=colors[proto],
                    label=run["label"])
            any_plot = True

        timeout_times: list = []
        for run in run_list:
            ev = _load_event_times_s(run, missing)
            timeout_times.extend(ev["timeouts"])
        _add_event_vlines(ax, timeout_times,
                          "orange", "TIMEOUT", lw=0.9, linestyle=":", alpha=0.7)

        ax.set_ylabel("Dock util.", fontsize=8)
        ax.set_ylim(-0.05, 1.15)
        ax.set_title(proto, fontsize=9, pad=2)
        deduplicate_legend(ax, fontsize=6)

    if not any_plot:
        plt.close(fig)
        missing.append("PLOT 07_dock_util_with_timeouts: no dock utilisation data")
        return

    axes[-1].set_xlabel("Experiment time (min)")
    fig.suptitle("Dock Utilisation + TIMEOUT Markers — All Protocols", fontsize=11, y=1.01)
    fig.tight_layout()
    savefig(fig, fig_dir, "07_dock_util_with_timeouts")
    print("  [OK] 07_dock_util_with_timeouts")


def _plot_07_dock_util_with_timeouts_merged(runs, fig_dir, missing, bin_sec: float = 60.0):
    """G7/A2-M — Dock utilisation mean±1σ across replicates + TIMEOUT density, per protocol.

    Replicates are binned and averaged with _merged_ts.  TIMEOUT events from
    all replicates are pooled and drawn as a KDE-smoothed density strip at the
    bottom of each subplot so the viewer can see *when* timeouts tend to cluster
    without the visual noise of many individual vlines.

    Source: charge_queue_timeseries.csv (dock util), charge_events.csv (timeouts).
    """
    groups = _group_by_protocol(runs)
    protos = list(groups.keys())
    n = len(protos)
    if n == 0:
        return

    fig, axes = plt.subplots(n, 1, figsize=(14, 3.5 * n), sharex=True)
    if n == 1:
        axes = [axes]

    colors = protocol_color_map(protos)
    any_plot = False

    for ax, proto in zip(axes, protos):
        run_list = groups[proto]

        # --- merged timeseries (mean ± 1σ) ---
        t_grid, mean, std = _merged_ts(
            run_list, "charge_queue_timeseries", "ugv_dock_utilization", missing, bin_sec
        )
        if t_grid is not None:
            valid = ~np.isnan(mean)
            t_m = t_grid[valid] / 60.0
            m_v = mean[valid]
            s_v = std[valid]
            ax.plot(t_m, m_v, color=colors[proto], linewidth=1.8, label="mean")
            ax.fill_between(t_m, m_v - s_v, m_v + s_v,
                            alpha=0.25, color=colors[proto], label="±1σ")
            any_plot = True

        # --- pooled TIMEOUT times from all replicates ---
        timeout_min: list = []
        for run in run_list:
            ev = _load_event_times_s(run, [])
            timeout_min.extend(t / 60.0 for t in ev["timeouts"])

        if timeout_min:
            # draw as a rug at y=0 with small alpha so they don't dominate
            ax.plot(timeout_min, [0.02] * len(timeout_min),
                    "|", color="orange", alpha=0.55, markersize=8,
                    markeredgewidth=1.0, label="TIMEOUT (all replicates)")

        ax.set_ylabel("Dock util.", fontsize=8)
        ax.set_ylim(-0.08, 1.15)
        ax.set_title(proto, fontsize=9, pad=2)
        deduplicate_legend(ax, fontsize=6)

    if not any_plot:
        plt.close(fig)
        missing.append("PLOT 07_dock_util_with_timeouts_merged: no dock utilisation data")
        return

    axes[-1].set_xlabel("Experiment time (min)")
    fig.suptitle("Dock Utilisation — Merged Replicates (mean ± 1σ) + TIMEOUT Events",
                 fontsize=11, y=1.01)
    fig.tight_layout()
    savefig(fig, fig_dir, "07_dock_util_with_timeouts_merged")
    print("  [OK] 07_dock_util_with_timeouts_merged")


# ---------------------------------------------------------------------------
# 07-B  Lagged Pearson correlation
# ---------------------------------------------------------------------------

def _plot_07_lagged_correlation(runs, fig_dir, missing, bin_sec: float = 60.0):
    """G7/B — Per-protocol 2x2 lag-correlation subplots.

    Pairs analysed (x leads to y):
        PDR <-> TIMEOUT count
        PDR <-> death count
        dock utilisation <-> TIMEOUT count
        E2E delay <-> TIMEOUT count

    Lags span [-10 bins, +10 bins] at *bin_sec* seconds/bin (default +-10 min).
    """
    groups = _group_by_protocol(runs)
    max_lag_bins = 10

    summary_rows = []

    for proto, run_list in groups.items():
        pdr_series:   list = []
        util_series:  list = []
        delay_series: list = []
        to_series:    list = []
        death_series: list = []

        t_max_s = 0.0
        for run in run_list:
            net_df = _load(run, "network_timeseries", missing)
            if net_df is not None and "t_rel_s" in net_df.columns:
                t_max_s = max(t_max_s, float(net_df["t_rel_s"].max()))

        if t_max_s == 0.0:
            continue

        n_bins = max(1, int(np.ceil(t_max_s / bin_sec)))
        edges  = np.arange(0, (n_bins + 1) * bin_sec, bin_sec)

        for run in run_list:
            net_df = _load(run, "network_timeseries", missing)
            st_df  = _load(run, "status_timeseries",  missing)
            ev     = _load_event_times_s(run, missing)

            if net_df is not None and "window_pdr" in net_df.columns and "t_rel_s" in net_df.columns:
                pdr_vals = net_df[net_df["window_pdr"] >= 0].sort_values("t_rel_s")
                if not pdr_vals.empty:
                    bin_idx = np.clip(np.digitize(pdr_vals["t_rel_s"].values, edges) - 1, 0, n_bins - 1)
                    pdr_bin = np.full(n_bins, float("nan"))
                    for b in range(n_bins):
                        sel = pdr_vals["window_pdr"].values[bin_idx == b]
                        if len(sel) > 0:
                            pdr_bin[b] = float(np.nanmean(sel))
                    pdr_series.append(pdr_bin)

            if (st_df is not None
                    and "docks_occupied" in st_df.columns
                    and "total_docks" in st_df.columns
                    and "t_rel_s" in st_df.columns):
                st_clean = st_df.dropna(subset=["docks_occupied", "total_docks", "t_rel_s"]).copy()
                total = st_clean["total_docks"].replace(0, np.nan)
                util_vals = (st_clean["docks_occupied"] / total).values
                t_vals    = st_clean["t_rel_s"].values
                if len(t_vals) > 0:
                    bin_idx = np.clip(np.digitize(t_vals, edges) - 1, 0, n_bins - 1)
                    util_bin = np.full(n_bins, float("nan"))
                    for b in range(n_bins):
                        sel = util_vals[bin_idx == b]
                        if len(sel) > 0:
                            util_bin[b] = float(np.nanmean(sel))
                    util_series.append(util_bin)

            if (net_df is not None
                    and "window_delay_mean_ms" in net_df.columns
                    and "t_rel_s" in net_df.columns):
                delay_vals = net_df[net_df["window_delay_mean_ms"] >= 0].sort_values("t_rel_s")
                if not delay_vals.empty:
                    bin_idx = np.clip(np.digitize(delay_vals["t_rel_s"].values, edges) - 1, 0, n_bins - 1)
                    delay_bin = np.full(n_bins, float("nan"))
                    for b in range(n_bins):
                        sel = delay_vals["window_delay_mean_ms"].values[bin_idx == b]
                        if len(sel) > 0:
                            delay_bin[b] = float(np.nanmean(sel))
                    delay_series.append(delay_bin)

            to_series.append(_bin_events_to_grid(ev["timeouts"],    edges))
            death_series.append(_bin_events_to_grid(ev["all_deaths"], edges))

        def _avg(series_list):
            if not series_list:
                return np.full(n_bins, float("nan"))
            arr = np.vstack(series_list)
            with warnings.catch_warnings():
                warnings.simplefilter("ignore", RuntimeWarning)
                return np.nanmean(arr, axis=0)

        pdr_avg   = _avg(pdr_series)
        util_avg  = _avg(util_series)
        delay_avg = _avg(delay_series)
        to_avg    = _avg(to_series)
        death_avg = _avg(death_series)

        pairs = [
            ("PDR",            pdr_avg,   "TIMEOUT count", to_avg),
            ("PDR",            pdr_avg,   "Death count",   death_avg),
            ("Dock util.",     util_avg,  "TIMEOUT count", to_avg),
            ("E2E delay (ms)", delay_avg, "TIMEOUT count", to_avg),
        ]

        fig, axes = plt.subplots(2, 2, figsize=(12, 8), constrained_layout=True)
        proto_safe = proto.replace("/", "_")
        fig.suptitle(f"Lagged Pearson Correlation — {proto}", fontsize=11)

        for ax, (x_label, x_arr, y_label, y_arr) in zip(axes.flat, pairs):
            lags, corrs = _lagged_pearson(x_arr, y_arr, max_lag_bins)
            lag_m = lags * (bin_sec / 60.0)
            ax.plot(lag_m, corrs, linewidth=1.5, color="steelblue",
                    marker="o", markersize=3)
            ax.axvline(0, color="gray", linestyle="--", linewidth=0.8)
            ax.axhline(0, color="gray", linestyle="-",  linewidth=0.5)
            ax.set_xlabel("Lag (min)  [positive = y lags x]", fontsize=8)
            ax.set_ylabel("Pearson r", fontsize=8)
            ax.set_title(f"{x_label}  ->  {y_label}", fontsize=9)
            ax.set_ylim(-1.05, 1.05)

            valid = ~np.isnan(corrs)
            if valid.any():
                peak_idx = int(np.argmax(np.abs(corrs[valid])))
                peak_r   = float(corrs[valid][peak_idx])
                peak_lag = float(lag_m[valid][peak_idx])
                ax.scatter([peak_lag], [peak_r], color="red", zorder=5, s=40,
                           label=f"peak r={peak_r:.2f} @{peak_lag:.0f}min")
                ax.legend(fontsize=7)
                summary_rows.append({
                    "protocol":     proto,
                    "x_series":     x_label,
                    "y_series":     y_label,
                    "peak_r":       round(peak_r, 4),
                    "peak_lag_min": round(peak_lag, 1),
                })

        savefig(fig, fig_dir, f"07_lagged_corr_{proto_safe}")
        print(f"  [OK] 07_lagged_corr_{proto_safe}")

    if summary_rows:
        summary_path = fig_dir.parent / "summary_tables" / "07_lagged_corr_summary.csv"
        summary_path.parent.mkdir(parents=True, exist_ok=True)
        pd.DataFrame(summary_rows).to_csv(summary_path, index=False)
        print("  [OK] summary_tables/07_lagged_corr_summary.csv")


# ---------------------------------------------------------------------------
# 07-C  Death-slope vs PDR-dip boxplot
# ---------------------------------------------------------------------------

def _plot_07_death_slope_pdr_dips(runs, fig_dir, missing, bin_sec: float = 60.0):
    """G7/C — Rolling death rate slope during PDR dip intervals vs normal periods.

    For each protocol:
    - Bin deaths into *bin_sec* bins -> death_rate_per_bin.
    - Compute rolling slope of death rate via _rolling_slope().
    - Label each bin as 'dip' or 'normal' based on _pdr_dip_intervals().
    - Boxplot slope values by dip/normal.
    """
    groups = _group_by_protocol(runs)

    dip_slopes:    dict = {}
    normal_slopes: dict = {}

    for proto, run_list in groups.items():
        for run in run_list:
            net_df = _load(run, "network_timeseries", missing)
            ev     = _load_event_times_s(run, missing)

            if net_df is None or "window_pdr" not in net_df.columns or "t_rel_s" not in net_df.columns:
                continue

            net_df = net_df[net_df["window_pdr"] >= 0].sort_values("t_rel_s")
            if net_df.empty:
                continue

            t_max_s = float(net_df["t_rel_s"].max())
            n_bins  = max(1, int(np.ceil(t_max_s / bin_sec)))
            edges   = np.arange(0, (n_bins + 1) * bin_sec, bin_sec)
            bin_ctr = (edges[:-1] + edges[1:]) / 2.0

            death_counts = _bin_events_to_grid(ev["all_deaths"], edges)
            slope = _rolling_slope(death_counts, window=3)

            t_s   = net_df["t_rel_s"].values
            pdr   = net_df["window_pdr"].values
            intervals, _ = _pdr_dip_intervals(t_s, pdr)

            in_dip_mask = np.zeros(n_bins, dtype=bool)
            for (ts, te) in intervals:
                in_dip_mask |= (bin_ctr >= ts) & (bin_ctr <= te)

            dip_slopes.setdefault(proto, []).extend(slope[in_dip_mask].tolist())
            normal_slopes.setdefault(proto, []).extend(slope[~in_dip_mask].tolist())

    if not dip_slopes:
        missing.append("PLOT 07_death_slope_vs_pdr_dip: insufficient PDR / death data")
        return

    protos = sorted(set(list(dip_slopes.keys()) + list(normal_slopes.keys())))
    n = len(protos)
    fig, axes = plt.subplots(1, n, figsize=(max(6, 3 * n), 5), sharey=True)
    if n == 1:
        axes = [axes]

    for ax, proto in zip(axes, protos):
        data_dip    = [v for v in dip_slopes.get(proto, [])    if not np.isnan(v)]
        data_normal = [v for v in normal_slopes.get(proto, []) if not np.isnan(v)]
        boxes  = []
        labels = []
        if data_normal:
            boxes.append(data_normal)
            labels.append("Normal")
        if data_dip:
            boxes.append(data_dip)
            labels.append("PDR dip")
        if boxes:
            bp = ax.boxplot(boxes, patch_artist=True, notch=False)
            box_colors = ["#6baed6", "#fc8d59"][:len(boxes)]
            for patch, c in zip(bp["boxes"], box_colors):
                patch.set_facecolor(c)
            ax.set_xticklabels(labels, fontsize=8)
        ax.set_title(proto, fontsize=8)
        ax.axhline(0, color="gray", linestyle="--", linewidth=0.8)

    axes[0].set_ylabel("Rolling death rate slope (deaths/bin/bin)")
    fig.suptitle("Death Rate Slope: PDR Dip Intervals vs Normal Periods", fontsize=10)
    fig.tight_layout()
    savefig(fig, fig_dir, "07_death_slope_vs_pdr_dip")
    print("  [OK] 07_death_slope_vs_pdr_dip")


# ===========================================================================
# Derived tables
# ===========================================================================

def _save_qos_summary(runs, derived_dir, missing):
    frames = []
    for run in runs:
        df = _load(run, "qos_metrics", missing)
        if df is None:
            continue
        if "protocol" not in df.columns:
            df = df.copy()
            df.insert(0, "protocol", run["protocol"])
            df.insert(1, "replicate", run["replicate"])
        frames.append(df)

    if not frames:
        return

    out = Path(derived_dir) / "qos_summary_all_protocols.csv"
    out.parent.mkdir(parents=True, exist_ok=True)
    pd.concat(frames, ignore_index=True).to_csv(out, index=False)
    print(f"  [OK] derived/qos_summary_all_protocols.csv")


def _save_charge_events_combined(runs, derived_dir, missing):
    frames = []
    for run in runs:
        df = _load(run, "charge_events", missing)
        if df is None:
            continue
        df = df.copy()
        if "protocol" not in df.columns:
            df.insert(0, "protocol", run["protocol"])
        if "replicate" not in df.columns:
            df.insert(1, "replicate", run["replicate"])
        frames.append(df)

    if not frames:
        return

    out = Path(derived_dir) / "charge_events_all_protocols.csv"
    out.parent.mkdir(parents=True, exist_ok=True)
    pd.concat(frames, ignore_index=True).to_csv(out, index=False)
    print(f"  [OK] derived/charge_events_all_protocols.csv")


def _save_kpi_summary_table(runs, summary_dir, missing):
    rows = []
    for run in runs:
        s = _load_summary(run)
        if s is None:
            missing.append(f"MISSING summary.json for run: {run['label']}")
            # Still populate from CSVs where possible.
            s = {}

        ch = s.get("charging", {})
        fl = s.get("fleet", {})
        net = s.get("network", {})
        by_cat = net.get("by_category", [])
        total_gen = sum(c.get("generated", 0) for c in by_cat)
        total_del = sum(c.get("delivered", 0) for c in by_cat)
        overall_pdr = total_del / total_gen if total_gen > 0 else _overall_pdr_from_csvs(run, missing)

        rows.append(
            {
                "protocol":                    run["protocol"],
                "replicate":                   run["replicate"],
                "requests_total":              ch.get("requests_total"),
                "started":                     ch.get("started"),
                "success_rate":                ch.get("success_rate"),
                "timeouts":                    ch.get("timeouts"),
                "dropped":                     ch.get("dropped"),
                "preempted":                   ch.get("preempted"),
                "energy_depleted":             ch.get("energy_depleted"),
                "decision_latency_mean_ms":    ch.get("decision_latency_ms", {}).get("mean"),
                "decision_latency_p95_ms":     ch.get("decision_latency_ms", {}).get("p95"),
                "energy_recovered_mean_pct":   ch.get("energy_recovered", {}).get("mean"),
                "fleet_size":                  fl.get("fleet_size"),
                "dead_event_count":            fl.get("dead_event_count"),
                "survival_rate":               fl.get("survival_rate"),
                "overall_pdr":                 overall_pdr,
            }
        )

    if not rows:
        return

    out = Path(summary_dir) / "protocol_kpi_summary.csv"
    out.parent.mkdir(parents=True, exist_ok=True)
    pd.DataFrame(rows).to_csv(out, index=False)
    print(f"  [OK] summary_tables/protocol_kpi_summary.csv ({len(rows)} rows)")


# ===========================================================================
# Missing-data report
# ===========================================================================

_NEW_SCHEMA_TABLES = [
    "packet_generated_events.csv",
    "packet_delivered_events.csv",
    "charge_request_events.csv",
    "charge_decision_events.csv",
    "charge_session_events.csv",
    "summary_snapshots.jsonl",
]


def _write_missing_report(missing: list, output_root: Path):
    out = output_root / "data_requirements_missing.md"
    out.parent.mkdir(parents=True, exist_ok=True)

    lines = [
        "# Missing Data Report",
        "",
        "Auto-generated by `scripts/run_all_plots_test_round2.py`.",
        f"Input root: `{output_root.parent / 'experiment_data_collection/test_round2'}`",
        "",
        "## Missing files / columns",
        "",
    ]
    if not missing:
        lines.append("**No missing data detected — all expected files were present.**")
    else:
        for entry in sorted(set(missing)):
            lines.append(f"- {entry}")

    lines += [
        "",
        "## New-schema event tables (test_round2+)",
        "",
        "The following tables are written by the upgraded `network_monitor_node`",
        f"(branch `claude/upgrade-data-collection-logging-IVIvi`) and are **not**",
        "present in `test_round1` data:",
        "",
    ]
    for tbl in _NEW_SCHEMA_TABLES:
        lines.append(f"- `{tbl}`")

    lines += [
        "",
        "These tables enable additional plots (packet-level latency ECDF,",
        "charge-session energy in Wh, etc.).  Re-run experiments with the",
        "upgraded node to populate them.",
        "",
        "## Column renames between schemas",
        "",
        "| Old (test_round1)       | New (test_round2+) |",
        "| ----------------------- | ------------------ |",
        "| `time`                  | `time_s`           |",
        "| `ack_time`              | `ack_time_s`       |",
        "| `creation_time`         | `creation_time_s`  |",
        "| `request_time`          | `request_time_s`   |",
        "| `decision_time`         | `decision_time_s`  |",
        "| `energy_recovered`      | `energy_recovered_pct` |",
        "| *(absent)*              | `t_rel_s`          |",
        "| *(absent)*              | `protocol_name`    |",
        "| *(absent)*              | `replicate_id`     |",
        "| *(absent)*              | `run_instance_id`  |",
        "",
        "The pipeline applies these renames transparently so that old-schema",
        "data is plotted without modification.",
    ]

    out.write_text("\n".join(lines) + "\n")
    print(f"  [OK] {out.relative_to(output_root.parent)}")


# ===========================================================================
# Group 08 — Cross-layer analysis pipeline
# ===========================================================================

_CROSS_LAYER_SCRIPTS = [
    "build_cross_layer_windows.py",
    "cross_layer_lagged_correlation.py",
    "plot_cross_layer_overlays.py",
    "compute_charging_fairness.py",
]


def _run_cross_layer_pipeline(
    input_root: Path,
    output_root: Path,
    bin_sec: float,
    max_lag_min: float,
    missing: list,
) -> None:
    """Run the four cross-layer scripts as isolated subprocesses.

    Each script is launched as a fresh ``sys.executable`` process so it
    starts with a clean memory space, completely decoupled from the heap
    that Groups 01-07 accumulated (matplotlib figures, DataFrames, etc.).
    stdout/stderr from each subprocess stream directly to the terminal.

    Steps
    -----
    1. build_cross_layer_windows    — time-binned window tables
    2. cross_layer_lagged_correlation — Pearson/Spearman lag curves + heatmaps
    3. plot_cross_layer_overlays    — DPR/delay timeseries with event markers
    4. compute_charging_fairness    — Jain's fairness index per protocol
    """
    import subprocess

    scripts_dir = Path(__file__).parent
    cross_root  = output_root / "cross_layer"

    # Check that every add-on script is present before starting.
    missing_scripts = [s for s in _CROSS_LAYER_SCRIPTS
                       if not (scripts_dir / s).exists()]
    if missing_scripts:
        msg = f"Group 08 skipped — scripts not found: {missing_scripts}"
        print(f"  [WARN] {msg}")
        missing.append(msg)
        return

    steps = [
        {
            "name":   "Window tables",
            "script": "build_cross_layer_windows.py",
            "args":   ["--input", str(input_root),
                       "--output", str(cross_root),
                       "--bin-sec", str(bin_sec)],
        },
        {
            "name":   "Lagged correlation",
            "script": "cross_layer_lagged_correlation.py",
            "args":   ["--input", str(cross_root),
                       "--output", str(cross_root),
                       "--max-lag-min", str(max_lag_min),
                       "--bin-sec", str(bin_sec)],
        },
        {
            "name":   "Event overlays",
            "script": "plot_cross_layer_overlays.py",
            "args":   ["--input", str(cross_root),
                       "--output", str(cross_root)],
        },
        {
            "name":   "Charging fairness",
            "script": "compute_charging_fairness.py",
            "args":   ["--input", str(input_root),
                       "--output", str(cross_root)],
        },
    ]

    any_failure = False
    for step in steps:
        cmd = [sys.executable, str(scripts_dir / step["script"])] + step["args"]
        print(f"\n  [08] {step['name']} …")
        # stdout/stderr inherit from parent → stream directly to terminal.
        result = subprocess.run(cmd)
        if result.returncode != 0:
            msg = (f"Group 08 step '{step['name']}' exited with code "
                   f"{result.returncode}")
            print(f"  [WARN] {msg}")
            missing.append(msg)
            any_failure = True
        else:
            print(f"  [OK] {step['name']}")

    if not any_failure:
        print(f"\n  Group 08 complete. Outputs → {cross_root}/")


# ===========================================================================
# Main
# ===========================================================================

def main():
    args = _parse_args()
    input_root = Path(args.input)
    output_root = Path(args.output)
    bin_sec = args.bin_seconds  # reserved for future binned timeseries helpers

    fig_dirs = {
        "01": output_root / "figures" / "01_validation",
        "02": output_root / "figures" / "02_per_protocol_stats",
        "03": output_root / "figures" / "03_cross_protocol_charging",
        "04": output_root / "figures" / "04_policy_radar",
        "05": output_root / "figures" / "05_weather",
        "06": output_root / "figures" / "06_network_qos_delay",
        "07": output_root / "figures" / "07_causal_analysis",
    }
    derived_dir = output_root / "derived"
    summary_dir = output_root / "summary_tables"

    for d in [*fig_dirs.values(), derived_dir, summary_dir]:
        d.mkdir(parents=True, exist_ok=True)

    missing: list = []

    # ------------------------------------------------------------------
    # Discover runs
    # ------------------------------------------------------------------
    runs = discover_runs(input_root)
    if not runs:
        print(f"ERROR: no run folders found under {input_root}", file=sys.stderr)
        _write_missing_report(
            [f"Input root not found or contains no run folders: {input_root}"], output_root
        )
        sys.exit(1)

    print(f"\nFound {len(runs)} run(s):")
    for r in runs:
        print(f"  {r['label']:30s}  {r['folder']}")

    # ------------------------------------------------------------------
    # Group 01 — Validation  (2 plots)
    # ------------------------------------------------------------------
    print("\n[Group 01] Validation")
    _plot_01_network_pdr_over_time(runs, fig_dirs["01"], missing)
    _plot_01_battery_ecdf(runs, fig_dirs["01"], missing)
    _plot_01_packet_generated_per_run(runs, fig_dirs["01"], missing)

    # ------------------------------------------------------------------
    # Group 02 — Per-Protocol Stats  (4 plots)
    # ------------------------------------------------------------------
    print("\n[Group 02] Per-Protocol Stats")
    _plot_02_charge_success_rate(runs, fig_dirs["02"], missing)
    _plot_02_decision_latency(runs, fig_dirs["02"], missing)
    _plot_02_effective_wait(runs, fig_dirs["02"], missing)
    _plot_02_energy_recovered(runs, fig_dirs["02"], missing)

    # ------------------------------------------------------------------
    # Group 03 — Cross-Protocol Charging  (4 plots)
    # ------------------------------------------------------------------
    print("\n[Group 03] Cross-Protocol Charging")
    _plot_03_charge_queue_length(runs, fig_dirs["03"], missing)
    _plot_03_dock_utilization(runs, fig_dirs["03"], missing)
    _plot_03_charge_outcome_breakdown(runs, fig_dirs["03"], missing)
    _plot_03_dead_uav_cumulative(runs, fig_dirs["03"], missing)
    _plot_03_cumulative_energy_charged(runs, fig_dirs["03"], missing)

    # ------------------------------------------------------------------
    # Group 04 — Policy Radar  (1 plot)
    # ------------------------------------------------------------------
    print("\n[Group 04] Policy Radar")
    _plot_04_policy_radar(runs, fig_dirs["04"], missing)

    # ------------------------------------------------------------------
    # Group 05 — Weather  (2 plots)
    # ------------------------------------------------------------------
    print("\n[Group 05] Weather")
    _plot_05_pdr_vs_weather_regime(runs, fig_dirs["05"], missing)
    _plot_05_weather_timeseries(runs, fig_dirs["05"], missing)

    # ------------------------------------------------------------------
    # Merged plots — one entry per protocol, replicates pooled  (12 plots)
    # ------------------------------------------------------------------
    print("\n[Group 01 Merged]")
    _plot_01_network_pdr_over_time_merged(runs, fig_dirs["01"], missing, bin_sec)
    _plot_01_battery_ecdf_merged(runs, fig_dirs["01"], missing)
    _plot_01_pdr_cdf_merged(runs, fig_dirs["01"], missing)
    _plot_01_packet_generated_merged(runs, fig_dirs["01"], missing)
    _plot_01_mean_pdr_merged(runs, fig_dirs["01"], missing)

    print("\n[Group 02 Merged]")
    _plot_02_charge_success_rate_merged(runs, fig_dirs["02"], missing)
    _plot_02_decision_latency_merged(runs, fig_dirs["02"], missing)
    _plot_02_effective_wait_merged(runs, fig_dirs["02"], missing)
    _plot_02_energy_recovered_merged(runs, fig_dirs["02"], missing)

    print("\n[Group 03 Merged]")
    _plot_03_charge_queue_length_merged(runs, fig_dirs["03"], missing, bin_sec)
    _plot_03_dock_utilization_merged(runs, fig_dirs["03"], missing, bin_sec)
    _plot_03_charge_outcome_breakdown_merged(runs, fig_dirs["03"], missing)
    _plot_03_dead_uav_cumulative_merged(runs, fig_dirs["03"], missing, bin_sec)
    _plot_03_cumulative_energy_charged_merged(runs, fig_dirs["03"], missing, bin_sec)

    print("\n[Group 04 Merged]")
    _plot_04_policy_radar_merged(runs, fig_dirs["04"], missing)

    print("\n[Group 05 Merged]")
    _plot_05_pdr_vs_weather_regime_merged(runs, fig_dirs["05"], missing)
    _plot_05_delay_vs_weather_regime_merged(runs, fig_dirs["05"], missing)

    # ------------------------------------------------------------------
    # Group 05 additional (single-replicate)
    # ------------------------------------------------------------------
    print("\n[Group 05 additional]")
    _plot_05_delay_vs_weather_regime(runs, fig_dirs["05"], missing)

    # ------------------------------------------------------------------
    # Group 06 — Network QoS & E2E Delay
    # ------------------------------------------------------------------
    print("\n[Group 06] Network QoS & E2E Delay")
    _plot_06_qos_heatmap(runs, fig_dirs["06"], missing)
    _plot_06_e2e_delay(runs, fig_dirs["06"], missing)

    print("\n[Group 06 Merged]")
    _plot_06_qos_heatmap_merged(runs, fig_dirs["06"], missing)
    _plot_06_e2e_delay_merged(runs, fig_dirs["06"], missing)
    _plot_06_mean_e2e_delay_merged(runs, fig_dirs["06"], missing)
    _plot_correlation_heatmap_pdr(runs, fig_dirs["06"], missing)
    _plot_correlation_heatmap_e2e(runs, fig_dirs["06"], missing)
    _plot_06_pdr_vs_e2e_scatter(runs, fig_dirs["06"], missing)

    # ------------------------------------------------------------------
    # Group 07 — Causal Analysis
    # ------------------------------------------------------------------
    print("\n[Group 07] Causal Analysis")
    _plot_07_pdr_with_events_per_protocol(runs, fig_dirs["07"], missing)
    _plot_07_pdr_with_events_panel(runs, fig_dirs["07"], missing)
    _plot_07_pdr_with_events_merged(runs, fig_dirs["07"], missing, bin_sec)
    _plot_07_pdr_with_events_merged_variance(runs, fig_dirs["07"], missing, bin_sec)
    _plot_07_pdr_with_events_merged_panel(runs, fig_dirs["07"], missing, bin_sec)
    _plot_07_pdr_with_events_merged_variance_all(runs, fig_dirs["07"], missing, bin_sec)
    _plot_07_window_pdr_distribution(runs, fig_dirs["07"], missing)
    _plot_07_dock_util_with_timeouts(runs, fig_dirs["07"], missing)
    _plot_07_dock_util_with_timeouts_merged(runs, fig_dirs["07"], missing, bin_sec)
    _plot_07_lagged_correlation(runs, fig_dirs["07"], missing, bin_sec)
    _plot_07_death_slope_pdr_dips(runs, fig_dirs["07"], missing, bin_sec)

    # ------------------------------------------------------------------
    # Group 08 — Cross-layer analysis (disabled: scripts have known bugs
    #             that cause system crashes; unwired until fixed)
    # ------------------------------------------------------------------
    print("\n[Group 08] Cross-layer Analysis — SKIPPED (bugged scripts)")
    missing.append("Group 08 skipped: cross-layer scripts cause system crashes")

    # ------------------------------------------------------------------
    # Derived tables
    # ------------------------------------------------------------------
    print("\n[Derived tables]")
    _save_qos_summary(runs, derived_dir, missing)
    _save_charge_events_combined(runs, derived_dir, missing)

    # ------------------------------------------------------------------
    # Summary KPI table
    # ------------------------------------------------------------------
    print("\n[Summary table]")
    _save_kpi_summary_table(runs, summary_dir, missing)

    # ------------------------------------------------------------------
    # Missing-data report
    # ------------------------------------------------------------------
    print("\n[Missing data report]")
    _write_missing_report(missing, output_root)

    print(f"\nDone. All outputs written under: {output_root}/")


if __name__ == "__main__":
    main()
