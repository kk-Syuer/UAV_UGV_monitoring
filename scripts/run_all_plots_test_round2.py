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
    Load ``summary.json`` for *run*.  Falls back to the last line of
    ``summary_snapshots.jsonl`` (new schema).  Returns ``None`` if neither
    file is present.
    """
    path = run["folder"] / "summary.json"
    if path.exists():
        with open(path) as fh:
            return json.load(fh)

    jsonl_path = run["folder"] / "summary_snapshots.jsonl"
    if jsonl_path.exists():
        lines = [ln.strip() for ln in jsonl_path.read_text().splitlines() if ln.strip()]
        if lines:
            return json.loads(lines[-1])
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


def _mean_energy_from_csvs(run: dict, missing: list) -> float:
    df = _load(run, "charge_events", missing)
    if df is None:
        return float("nan")
    col = next((c for c in ("energy_recovered_pct", "energy_recovered") if c in df.columns), None)
    if col is None:
        return float("nan")
    return float(df[col].replace(-1, float("nan")).mean())


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
        df = _load(run, "charge_events", missing)
        if df is None:
            continue
        col = next(
            (c for c in ("energy_recovered_pct", "energy_recovered") if c in df.columns),
            None,
        )
        if col is None:
            continue
        # Restrict to sessions where charging was actually started.
        if "outcome" in df.columns:
            df = df[df["outcome"].isin(["STARTED", "ENERGY_DEPLETED"])].copy()
        vals = df[col].replace(-1, float("nan")).dropna().values
        if len(vals) > 0:
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
    bp = ax.boxplot(data, tick_labels=regimes, patch_artist=True, showfliers=False)
    for patch in bp["boxes"]:
        patch.set_facecolor("#4878d0")
        patch.set_alpha(0.6)
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
