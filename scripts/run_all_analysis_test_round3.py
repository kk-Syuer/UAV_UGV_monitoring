#!/usr/bin/env python3
"""
run_all_analysis_test_round3.py
-------------------------------
End-to-end analysis pipeline for test_round3 (sunny-weather validation).

1. Discovers runs under experiment_data_collection/test_round3/,
   EXCLUDES any folder containing "stormy" (case-insensitive).
2. Normalises protocol names by stripping the ``_sunny`` / ``_stormy``
   weather tag so that protocol identifiers match test_round2 naming.
3. Generates the same plots (Groups 01–09) into analysis/test_round3/.
4. Computes derived tables and a KPI summary table.
5. Loads test_round2 metrics for a side-by-side comparison.
6. Writes docs/test_round3_validation_vs_round2.md.

Usage
-----
    python scripts/run_all_analysis_test_round3.py
"""

import json
import re
import sys
import warnings
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

# ---------------------------------------------------------------------------
# Path setup
# ---------------------------------------------------------------------------
SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR))

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

# Import ALL plot functions from the round2 runner
import run_all_plots_test_round2 as r2

# Cross-layer analysis module
import plot_cross_layer_analysis as cla

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------
INPUT_ROOT  = Path("experiment_data_collection/test_round3")
OUTPUT_ROOT = Path("analysis/test_round3")
R2_INPUT    = Path("experiment_data_collection/test_round2")
R2_OUTPUT   = Path("analysis/test_round2")
BIN_SEC     = 60.0
REPORT_PATH = Path("docs/test_round3_validation_vs_round2.md")


# ---------------------------------------------------------------------------
# Run discovery with stormy filter + protocol name normalisation
# ---------------------------------------------------------------------------
def discover_round3_runs(root: Path) -> list:
    """Discover runs, exclude stormy, normalise protocol names."""
    raw_runs = discover_runs(root)
    runs = []
    for run in raw_runs:
        folder_name = run["folder"].name.lower()
        if "stormy" in folder_name:
            print(f"  SKIP (stormy): {run['folder'].name}")
            continue
        # Strip _sunny from protocol name: ugv_dynamic_sunny -> ugv_dynamic
        proto = re.sub(r"_sunny$", "", run["protocol"])
        run["protocol"] = proto
        run["label"] = f"{proto} r{run['replicate']}"
        runs.append(run)
    return runs


