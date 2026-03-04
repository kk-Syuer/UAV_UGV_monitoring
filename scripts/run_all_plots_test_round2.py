#!/usr/bin/env python3
"""
run_all_plots_test_round2.py
-----------------------------
End-to-end plotting pipeline for UAV/UGV experiment results.

Produces 13 figures across 5 thematic groups, derived CSV tables, a
protocol KPI summary table, and a missing-data report.

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
    """Return an array of energy-recovered (%) values for completed charge sessions.

    Tries schemas in order:
    1. charge_events.csv  — column ``energy_recovered`` or ``energy_recovered_pct``,
       filtered to outcome STARTED | ENERGY_DEPLETED.
    2. charge_session_events.csv (new schema) — DOCK_END rows,
       ``battery_after - battery_before`` (both already in % units).
    Returns None only when neither file is present; returns an empty array
    when files exist but no completed sessions are found.
    """
    # --- old schema (charge_events.csv) ---
    # Use a probe list so that "file missing" is not reported when the new-schema
    # file below covers the same information.
    _probe: list = []
    df = _load(run, "charge_events", _probe)
    if df is not None:
        col = next((c for c in ("energy_recovered_pct", "energy_recovered") if c in df.columns), None)
        if col is not None:
            if "outcome" in df.columns:
                df = df[df["outcome"].isin(["STARTED", "ENERGY_DEPLETED"])].copy()
            return df[col].replace(-1, float("nan")).dropna().values

    # --- new schema (charge_session_events.csv) ---
    df2 = _load(run, "charge_session_events", missing)
    if df2 is not None and "event_type" in df2.columns:
        ended = df2[df2["event_type"] == "DOCK_END"].copy()
        if "battery_before" in ended.columns and "battery_after" in ended.columns:
            vals = (ended["battery_after"] - ended["battery_before"]).replace(-1, float("nan")).dropna()
            return vals.values

    # Both files absent — propagate old-schema probe warnings now.
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
        "Energy recovered (%)",
        "Energy Recovered per Charge Session (STARTED events)",
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


def _plot_03_charge_outcome_breakdown(runs, fig_dir, missing):
    """G3/P3 — Stacked bar of charge request outcomes per protocol."""
    OUTCOMES = ["STARTED", "REJECTED", "DROPPED", "TIMEOUT", "PREEMPTED", "ENERGY_DEPLETED"]
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
        row = {"label": run["label"]}
        for o in OUTCOMES:
            row[o] = int(counts.get(o, 0))
        rows.append(row)

    if not rows:
        missing.append("PLOT 03_charge_outcome_breakdown: no outcome data")
        return

    summary_df = pd.DataFrame(rows).set_index("label")
    labels = summary_df.index.tolist()

    fig, ax = plt.subplots(figsize=(max(8, len(labels) * 1.5), 6))
    bottom = np.zeros(len(labels))
    for outcome in OUTCOMES:
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
        data, "Energy recovered (%)",
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
    """G3/P3-M — Stacked bar of outcomes per protocol (counts summed across replicates)."""
    OUTCOMES = ["STARTED", "REJECTED", "DROPPED", "TIMEOUT", "PREEMPTED", "ENERGY_DEPLETED"]
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
    for proto, run_list in groups.items():
        row = {o: 0 for o in OUTCOMES}
        row["label"] = proto
        has_data = False
        for run in run_list:
            df = _load(run, "charge_events", missing)
            if df is None or "outcome" not in df.columns:
                continue
            counts = df["outcome"].value_counts()
            for o in OUTCOMES:
                row[o] += int(counts.get(o, 0))
            has_data = True
        if has_data:
            rows.append(row)

    if not rows:
        missing.append("PLOT 03_charge_outcome_breakdown_merged: no outcome data")
        return

    summary_df = pd.DataFrame(rows).set_index("label")
    labels = summary_df.index.tolist()
    fig, ax = plt.subplots(figsize=(max(8, len(labels) * 1.5), 6))
    bottom = np.zeros(len(labels))
    for outcome in OUTCOMES:
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

    print("\n[Group 04 Merged]")
    _plot_04_policy_radar_merged(runs, fig_dirs["04"], missing)

    print("\n[Group 05 Merged]")
    _plot_05_pdr_vs_weather_regime_merged(runs, fig_dirs["05"], missing)

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