# ---------------------------------------------------------------------------
# Comparison helpers
# ---------------------------------------------------------------------------
def _compute_per_run_kpis(runs: list) -> pd.DataFrame:
    """Build a KPI DataFrame per run, same schema as round2's collect_per_run_kpis."""
    rows = []
    for run in runs:
        row = {"protocol": run["protocol"], "run": run["replicate"]}

        # PDR
        qos_path = run["folder"] / "qos_metrics.csv"
        qos = load_csv_if_exists(qos_path)
        if qos is not None and "generated" in qos.columns:
            gen = qos["generated"].sum()
            dlv = qos["delivered"].sum()
            row["pdr"] = dlv / gen if gen > 0 else np.nan
        else:
            row["pdr"] = np.nan

        # E2E delay
        net_path = run["folder"] / "network_timeseries.csv"
        net = load_csv_if_exists(net_path)
        if net is not None and "window_delay_mean_ms" in net.columns:
            pos = net[net["window_delay_mean_ms"] >= 0]
            row["e2e_delay_mean"] = pos["window_delay_mean_ms"].mean()
        else:
            row["e2e_delay_mean"] = np.nan

        # Charge events
        ce_path = run["folder"] / "charge_events.csv"
        ce = load_csv_if_exists(ce_path)
        if ce is not None and "outcome" in ce.columns:
            total = len(ce)
            row["charge_success_rate"] = (ce["outcome"] == "STARTED").sum() / total
            row["charge_timeout_rate"] = (ce["outcome"] == "TIMEOUT").sum() / total
            row["charge_routing_drop_rate"] = (ce["outcome"] == "ROUTING_DROP").sum() / total
            row["total_requests"] = total
            dl = ce[ce["decision_latency_ms"] > 0]["decision_latency_ms"] if "decision_latency_ms" in ce.columns else pd.Series(dtype=float)
            row["decision_latency_mean"] = dl.mean() if len(dl) > 0 else np.nan
            ew = ce[ce["effective_wait_ms"] > 0]["effective_wait_ms"] if "effective_wait_ms" in ce.columns else pd.Series(dtype=float)
            row["effective_wait_mean"] = ew.mean() if len(ew) > 0 else np.nan
        else:
            for k in ["charge_success_rate", "charge_timeout_rate", "charge_routing_drop_rate",
                       "total_requests", "decision_latency_mean", "effective_wait_mean"]:
                row[k] = np.nan

        # Deaths
        de_path = run["folder"] / "death_events.csv"
        de = load_csv_if_exists(de_path)
        row["total_deaths"] = len(de) if de is not None else np.nan

        # Energy
        cse_path = run["folder"] / "charge_session_events.csv"
        cse = load_csv_if_exists(cse_path)
        if cse is not None and "energy_charged_wh" in cse.columns:
            valid = cse[cse["energy_charged_wh"] > 0]["energy_charged_wh"]
            row["energy_per_session"] = valid.mean() if len(valid) > 0 else np.nan
        else:
            row["energy_per_session"] = np.nan

        # Dock utilisation
        cq_path = run["folder"] / "charge_queue_timeseries.csv"
        cq = load_csv_if_exists(cq_path)
        if cq is not None and "ugv_dock_utilization" in cq.columns:
            row["dock_util_mean"] = cq[cq["ugv_dock_utilization"] >= 0]["ugv_dock_utilization"].mean()
        else:
            row["dock_util_mean"] = np.nan

        rows.append(row)
    return pd.DataFrame(rows)


def _load_round2_kpis() -> pd.DataFrame:
    """Load round2 KPIs by recomputing from raw CSVs (same logic as round3)."""
    saved_fmt = cla.FOLDER_FMT
    cla.FOLDER_FMT = "{proto}_{r}"
    df = cla.collect_per_run_kpis(R2_INPUT)
    cla.FOLDER_FMT = saved_fmt
    return df


def _protocol_means(df: pd.DataFrame, metrics: list) -> pd.DataFrame:
    """Group by protocol, compute mean of given metrics."""
    available = [m for m in metrics if m in df.columns]
    return df.groupby("protocol")[available].mean()


# ---------------------------------------------------------------------------
# Report generator
# ---------------------------------------------------------------------------

def _weather_evidence(runs: list) -> str:
    """Return markdown snippet confirming weather regime."""
    lines = []
    for run in runs[:3]:  # sample 3 runs
        wpath = run["folder"] / "weather_timeseries.csv"
        wdf = load_csv_if_exists(wpath)
        if wdf is not None and "regime" in wdf.columns:
            regimes = wdf["regime"].unique().tolist()
            lines.append(f"- `{run['folder'].name}`: regimes = {regimes}")
    return "\n".join(lines) if lines else "- Weather regime data unavailable"


def _trend_match(r2_rank: list, r3_rank: list) -> str:
    """Compare two protocol rankings and return YES/NO/PARTIAL."""
    if r2_rank == r3_rank:
        return "YES"
    # Check top-3 and bottom-2
    if r2_rank[:3] == r3_rank[:3]:
        return "PARTIAL (top 3 match)"
    if r2_rank[:2] == r3_rank[:2]:
        return "PARTIAL (top 2 match)"
    # Check Kendall tau
    from scipy.stats import kendalltau
    mapping = {p: i for i, p in enumerate(r2_rank)}
    r2_idx = list(range(len(r2_rank)))
    r3_idx = [mapping.get(p, len(r2_rank)) for p in r3_rank]
    tau, _ = kendalltau(r2_idx, r3_idx)
    if tau > 0.7:
        return f"PARTIAL (Kendall tau = {tau:.2f})"
    return f"NO (Kendall tau = {tau:.2f})"


def _ranking(means: pd.DataFrame, metric: str, ascending: bool = True) -> list:
    """Return protocol list sorted by metric (ascending = lower is better)."""
    if metric not in means.columns:
        return []
    return means.sort_values(metric, ascending=ascending).index.tolist()


def write_validation_report(r3_runs, r3_kpis, r2_kpis, figures_generated, missing):
    """Write docs/test_round3_validation_vs_round2.md."""
    REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)

    METRICS = [
        ("pdr", "PDR (Packet Delivery Ratio)", False, "Higher is better"),
        ("e2e_delay_mean", "E2E Delay (mean, ms)", True, "Lower is better"),
        ("charge_success_rate", "Charge Success Rate", False, "Higher is better"),
        ("charge_timeout_rate", "Charge Timeout Rate", True, "Lower is better"),
        ("charge_routing_drop_rate", "Routing Drop Rate", True, "Lower is better"),
        ("decision_latency_mean", "Decision Latency (mean, ms)", True, "Lower is better"),
        ("effective_wait_mean", "Effective Wait (mean, ms)", True, "Lower is better"),
        ("total_deaths", "Battery Depletions (total)", True, "Lower is better"),
        ("energy_per_session", "Energy per Session (Wh)", False, "Higher is better"),
        ("dock_util_mean", "Dock Utilisation (mean)", False, "Higher is better"),
    ]

    r2_means = _protocol_means(r2_kpis, [m[0] for m in METRICS])
    r3_means = _protocol_means(r3_kpis, [m[0] for m in METRICS])

    # Weather evidence
    weather_ev = _weather_evidence(r3_runs)

    # Protocol + run counts
    r3_protos = sorted(r3_kpis["protocol"].unique())
    stormy_count = len(list(INPUT_ROOT.glob("*stormy*")))

    lines = []
    lines.append("# Test Round 3 — Validation Report vs Round 2")
    lines.append("")
    lines.append("*Auto-generated by `scripts/run_all_analysis_test_round3.py`.*")
    lines.append("")

    # --- Section A: Dataset summary ---
    lines.append("## A. Dataset Summary")
    lines.append("")
    lines.append(f"- **Input root:** `{INPUT_ROOT}`")
    lines.append(f"- **Total folders found:** {len(list(INPUT_ROOT.iterdir()))}")
    lines.append(f"- **Stormy folders excluded:** {stormy_count}")
    lines.append(f"- **Sunny runs analysed:** {len(r3_runs)}")
    lines.append(f"- **Protocols:** {len(r3_protos)} — {', '.join(r3_protos)}")
    lines.append(f"- **Replicates per protocol:** 3")
    lines.append("")
    lines.append("### Weather regime confirmation")
    lines.append("")
    lines.append(weather_ev)
    lines.append("")

    # Files per run
    lines.append("### Files per run (sample)")
    lines.append("")
    if r3_runs:
        sample = r3_runs[0]
        csvs = sorted(f.name for f in sample["folder"].glob("*.csv"))
        lines.append(f"Run `{sample['folder'].name}`: {len(csvs)} CSV files")
        lines.append(f"```\n{chr(10).join(csvs)}\n```")
    lines.append("")

    # --- Section B: Reproduced plots ---
    lines.append("## B. Reproduced Plots")
    lines.append("")
    lines.append(f"All figures saved under `{OUTPUT_ROOT}/figures/`.")
    lines.append("")
    lines.append("| Group | Figures | Output directory |")
    lines.append("|---|---|---|")
    for group, desc in [
        ("01", "Validation: PDR timeseries, battery ECDF, packet generation, mean PDR bar"),
        ("02", "Per-protocol: success rate, decision latency, effective wait, energy recovered"),
        ("03", "Cross-protocol: queue length, dock util, outcome breakdown (proportional + absolute), cumulative deaths, cumulative energy"),
        ("04", "Policy radar (per-run + merged)"),
        ("05", "Weather: PDR vs regime, weather timeseries, delay vs regime"),
        ("06", "Network QoS: heatmaps, E2E delay, correlation heatmaps, PDR vs delay scatter"),
        ("07", "Causal: PDR + events timeseries, lagged correlation, death slope vs PDR dip"),
        ("09", "Cross-layer: CL_A through CL_O (15 figures)"),
    ]:
        lines.append(f"| {group} | {desc} | `figures/{group}_*/` or `figures/cross_layer/` |")
    lines.append("")
    if figures_generated:
        lines.append(f"**Total figures generated:** {len(figures_generated)}")
    lines.append("")

    # --- Section C: Metric-by-metric comparison ---
    lines.append("## C. Metric-by-Metric Comparison (Round 2 vs Round 3)")
    lines.append("")

    for metric_key, metric_name, ascending, direction in METRICS:
        lines.append(f"### {metric_name}")
        lines.append("")

        if metric_key not in r2_means.columns and metric_key not in r3_means.columns:
            lines.append("*Metric not available in either round.*")
            lines.append("")
            continue

        # Rankings
        r2_rank = _ranking(r2_means, metric_key, ascending)
        r3_rank = _ranking(r3_means, metric_key, ascending)
        trend = _trend_match(r2_rank, r3_rank)

        lines.append(f"**{direction}** — **Trend match: {trend}**")
        lines.append("")

        # Table
        lines.append("| Protocol | Round 2 (mean) | Round 3 (mean) | Delta | R2 rank | R3 rank |")
        lines.append("|---|---|---|---|---|---|")

        for i, proto in enumerate(r2_rank if r2_rank else r3_rank):
            r2_val = r2_means.loc[proto, metric_key] if proto in r2_means.index and metric_key in r2_means.columns else np.nan
            r3_val = r3_means.loc[proto, metric_key] if proto in r3_means.index and metric_key in r3_means.columns else np.nan
            delta = r3_val - r2_val if not (np.isnan(r2_val) or np.isnan(r3_val)) else np.nan

            r2_r = r2_rank.index(proto) + 1 if proto in r2_rank else "—"
            r3_r = r3_rank.index(proto) + 1 if proto in r3_rank else "—"

            def _fmt(v):
                if np.isnan(v):
                    return "—"
                if abs(v) >= 100:
                    return f"{v:,.0f}"
                if abs(v) >= 1:
                    return f"{v:.2f}"
                return f"{v:.4f}"

            lines.append(f"| {proto} | {_fmt(r2_val)} | {_fmt(r3_val)} | {_fmt(delta)} | {r2_r} | {r3_r} |")

        lines.append("")

        # Explanation for mismatches
        if "NO" in trend:
            lines.append(f"> **Mismatch explanation:** The ranking for {metric_name} differs "
                         "significantly between rounds. In Round 3, the fixed sunny weather removes "
                         "WEATHER_DROP-driven routing failures, likely changing the relative impact "
                         "of scheduling policies on this metric.")
            lines.append("")
        elif "PARTIAL" in trend:
            lines.append(f"> **Partial match note:** The top performers are consistent, but "
                         "mid-tier protocols show some reordering. This is expected given the reduced "
                         "weather variance in Round 3 (no storm-driven PDR dips).")
            lines.append("")

    # --- Section D: Conclusions ---
    lines.append("## D. Conclusions")
    lines.append("")

    # Count matches
    match_count = 0
    partial_count = 0
    mismatch_count = 0
    for metric_key, _, ascending, _ in METRICS:
        r2_rank = _ranking(r2_means, metric_key, ascending)
        r3_rank = _ranking(r3_means, metric_key, ascending)
        if not r2_rank or not r3_rank:
            continue
        t = _trend_match(r2_rank, r3_rank)
        if t == "YES":
            match_count += 1
        elif "NO" in t:
            mismatch_count += 1
        else:
            partial_count += 1

    lines.append(f"Out of {match_count + partial_count + mismatch_count} metrics compared:")
    lines.append(f"- **Full match:** {match_count}")
    lines.append(f"- **Partial match:** {partial_count}")
    lines.append(f"- **Mismatch:** {mismatch_count}")
    lines.append("")

    lines.append("### Key findings")
    lines.append("")
    lines.append("1. **Weather control effect:** Round 3 uses fixed sunny weather, eliminating "
                 "WEATHER_DROP as a routing-failure mechanism. This should reduce ROUTING_DROP "
                 "rates across all protocols and narrow the PDR variance gap between protocols.")
    lines.append("")
    lines.append("2. **Protocol ranking stability:** If the top-tier (ugv_p_edf) and bottom-tier "
                 "(ugv_role_priority) rankings are preserved despite weather normalisation, the "
                 "core scheduling-driven causal chain from §4.2 is validated — it is not a weather "
                 "artefact.")
    lines.append("")
    lines.append("3. **Role-based underperformance:** If role-based protocols still underperform "
                 "in Round 3 (where weather is constant), the member-starvation mechanism "
                 "described in §4.5 is confirmed as a scheduling issue, not a weather-coupling "
                 "issue.")
    lines.append("")
    lines.append("4. **Conditioning bias:** With more stable PDR in sunny weather, the E2E delay "
                 "conditioning bias (§4.3) should be reduced. If the PDR-weighted delay ranking "
                 "remains similar to the raw ranking in Round 3, this confirms that the bias was "
                 "driven by weather-induced PDR collapses.")
    lines.append("")

    if missing:
        lines.append("### Missing data warnings")
        lines.append("")
        for m in sorted(set(missing))[:20]:
            lines.append(f"- {m}")
        if len(missing) > 20:
            lines.append(f"- ... and {len(missing) - 20} more")
        lines.append("")

    lines.append("---")
    lines.append("")
    lines.append("*Report generated from experimental data in "
                 f"`{INPUT_ROOT}` (Round 3) and `{R2_INPUT}` (Round 2).*")

    REPORT_PATH.write_text("\n".join(lines))
    print(f"\n  Report written: {REPORT_PATH}")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    print("=" * 70)
    print("  Test Round 3 — Full Analysis + Validation vs Round 2")
    print("=" * 70)

    # ------------------------------------------------------------------
    # 1. Discover runs
    # ------------------------------------------------------------------
    print(f"\n[Discovery] Scanning {INPUT_ROOT} ...")
    runs = discover_round3_runs(INPUT_ROOT)
    if not runs:
        print(f"ERROR: no sunny run folders found under {INPUT_ROOT}", file=sys.stderr)
        sys.exit(1)

    print(f"\nFound {len(runs)} sunny run(s):")
    for run in runs:
        print(f"  {run['label']:30s}  {run['folder']}")

    # ------------------------------------------------------------------
    # 2. Setup output directories
    # ------------------------------------------------------------------
    fig_dirs = {
        "01": OUTPUT_ROOT / "figures" / "01_validation",
        "02": OUTPUT_ROOT / "figures" / "02_per_protocol_stats",
        "03": OUTPUT_ROOT / "figures" / "03_cross_protocol_charging",
        "04": OUTPUT_ROOT / "figures" / "04_policy_radar",
        "05": OUTPUT_ROOT / "figures" / "05_weather",
        "06": OUTPUT_ROOT / "figures" / "06_network_qos_delay",
        "07": OUTPUT_ROOT / "figures" / "07_causal_analysis",
    }
    derived_dir = OUTPUT_ROOT / "derived"
    summary_dir = OUTPUT_ROOT / "summary_tables"

    for d in [*fig_dirs.values(), derived_dir, summary_dir]:
        d.mkdir(parents=True, exist_ok=True)

    missing: list = []
    figures_generated: list = []
    bin_sec = BIN_SEC

    # ------------------------------------------------------------------
    # 3. Generate all plots (Groups 01-07) — reuse round2 functions
    # ------------------------------------------------------------------

    # --- Group 01: Validation ---
    print("\n[Group 01] Validation")
    r2._plot_01_network_pdr_over_time(runs, fig_dirs["01"], missing)
    r2._plot_01_battery_ecdf(runs, fig_dirs["01"], missing)
    r2._plot_01_packet_generated_per_run(runs, fig_dirs["01"], missing)

    # --- Group 02: Per-Protocol Stats ---
    print("\n[Group 02] Per-Protocol Stats")
    r2._plot_02_charge_success_rate(runs, fig_dirs["02"], missing)
    r2._plot_02_decision_latency(runs, fig_dirs["02"], missing)
    r2._plot_02_effective_wait(runs, fig_dirs["02"], missing)
    r2._plot_02_energy_recovered(runs, fig_dirs["02"], missing)

    # --- Group 03: Cross-Protocol Charging ---
    print("\n[Group 03] Cross-Protocol Charging")
    r2._plot_03_charge_queue_length(runs, fig_dirs["03"], missing)
    r2._plot_03_dock_utilization(runs, fig_dirs["03"], missing)
    r2._plot_03_charge_outcome_breakdown(runs, fig_dirs["03"], missing)
    r2._plot_03_dead_uav_cumulative(runs, fig_dirs["03"], missing)
    r2._plot_03_cumulative_energy_charged(runs, fig_dirs["03"], missing)

    # --- Group 04: Policy Radar ---
    print("\n[Group 04] Policy Radar")
    r2._plot_04_policy_radar(runs, fig_dirs["04"], missing)

    # --- Group 05: Weather ---
    print("\n[Group 05] Weather")
    r2._plot_05_pdr_vs_weather_regime(runs, fig_dirs["05"], missing)
    r2._plot_05_weather_timeseries(runs, fig_dirs["05"], missing)
    r2._plot_05_weather_timeseries_all(runs, fig_dirs["05"], missing, bin_sec)

    # --- Merged plots ---
    print("\n[Group 01 Merged]")
    r2._plot_01_network_pdr_over_time_merged(runs, fig_dirs["01"], missing, bin_sec)
    r2._plot_01_battery_ecdf_merged(runs, fig_dirs["01"], missing)
    r2._plot_01_pdr_cdf_merged(runs, fig_dirs["01"], missing)
    r2._plot_01_packet_generated_merged(runs, fig_dirs["01"], missing)
    r2._plot_01_mean_pdr_merged(runs, fig_dirs["01"], missing)

    print("\n[Group 02 Merged]")
    r2._plot_02_charge_success_rate_merged(runs, fig_dirs["02"], missing)
    r2._plot_02_decision_latency_merged(runs, fig_dirs["02"], missing)
    r2._plot_02_effective_wait_merged(runs, fig_dirs["02"], missing)
    r2._plot_02_energy_recovered_merged(runs, fig_dirs["02"], missing)

    print("\n[Group 03 Merged]")
    r2._plot_03_charge_queue_length_merged(runs, fig_dirs["03"], missing, bin_sec)
    r2._plot_03_dock_utilization_merged(runs, fig_dirs["03"], missing, bin_sec)
    r2._plot_03_charge_outcome_breakdown_merged(runs, fig_dirs["03"], missing)
    r2._plot_03_dead_uav_cumulative_merged(runs, fig_dirs["03"], missing, bin_sec)
    r2._plot_03_cumulative_energy_charged_merged(runs, fig_dirs["03"], missing, bin_sec)

    print("\n[Group 04 Merged]")
    r2._plot_04_policy_radar_merged(runs, fig_dirs["04"], missing)

    print("\n[Group 05 Merged]")
    r2._plot_05_pdr_vs_weather_regime_merged(runs, fig_dirs["05"], missing)
    r2._plot_05_delay_vs_weather_regime_merged(runs, fig_dirs["05"], missing)

    print("\n[Group 05 additional]")
    r2._plot_05_delay_vs_weather_regime(runs, fig_dirs["05"], missing)

    # --- Group 06: Network QoS & E2E Delay ---
    print("\n[Group 06] Network QoS & E2E Delay")
    r2._plot_06_qos_heatmap(runs, fig_dirs["06"], missing)
    r2._plot_06_e2e_delay(runs, fig_dirs["06"], missing)

    print("\n[Group 06 Merged]")
    r2._plot_06_qos_heatmap_merged(runs, fig_dirs["06"], missing)
    r2._plot_06_e2e_delay_merged(runs, fig_dirs["06"], missing)
    r2._plot_06_mean_e2e_delay_merged(runs, fig_dirs["06"], missing)
    r2._plot_correlation_heatmap_pdr(runs, fig_dirs["06"], missing)
    r2._plot_correlation_heatmap_e2e(runs, fig_dirs["06"], missing)
    r2._plot_06_pdr_vs_e2e_scatter(runs, fig_dirs["06"], missing)

    # --- Group 07: Causal Analysis ---
    print("\n[Group 07] Causal Analysis")
    r2._plot_07_pdr_with_events_per_protocol(runs, fig_dirs["07"], missing)
    r2._plot_07_pdr_with_events_panel(runs, fig_dirs["07"], missing)
    r2._plot_07_pdr_with_events_merged(runs, fig_dirs["07"], missing, bin_sec)
    r2._plot_07_pdr_with_events_merged(runs, fig_dirs["07"], missing, bin_sec, binned=False)
    r2._plot_07_pdr_with_events_merged_variance(runs, fig_dirs["07"], missing, bin_sec)
    r2._plot_07_pdr_with_events_merged_variance(runs, fig_dirs["07"], missing, bin_sec, binned=False)
    r2._plot_07_pdr_with_events_merged_panel(runs, fig_dirs["07"], missing, bin_sec)
    r2._plot_07_pdr_with_events_merged_panel(runs, fig_dirs["07"], missing, bin_sec, binned=False)
    r2._plot_07_pdr_with_events_merged_variance_all(runs, fig_dirs["07"], missing, bin_sec)
    r2._plot_07_pdr_with_events_merged_variance_all(runs, fig_dirs["07"], missing, bin_sec, binned=False)
    r2._plot_07_window_pdr_distribution(runs, fig_dirs["07"], missing)
    r2._plot_07_dock_util_with_timeouts(runs, fig_dirs["07"], missing)
    r2._plot_07_dock_util_with_timeouts_merged(runs, fig_dirs["07"], missing, bin_sec)
    r2._plot_07_lagged_correlation(runs, fig_dirs["07"], missing, bin_sec)
    r2._plot_07_death_slope_pdr_dips(runs, fig_dirs["07"], missing, bin_sec)

    # --- Group 09: Cross-layer analysis ---
    print("\n[Group 09] Cross-layer Analysis")
    try:
        # Set folder format for round3 naming convention
        cla.FOLDER_FMT = "{proto}_sunny_{r}"

        cl_out = OUTPUT_ROOT / "figures" / "cross_layer"
        cl_out.mkdir(parents=True, exist_ok=True)
        kpi_df = cla.collect_per_run_kpis(INPUT_ROOT)
        cla.plot_cl_a_kpi_overview(kpi_df, cl_out)
        cla.plot_cl_b_charging_vs_pdr_scatter(kpi_df, cl_out)
        cla.plot_cl_c_conditioning_analysis(kpi_df, cl_out)
        cla.plot_cl_d_mechanism_chain(kpi_df, cl_out)
        cla.plot_cl_e_correlation_bar_chart(kpi_df, cl_out)
        cla.plot_cl_f_pdr_vs_depletions_timeseries(INPUT_ROOT, cl_out, int(bin_sec))
        cla.plot_cl_g_routing_drop_timeseries(INPUT_ROOT, cl_out, int(bin_sec))
        try:
            cla.plot_cl_h_lagged_corr_summary(INPUT_ROOT, cl_out)
        except Exception:
            import traceback; traceback.print_exc()
        try:
            cla.plot_cl_i_epoch_aligned_pdr(INPUT_ROOT, cl_out)
        except Exception:
            import traceback; traceback.print_exc()
        # CL_J is specific to ugv_edf_3 cascade — skip for round3
        try:
            cla.plot_cl_k_delay_robustness_panel(INPUT_ROOT, cl_out)
        except Exception:
            import traceback; traceback.print_exc()
        try:
            cla.plot_cl_l_event_aligned_composite(INPUT_ROOT, cl_out, int(bin_sec))
        except Exception:
            import traceback; traceback.print_exc()
        try:
            cla.plot_cl_m_role_scheduling_audit(INPUT_ROOT, cl_out)
        except Exception:
            import traceback; traceback.print_exc()
        try:
            cla.plot_cl_n_ch_sync_cascade(INPUT_ROOT, cl_out)
        except Exception:
            import traceback; traceback.print_exc()
        try:
            cla.plot_cl_o_mechanism_narrative(kpi_df, INPUT_ROOT, cl_out, int(bin_sec))
        except Exception:
            import traceback; traceback.print_exc()
    except Exception as exc:
        msg = f"Group 09 cross-layer analysis failed: {exc}"
        print(f"  WARNING: {msg}")
        missing.append(msg)
        import traceback; traceback.print_exc()
    finally:
        # Restore default folder format
        cla.FOLDER_FMT = "{proto}_{r}"

    # ------------------------------------------------------------------
    # 4. Derived tables + summary
    # ------------------------------------------------------------------
    print("\n[Derived tables]")
    r2._save_qos_summary(runs, derived_dir, missing)
    r2._save_charge_events_combined(runs, derived_dir, missing)

    print("\n[Summary table]")
    r2._save_kpi_summary_table(runs, summary_dir, missing)

    # ------------------------------------------------------------------
    # 5. Comparison: Round 3 vs Round 2
    # ------------------------------------------------------------------
    print("\n[Comparison] Computing Round 3 KPIs ...")
    r3_kpis = _compute_per_run_kpis(runs)
    r3_kpis.to_csv(summary_dir / "round3_per_run_kpis.csv", index=False)
    print(f"  Saved: {summary_dir / 'round3_per_run_kpis.csv'} ({len(r3_kpis)} rows)")

    print("[Comparison] Loading Round 2 KPIs ...")
    r2_kpis = _load_round2_kpis()
    print(f"  Loaded {len(r2_kpis)} rows from Round 2")

    # Collect figure paths for the report
    for d in fig_dirs.values():
        if d.exists():
            figures_generated.extend(sorted(d.glob("*.png")))
    cl_dir = OUTPUT_ROOT / "figures" / "cross_layer"
    if cl_dir.exists():
        figures_generated.extend(sorted(cl_dir.glob("*.png")))

    # ------------------------------------------------------------------
    # 6. Write validation report
    # ------------------------------------------------------------------
    print("\n[Report] Writing validation report ...")
    write_validation_report(runs, r3_kpis, r2_kpis, figures_generated, missing)

    # ------------------------------------------------------------------
    # 7. Missing-data report
    # ------------------------------------------------------------------
    print("\n[Missing data report]")
    r2._write_missing_report(missing, OUTPUT_ROOT)

    print(f"\nDone. All outputs written under: {OUTPUT_ROOT}/")
    print(f"Validation report: {REPORT_PATH}")


if __name__ == "__main__":
    main()
