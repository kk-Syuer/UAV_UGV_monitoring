#!/usr/bin/env python3
"""
plot_cross_layer_analysis.py
----------------------------
Produces cross-layer analysis figures that link UAV charging scheduling KPIs
to network quality metrics (PDR, E2E delay) and fleet survivability.

This script is part of the Group 09 (cross-layer) analysis for Test Round 2.

Outputs (saved to <output_root>/figures/cross_layer/):
    CL_A_protocol_kpi_overview.png           -- Bar chart: PDR, depletions, success rate
    CL_B_charging_vs_pdr_scatter.png         -- Scatter matrix: charging KPIs vs PDR
    CL_C_e2e_delay_conditioning_analysis.png -- Conditioning bias analysis
    CL_D_mechanism_chain.png                 -- Three-step mechanism chain scatter
    CL_E_correlation_bar_chart.png           -- Pearson + Spearman bar chart
    CL_F_pdr_vs_depletions_timeseries.png    -- PDR timeseries + depletion rate overlay
    CL_G_routing_drop_timeseries.png         -- Routing drop rate over time per protocol
    CL_H_lagged_correlation_summary.png      -- §4.4: cross-protocol lag-correlation curves
    CL_I_epoch_aligned_pdr.png               -- §4.4: epoch-aligned PDR around depletion bursts
    CL_J_ugv_edf3_cascade.png               -- §4.4/Anomaly A: ugv_edf_3 cascade timeseries
    CL_K_delay_robustness_panel.png         -- Conditioning-aware delay: sample-size scatter + DPR-weighted ranking
    CL_L_event_aligned_composite.png        -- Event-aligned 4-pane composite per protocol + merged
    CL_M_role_scheduling_audit.png          -- Role-stratified scheduling audit: the CH priority paradox
    CL_N_ch_sync_cascade.png                -- CH depletion synchronization → priority congestion → member cascade
    CL_O_mechanism_narrative.png            -- §4.2 narrative: flow diagram + temporal evidence (best vs role-based)

Data sources (all per-run CSVs):
    qos_metrics.csv          -> PDR (generated, delivered)
    network_timeseries.csv   -> window_pdr, window_delay_mean_ms, t_rel_s
    charge_events.csv        -> outcome, decision_latency_ms, effective_wait_ms
    death_events.csv         -> t_rel_s, role (1=CH, 0=member)
    charge_session_events.csv-> energy_charged_wh
    charge_queue_timeseries.csv -> ugv_dock_utilization, queue_length_total

Usage
-----
    python scripts/plot_cross_layer_analysis.py \\
        [--input  experiment_data_collection/test_round2] \\
        [--output analysis/test_round2] \\
        [--bin-seconds 600]

Called from run_all_plots_test_round2.py when --cross-layer flag is set.
"""

import argparse
import os
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from scipy import stats


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------
PROTOCOLS = [
    "ugv_dynamic", "ugv_edf", "ugv_fcfs", "ugv_p_dynamic_score",
    "ugv_p_edf", "ugv_p_role_priority", "ugv_role_priority",
]

# Template for per-run folder names inside data_root.
# Default "{proto}_{r}" works for test_round2  (e.g. ugv_dynamic_1).
# Override to "{proto}_sunny_{r}" for test_round3.
FOLDER_FMT = "{proto}_{r}"

def _run_dir(data_root: Path, proto: str, r: int) -> Path:
    """Return the folder path for a given (protocol, replicate) combo."""
    return data_root / FOLDER_FMT.format(proto=proto, r=r)

PROTO_LABELS = {
    "ugv_dynamic": "Dynamic",
    "ugv_edf": "EDF",
    "ugv_fcfs": "FCFS",
    "ugv_p_dynamic_score": "P-Dynamic",
    "ugv_p_edf": "P-EDF",
    "ugv_p_role_priority": "P-RolePrio",
    "ugv_role_priority": "RolePrio",
}
_COLORS = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b', '#e377c2']
PROTO_COLORS = {p: _COLORS[i] for i, p in enumerate(PROTOCOLS)}
REPLICATES = [1, 2, 3]
MAX_T_S = 180 * 60  # 180-minute runs


# ---------------------------------------------------------------------------
# Data loading helpers
# ---------------------------------------------------------------------------

def _load_csv(folder: Path, fname: str) -> pd.DataFrame:
    """Return a DataFrame or empty DataFrame on failure."""
    fp = folder / fname
    if not fp.exists():
        return pd.DataFrame()
    try:
        return pd.read_csv(fp)
    except Exception:
        return pd.DataFrame()


def collect_per_run_kpis(data_root: Path) -> pd.DataFrame:
    """
    Build a (21 × n_metrics) DataFrame of scalar KPIs, one row per run.

    Returns a DataFrame with columns:
        protocol, run, pdr, e2e_delay_mean, charge_success_rate,
        charge_timeout_rate, charge_routing_drop_rate, n_timeout, n_started,
        total_requests, decision_latency_mean, effective_wait_mean,
        total_deaths, ch_deaths, energy_per_session, total_energy, n_sessions,
        dock_util_mean, queue_length_mean
    """
    rows = []
    for proto in PROTOCOLS:
        for r in REPLICATES:
            folder = _run_dir(data_root, proto, r)
            row: dict = {"protocol": proto, "run": r}

            # PDR -------------------------------------------------------
            qos = _load_csv(folder, "qos_metrics.csv")
            if not qos.empty and "generated" in qos.columns:
                gen = qos["generated"].sum()
                dlv = qos["delivered"].sum()
                row["pdr"] = dlv / gen if gen > 0 else np.nan
            else:
                row["pdr"] = np.nan

            # E2E delay -------------------------------------------------
            net = _load_csv(folder, "network_timeseries.csv")
            if not net.empty and "window_delay_mean_ms" in net.columns:
                pos = net[net["window_delay_mean_ms"] >= 0]
                row["e2e_delay_mean"] = pos["window_delay_mean_ms"].mean()
            else:
                row["e2e_delay_mean"] = np.nan

            # Charge events ---------------------------------------------
            ce = _load_csv(folder, "charge_events.csv")
            if not ce.empty and "outcome" in ce.columns:
                total = len(ce)
                row["charge_success_rate"] = (ce["outcome"] == "STARTED").sum() / total
                row["charge_timeout_rate"] = (ce["outcome"] == "TIMEOUT").sum() / total
                row["charge_routing_drop_rate"] = (
                    ce["outcome"] == "ROUTING_DROP"
                ).sum() / total
                row["n_timeout"] = int((ce["outcome"] == "TIMEOUT").sum())
                row["n_started"] = int((ce["outcome"] == "STARTED").sum())
                row["total_requests"] = total
                dl = ce[ce["decision_latency_ms"] > 0]["decision_latency_ms"]
                row["decision_latency_mean"] = dl.mean() if len(dl) > 0 else np.nan
                ew = ce[ce["effective_wait_ms"] > 0]["effective_wait_ms"]
                row["effective_wait_mean"] = ew.mean() if len(ew) > 0 else np.nan
            else:
                row.update({
                    "charge_success_rate": np.nan, "charge_timeout_rate": np.nan,
                    "charge_routing_drop_rate": np.nan, "n_timeout": np.nan,
                    "n_started": np.nan, "total_requests": np.nan,
                    "decision_latency_mean": np.nan, "effective_wait_mean": np.nan,
                })

            # Death events ----------------------------------------------
            de = _load_csv(folder, "death_events.csv")
            if not de.empty:
                row["total_deaths"] = len(de)
                if "role" in de.columns:
                    row["ch_deaths"] = (de["role"] == 1).sum()
                else:
                    row["ch_deaths"] = np.nan
            else:
                row["total_deaths"] = np.nan
                row["ch_deaths"] = np.nan

            # Charge session energy -------------------------------------
            cse = _load_csv(folder, "charge_session_events.csv")
            if not cse.empty and "energy_charged_wh" in cse.columns:
                valid = cse[cse["energy_charged_wh"] > 0]["energy_charged_wh"]
                row["energy_per_session"] = valid.mean() if len(valid) > 0 else np.nan
                row["total_energy"] = valid.sum()
                row["n_sessions"] = len(valid)
            else:
                row["energy_per_session"] = np.nan
                row["total_energy"] = np.nan
                row["n_sessions"] = np.nan

            # Dock utilization / queue length ---------------------------
            cq = _load_csv(folder, "charge_queue_timeseries.csv")
            if not cq.empty:
                if "ugv_dock_utilization" in cq.columns:
                    valid_dock = cq[cq["ugv_dock_utilization"] >= 0]["ugv_dock_utilization"]
                    row["dock_util_mean"] = valid_dock.mean()
                else:
                    row["dock_util_mean"] = np.nan
                if "queue_length_total" in cq.columns:
                    row["queue_length_mean"] = cq["queue_length_total"].mean()
                else:
                    row["queue_length_mean"] = np.nan
            else:
                row["dock_util_mean"] = np.nan
                row["queue_length_mean"] = np.nan

            rows.append(row)

    return pd.DataFrame(rows)


# ---------------------------------------------------------------------------
# Individual plot functions
# ---------------------------------------------------------------------------

def plot_cl_a_kpi_overview(df: pd.DataFrame, out_dir: Path) -> None:
    """Bar chart: PDR, depletions, and charge success rate per protocol."""
    fig, axes = plt.subplots(1, 3, figsize=(15, 5))
    numeric_cols = df.select_dtypes(include="number").columns
    mean_df = df.groupby("protocol")[numeric_cols].mean()
    std_df = df.groupby("protocol")[numeric_cols].std()
    x = np.arange(len(PROTOCOLS))
    short_labels = [PROTO_LABELS[p] for p in PROTOCOLS]
    bar_colors = [PROTO_COLORS[p] for p in PROTOCOLS]

    for ax, metric, title, higher_better in [
        (axes[0], "pdr", "Mean PDR (higher = better)", True),
        (axes[1], "total_deaths", "Mean Depletion Events (lower = better)", False),
        (axes[2], "charge_success_rate", "Charge Success Rate (higher = better)", True),
    ]:
        vals = [mean_df.loc[p, metric] for p in PROTOCOLS]
        errs = [std_df.loc[p, metric] for p in PROTOCOLS]
        bars = ax.bar(x, vals, 0.6, yerr=errs, color=bar_colors, capsize=4, alpha=0.85)
        ax.set_xticks(x)
        ax.set_xticklabels(short_labels, rotation=45, ha="right", fontsize=9)
        ax.set_title(title, fontsize=10, fontweight="bold")
        ax.grid(axis="y", alpha=0.3)
        best_idx = int(np.nanargmax(vals)) if higher_better else int(np.nanargmin(vals))
        bars[best_idx].set_edgecolor("gold")
        bars[best_idx].set_linewidth(2.5)

    fig.suptitle(
        "Cross-Protocol KPI Overview\n(error bars = 1 SD across 3 replicates)",
        fontsize=12, fontweight="bold",
    )
    plt.tight_layout()
    _save(fig, out_dir / "CL_A_protocol_kpi_overview.png")


def plot_cl_b_charging_vs_pdr_scatter(df: pd.DataFrame, out_dir: Path) -> None:
    """2×3 scatter matrix: charging KPIs vs PDR with regression lines."""
    fig, axes = plt.subplots(2, 3, figsize=(15, 9))
    axes = axes.flatten()

    pairs = [
        ("charge_success_rate", "pdr", "Charge Success Rate", "PDR"),
        ("charge_routing_drop_rate", "pdr", "Routing Drop Rate", "PDR"),
        ("total_deaths", "pdr", "Battery Depletions", "PDR"),
        ("energy_per_session", "pdr", "Energy per Session (Wh)", "PDR"),
        ("dock_util_mean", "pdr", "Mean Dock Utilization", "PDR"),
        ("effective_wait_mean", "pdr", "Mean Effective Wait (ms)", "PDR"),
    ]

    for ax, (xvar, yvar, xlabel, ylabel) in zip(axes, pairs):
        sub = df[[xvar, yvar, "protocol"]].dropna()
        for proto in PROTOCOLS:
            pts = sub[sub["protocol"] == proto]
            ax.scatter(
                pts[xvar], pts[yvar],
                color=PROTO_COLORS[proto], label=PROTO_LABELS[proto],
                s=60, alpha=0.85, zorder=3,
            )
        if len(sub) >= 4:
            pr, pp = stats.pearsonr(sub[xvar], sub[yvar])
            sr, sp = stats.spearmanr(sub[xvar], sub[yvar])
            m, b, *_ = stats.linregress(sub[xvar], sub[yvar])
            xl = np.linspace(sub[xvar].min(), sub[xvar].max(), 100)
            ax.plot(xl, m * xl + b, "k--", alpha=0.5, linewidth=1.2)
            sig = ("***" if min(pp, sp) < 0.001
                   else ("**" if min(pp, sp) < 0.01
                         else ("*" if min(pp, sp) < 0.05 else "n.s.")))
            ax.set_title(
                f"r = {pr:.2f} (P),  ρ = {sr:.2f} (S)  {sig}",
                fontsize=9, fontweight="bold",
            )
        ax.set_xlabel(xlabel, fontsize=9)
        ax.set_ylabel(ylabel, fontsize=9)
        ax.grid(alpha=0.3)

    handles = [
        plt.scatter([], [], color=PROTO_COLORS[p], label=PROTO_LABELS[p])
        for p in PROTOCOLS
    ]
    axes[-1].legend(
        handles=handles, title="Protocol",
        fontsize=8, title_fontsize=9, loc="center", framealpha=0.9,
    )

    fig.suptitle(
        "Cross-Layer Scatter: Charging KPIs vs Network PDR\n"
        "(n = 21 runs; r = Pearson, ρ = Spearman)",
        fontsize=12, fontweight="bold",
    )
    plt.tight_layout()
    _save(fig, out_dir / "CL_B_charging_vs_pdr_scatter.png")


def plot_cl_c_conditioning_analysis(df: pd.DataFrame, out_dir: Path) -> None:
    """E2E delay conditioning bias: scatter + bubble chart."""
    fig, axes = plt.subplots(1, 2, figsize=(14, 5))

    # Left — E2E delay vs PDR (raw run scatter)
    ax = axes[0]
    for proto in PROTOCOLS:
        pts = df[df["protocol"] == proto]
        ax.scatter(
            pts["pdr"], pts["e2e_delay_mean"],
            color=PROTO_COLORS[proto], label=PROTO_LABELS[proto],
            s=80, alpha=0.85, zorder=3,
        )
    sub = df[["pdr", "e2e_delay_mean"]].dropna()
    if len(sub) >= 4:
        pr, _ = stats.pearsonr(sub["pdr"], sub["e2e_delay_mean"])
        ax.set_title(
            f"E2E Delay vs PDR\n(Pearson r = {pr:.2f}; conditioning bias likely)",
            fontsize=10, fontweight="bold",
        )
    ax.set_xlabel("PDR (packet delivery ratio)", fontsize=10)
    ax.set_ylabel("Mean E2E Delay (ms)", fontsize=10)
    ax.legend(fontsize=8, title="Protocol")
    ax.grid(alpha=0.3)

    # Annotate the ugv_edf_3 outlier if present
    edf3 = df[(df["protocol"] == "ugv_edf") & (df["run"] == 3)]
    if not edf3.empty:
        ax.annotate(
            "ugv_edf_3\n(85 depletions,\nbias artifact)",
            xy=(float(edf3["pdr"].values[0]), float(edf3["e2e_delay_mean"].values[0])),
            xytext=(0.54, 55), fontsize=7, color="red",
            arrowprops=dict(arrowstyle="->", color="red", lw=1),
        )

    # Right — protocol means bubble chart (bubble ∝ depletions)
    ax2 = axes[1]
    proto_means = df.groupby("protocol")[["pdr", "e2e_delay_mean", "total_deaths"]].mean()
    for proto in PROTOCOLS:
        row = proto_means.loc[proto]
        size = max(10, float(row["total_deaths"]) * 10)
        ax2.scatter(
            float(row["pdr"]), float(row["e2e_delay_mean"]),
            s=size, color=PROTO_COLORS[proto], alpha=0.8, zorder=3,
        )
        ax2.annotate(
            PROTO_LABELS[proto],
            (float(row["pdr"]), float(row["e2e_delay_mean"])),
            fontsize=8, xytext=(3, 3), textcoords="offset points",
        )
    ax2.set_xlabel("Mean PDR", fontsize=10)
    ax2.set_ylabel("Mean E2E Delay (ms)", fontsize=10)
    ax2.set_title(
        "Protocol Means: PDR vs E2E Delay\n(bubble size ∝ mean battery depletions)",
        fontsize=10, fontweight="bold",
    )
    ax2.grid(alpha=0.3)
    ax2.text(
        0.02, 0.98,
        "Note: delay computed on DELIVERED packets only.\n"
        "Low PDR → fewer hops → apparently lower delay.",
        transform=ax2.transAxes, fontsize=7, va="top",
        bbox=dict(boxstyle="round", facecolor="wheat", alpha=0.7),
    )

    fig.suptitle("E2E Delay Conditioning Bias Analysis", fontsize=12, fontweight="bold")
    plt.tight_layout()
    _save(fig, out_dir / "CL_C_e2e_delay_conditioning_analysis.png")


def plot_cl_d_mechanism_chain(df: pd.DataFrame, out_dir: Path) -> None:
    """Three-panel scatter: scheduling → depletions → network quality."""
    fig, axes = plt.subplots(1, 3, figsize=(15, 5))

    panel_specs = [
        ("charge_success_rate", "total_deaths",
         "Charge Success Rate", "Battery Depletions",
         "Charging → Fleet Survival"),
        ("total_deaths", "pdr",
         "Battery Depletions", "PDR",
         "Fleet Survival → Network PDR"),
        ("charge_routing_drop_rate", "pdr",
         "Charge Req. ROUTING_DROP Rate", "PDR",
         "Routing Drops → Network PDR"),
    ]

    for ax, (xvar, yvar, xlabel, ylabel, title) in zip(axes, panel_specs):
        sub = df[[xvar, yvar, "protocol"]].dropna()
        for proto in PROTOCOLS:
            pts = sub[sub["protocol"] == proto]
            ax.scatter(
                pts[xvar], pts[yvar],
                color=PROTO_COLORS[proto], label=PROTO_LABELS[proto],
                s=70, alpha=0.85, zorder=3,
            )
        if len(sub) >= 4:
            pr, pp = stats.pearsonr(sub[xvar], sub[yvar])
            sr, sp = stats.spearmanr(sub[xvar], sub[yvar])
            m, b, *_ = stats.linregress(sub[xvar], sub[yvar])
            xl = np.linspace(sub[xvar].min(), sub[xvar].max(), 100)
            ax.plot(xl, m * xl + b, "k--", alpha=0.5, linewidth=1.2)
            ax.set_title(
                f"{title}\nr = {pr:.2f}, ρ = {sr:.2f}\np = {pp:.4f}, {sp:.4f}",
                fontsize=9, fontweight="bold",
            )
        ax.set_xlabel(xlabel, fontsize=9)
        ax.set_ylabel(ylabel, fontsize=9)
        ax.grid(alpha=0.3)

    handles = [
        plt.scatter([], [], color=PROTO_COLORS[p], label=PROTO_LABELS[p])
        for p in PROTOCOLS
    ]
    axes[0].legend(handles=handles, title="Protocol", fontsize=7, title_fontsize=8)

    fig.suptitle(
        "Cross-Layer Mechanism: Charging → Survivability → Network\n"
        "(Three-step causal chain; n = 21 runs)",
        fontsize=12, fontweight="bold",
    )
    plt.tight_layout()
    _save(fig, out_dir / "CL_D_mechanism_chain.png")


def plot_cl_e_correlation_bar_chart(df: pd.DataFrame, out_dir: Path) -> None:
    """Side-by-side Pearson + Spearman bar charts for PDR and E2E delay."""
    kpi_keys = [
        "charge_success_rate", "charge_timeout_rate", "charge_routing_drop_rate",
        "decision_latency_mean", "effective_wait_mean", "total_deaths",
        "energy_per_session", "dock_util_mean",
    ]
    kpi_display = [
        "Success Rate", "Timeout Rate", "Routing Drop Rate",
        "Decision Latency", "Effective Wait", "Depletions",
        "Energy/Session", "Dock Util",
    ]
    net_vars = [("pdr", "PDR"), ("e2e_delay_mean", "E2E Delay")]

    fig, axes = plt.subplots(1, 2, figsize=(13, 5))

    for ax_idx, (nv, nv_label) in enumerate(net_vars):
        ax = axes[ax_idx]
        pearson_rs, spearman_rs = [], []
        for kv in kpi_keys:
            sub = df[[nv, kv]].dropna()
            if len(sub) < 5:
                pearson_rs.append(np.nan)
                spearman_rs.append(np.nan)
            else:
                pr, _ = stats.pearsonr(sub[nv], sub[kv])
                sr, _ = stats.spearmanr(sub[nv], sub[kv])
                pearson_rs.append(pr)
                spearman_rs.append(sr)

        x = np.arange(len(kpi_keys))
        width = 0.35
        ax.bar(x - width / 2, pearson_rs, width, label="Pearson r", alpha=0.8, color="steelblue")
        ax.bar(x + width / 2, spearman_rs, width, label="Spearman ρ", alpha=0.8, color="coral")
        ax.axhline(0, color="black", linewidth=0.8)
        ax.axhline(0.3, color="gray", linewidth=0.5, linestyle="--", alpha=0.5)
        ax.axhline(-0.3, color="gray", linewidth=0.5, linestyle="--", alpha=0.5)
        ax.set_xticks(x)
        ax.set_xticklabels(kpi_display, rotation=45, ha="right", fontsize=8)
        ax.set_ylabel("Correlation coefficient", fontsize=9)
        ax.set_title(f"Correlations with {nv_label}", fontsize=10, fontweight="bold")
        ax.legend(fontsize=8)
        ax.set_ylim(-0.75, 0.75)
        ax.grid(axis="y", alpha=0.3)

    fig.suptitle(
        "Cross-Layer Correlation: Charging KPIs vs Network Metrics\n"
        "(n = 21 runs; dashed lines = |r| = 0.3 reference)",
        fontsize=12, fontweight="bold",
    )
    plt.tight_layout()
    _save(fig, out_dir / "CL_E_correlation_bar_chart.png")


def plot_cl_f_pdr_vs_depletions_timeseries(
    data_root: Path, out_dir: Path, bin_sec: int = 600
) -> None:
    """Mean PDR timeseries + depletion rate overlay per protocol (stacked subplots)."""
    bins = np.arange(0, MAX_T_S + bin_sec, bin_sec)
    bin_centers_min = (bins[:-1] + bins[1:]) / 2 / 60

    fig, axes = plt.subplots(len(PROTOCOLS), 1, figsize=(14, 3 * len(PROTOCOLS)), sharex=True)

    for i, proto in enumerate(PROTOCOLS):
        ax = axes[i]
        ax_twin = ax.twinx()

        pdr_grids, death_bins_list = [], []
        for r in REPLICATES:
            net = _load_csv(_run_dir(data_root, proto, r), "network_timeseries.csv")
            if not net.empty and "window_pdr" in net.columns:
                net = net[net["window_pdr"] >= 0]
                pdr_binned = []
                for b0, b1 in zip(bins[:-1], bins[1:]):
                    seg = net[(net["t_rel_s"] >= b0) & (net["t_rel_s"] < b1)]
                    pdr_binned.append(seg["window_pdr"].mean() if len(seg) > 0 else np.nan)
                pdr_grids.append(pdr_binned)

            de = _load_csv(_run_dir(data_root, proto, r), "death_events.csv")
            if not de.empty and "t_rel_s" in de.columns:
                death_bins_list.append(np.histogram(de["t_rel_s"].values, bins=bins)[0])

        if pdr_grids:
            mean_pdr = np.nanmean(pdr_grids, axis=0)
            std_pdr = np.nanstd(pdr_grids, axis=0)
            ax.plot(bin_centers_min, mean_pdr, color=PROTO_COLORS[proto], linewidth=2)
            ax.fill_between(
                bin_centers_min, mean_pdr - std_pdr, mean_pdr + std_pdr,
                alpha=0.2, color=PROTO_COLORS[proto],
            )
        ax.set_ylabel("PDR", fontsize=8)
        ax.set_ylim(-0.05, 1.1)
        mean_pdr_val = float(np.nanmean(mean_pdr)) if pdr_grids else float("nan")
        ax.set_title(
            f"{PROTO_LABELS[proto]}  (mean PDR = {mean_pdr_val:.3f})",
            fontsize=9, fontweight="bold", loc="left",
        )

        if death_bins_list:
            mean_deaths = np.mean(death_bins_list, axis=0)
            ax_twin.bar(bin_centers_min, mean_deaths, width=8, alpha=0.35, color="firebrick")
            ax_twin.set_ylim(0, max(10.0, float(mean_deaths.max()) * 3))
        ax_twin.set_ylabel("Depletions / bin", fontsize=7, color="firebrick")
        ax_twin.tick_params(axis="y", colors="firebrick")

    axes[-1].set_xlabel("Time (minutes)", fontsize=10)
    fig.suptitle(
        "Mean PDR Timeseries with Battery Depletion Rate\n"
        "(PDR: mean ± 1σ across 3 replicates; bars = mean depletions per bin)",
        fontsize=12, fontweight="bold",
    )
    plt.tight_layout()
    _save(fig, out_dir / "CL_F_pdr_vs_depletions_timeseries.png")


def plot_cl_g_routing_drop_timeseries(
    data_root: Path, out_dir: Path, bin_sec: int = 600
) -> None:
    """Charge request ROUTING_DROP rate over time, one line per protocol."""
    bins = np.arange(0, MAX_T_S + bin_sec, bin_sec)
    bin_centers_min = (bins[:-1] + bins[1:]) / 2 / 60

    fig, ax = plt.subplots(figsize=(12, 5))

    for proto in PROTOCOLS:
        rd_rates_all = []
        for r in REPLICATES:
            ce = _load_csv(_run_dir(data_root, proto, r), "charge_events.csv")
            if ce.empty or "outcome" not in ce.columns:
                continue
            bin_rates = []
            for b0, b1 in zip(bins[:-1], bins[1:]):
                seg = ce[(ce["t_rel_s"] >= b0) & (ce["t_rel_s"] < b1)]
                if len(seg) > 0:
                    bin_rates.append((seg["outcome"] == "ROUTING_DROP").mean())
                else:
                    bin_rates.append(np.nan)
            rd_rates_all.append(bin_rates)

        if rd_rates_all:
            mean_rd = np.nanmean(rd_rates_all, axis=0)
            ax.plot(
                bin_centers_min, mean_rd,
                color=PROTO_COLORS[proto], label=PROTO_LABELS[proto],
                linewidth=2, alpha=0.85,
            )

    ax.set_xlabel("Time (minutes)", fontsize=10)
    ax.set_ylabel("Charge Request ROUTING_DROP Rate", fontsize=10)
    ax.set_title(
        "Charge Request Routing Drop Rate Over Time\n"
        "(ROUTING_DROP = request lost in transit; caused by WEATHER_DROP events)",
        fontsize=11, fontweight="bold",
    )
    ax.legend(title="Protocol", fontsize=9, title_fontsize=10)
    ax.grid(alpha=0.3)
    ax.set_ylim(0, 1)
    plt.tight_layout()
    _save(fig, out_dir / "CL_G_routing_drop_timeseries.png")


# ---------------------------------------------------------------------------
# §4.4 Lagged-correlation illustration plots (CL_H, CL_I, CL_J)
# ---------------------------------------------------------------------------

def _lagged_pearson(x: np.ndarray, y: np.ndarray, max_lag: int):
    """Pearson r between x and y at integer lags [-max_lag, +max_lag].

    Positive lag = y lags behind x (x leads).
    Returns (lags_array, corrs_array).
    """
    x, y = np.asarray(x, float), np.asarray(y, float)
    n = len(x)
    lags = np.arange(-max_lag, max_lag + 1)
    rs = np.full(len(lags), np.nan)
    for i, lag in enumerate(lags):
        if lag > 0:
            xi, yi = x[: n - lag], y[lag:]
        elif lag < 0:
            xi, yi = x[-lag:], y[: n + lag]
        else:
            xi, yi = x, y
        valid = ~(np.isnan(xi) | np.isnan(yi))
        if valid.sum() < 5:
            continue
        xi, yi = xi[valid], yi[valid]
        denom = np.sqrt(np.var(xi) * np.var(yi))
        rs[i] = float(np.cov(xi, yi)[0, 1] / denom) if denom > 0 else np.nan
    return lags, rs


def _bin_mean(t: np.ndarray, v: np.ndarray, bins: np.ndarray) -> np.ndarray:
    """Return per-bin mean of v; np.nan when a bin is empty.

    Vectorised via np.searchsorted + np.bincount — avoids a per-bin Python
    loop that becomes very slow for large timeseries (>10 000 rows).
    """
    t = np.asarray(t, dtype=float)
    v = np.asarray(v, dtype=float)
    n_bins = len(bins) - 1
    out = np.full(n_bins, np.nan)
    bin_idx = np.searchsorted(bins, t, side="right") - 1  # 0-indexed
    valid = (bin_idx >= 0) & (bin_idx < n_bins) & ~np.isnan(v)
    if not valid.any():
        return out
    b = bin_idx[valid]
    w = v[valid]
    total = np.bincount(b, weights=w, minlength=n_bins)
    count = np.bincount(b, minlength=n_bins).astype(float)
    has_data = count > 0
    out[has_data] = total[has_data] / count[has_data]
    return out


def plot_cl_h_lagged_corr_summary(data_root: Path, out_dir: Path) -> None:
    """Cross-protocol lagged-correlation curves + peak-lag bar chart (§4.4).

    Left panel: Pearson r(PDR(t), depletions(t+lag)) vs lag (minutes) for every
    protocol, derived from all 3 replicates concatenated end-to-end.
    Right panel: horizontal bar chart of each protocol's peak negative r and the
    lag at which it occurs.

    Data: network_timeseries.csv (window_pdr), death_events.csv (t_rel_s).
    Output: CL_H_lagged_correlation_summary.png
    """
    bin_s = 60
    bins = np.arange(0, MAX_T_S + bin_s, bin_s)
    max_lag = 15  # ±15 bins = ±15 min

    fig, (ax_main, ax_peak) = plt.subplots(1, 2, figsize=(14, 5))

    peak_rows = []
    for proto in PROTOCOLS:
        pdr_chunks, death_chunks = [], []
        for r in REPLICATES:
            net = _load_csv(_run_dir(data_root, proto, r), "network_timeseries.csv")
            de = _load_csv(_run_dir(data_root, proto, r), "death_events.csv")
            if net.empty or de.empty:
                continue
            valid = net[net["window_pdr"] >= 0]
            pdr_b = _bin_mean(valid["t_rel_s"].values, valid["window_pdr"].values, bins)
            d_hist, _ = np.histogram(de["t_rel_s"].values, bins=bins)
            pdr_chunks.append(pdr_b)
            death_chunks.append(d_hist.astype(float))

        if not pdr_chunks:
            continue

        pdr_cat = np.concatenate(pdr_chunks)
        death_cat = np.concatenate(death_chunks)
        lags, rs = _lagged_pearson(pdr_cat, death_cat, max_lag)

        ax_main.plot(lags, rs, color=PROTO_COLORS[proto],
                     label=PROTO_LABELS[proto], linewidth=2, alpha=0.85)

        # Peak negative r (preferred) or absolute peak if none are negative.
        # Guard against all-NaN rs (np.nanargmax raises ValueError on all-NaN).
        finite_mask = ~np.isnan(rs)
        if not finite_mask.any():
            continue  # no valid correlations for this protocol — skip
        neg_mask = rs < 0
        if neg_mask.any():
            neg_rs, neg_lags = rs[neg_mask], lags[neg_mask]
            pk = int(np.argmin(neg_rs))
            peak_rows.append({
                "label": PROTO_LABELS[proto],
                "color": PROTO_COLORS[proto],
                "peak_r": float(neg_rs[pk]),
                "peak_lag": int(neg_lags[pk]),
            })
        else:
            finite_rs = rs[finite_mask]
            pk = int(np.argmax(np.abs(finite_rs)))
            peak_rows.append({
                "label": PROTO_LABELS[proto],
                "color": PROTO_COLORS[proto],
                "peak_r": float(finite_rs[pk]),
                "peak_lag": int(lags[finite_mask][pk]),
            })

    ax_main.axhline(0, color="black", linewidth=0.8)
    ax_main.axvline(0, color="gray", linewidth=0.6, linestyle="--", alpha=0.5)
    ax_main.axvspan(-10, -7, alpha=0.08, color="red", label="Expected lead window\n(−10 to −7 min)")
    ax_main.set_xlabel("Lag (minutes)\n[positive = depletions lag behind PDR]", fontsize=10)
    ax_main.set_ylabel("Pearson r  (PDR vs depletion rate)", fontsize=10)
    ax_main.set_title(
        "Lagged Cross-Correlation: PDR → Depletion Rate\n"
        "(3 replicates concatenated per protocol; 1-min bins)",
        fontsize=10, fontweight="bold",
    )
    ax_main.legend(fontsize=8, title="Protocol", loc="lower right")
    ax_main.grid(alpha=0.3)
    ax_main.set_xlim(-15, 15)

    # Right: peak-lag horizontal bar chart
    x = np.arange(len(peak_rows))
    bar_colors = [row["color"] for row in peak_rows]
    bars = ax_peak.barh(x, [row["peak_r"] for row in peak_rows],
                        color=bar_colors, alpha=0.85)
    ax_peak.set_yticks(x)
    ax_peak.set_yticklabels([row["label"] for row in peak_rows], fontsize=9)
    ax_peak.set_xlabel("Peak negative Pearson r", fontsize=10)
    ax_peak.set_title(
        "Peak Negative r & Lag\n(PDR drop leads depletions)",
        fontsize=10, fontweight="bold",
    )
    for bar, row in zip(bars, peak_rows):
        ax_peak.text(
            bar.get_width() - 0.005, bar.get_y() + bar.get_height() / 2,
            f"lag = {row['peak_lag']:+d} min",
            va="center", ha="right", fontsize=8, color="white", fontweight="bold",
        )
    ax_peak.axvline(0, color="black", linewidth=0.8)
    ax_peak.grid(axis="x", alpha=0.3)

    fig.suptitle(
        "§4.4 — PDR Drop Precedes Battery Depletion: Lagged Correlation Evidence",
        fontsize=12, fontweight="bold",
    )
    plt.tight_layout()
    _save(fig, out_dir / "CL_H_lagged_correlation_summary.png")


def plot_cl_i_epoch_aligned_pdr(data_root: Path, out_dir: Path) -> None:
    """Epoch-aligned mean PDR in a ±15 min window around depletion bursts (§4.4).

    For each 5-minute bin that contains ≥1 depletion event the PDR time series
    is extracted in a ±15 min window centred on that bin.  The mean trajectory
    (± 1σ across all bursts and replicates) is plotted per protocol, plus one
    combined panel.  The characteristic PDR dip *before* the depletion confirms
    the ~8–9 min lead time.

    Data: network_timeseries.csv (window_pdr), death_events.csv (t_rel_s).
    Output: CL_I_epoch_aligned_pdr.png
    """
    bin_s = 60
    epoch_half_s = 15 * 60  # ±15 min
    epoch_bins = np.arange(-epoch_half_s, epoch_half_s + bin_s, bin_s)
    n_epoch = len(epoch_bins) - 1
    burst_window_s = 300  # 5-min burst-detection window

    fig, axes = plt.subplots(2, 4, figsize=(16, 8), sharey=True, sharex=True)
    axes_flat = axes.flatten()

    all_epochs: list = []

    for ax_idx, proto in enumerate(PROTOCOLS):
        ax = axes_flat[ax_idx]
        proto_epochs: list = []

        for r in REPLICATES:
            net = _load_csv(_run_dir(data_root, proto, r), "network_timeseries.csv")
            de = _load_csv(_run_dir(data_root, proto, r), "death_events.csv")
            if net.empty or de.empty:
                continue
            valid = net[net["window_pdr"] >= 0].sort_values("t_rel_s")
            t_pdr = valid["t_rel_s"].values
            pdr_v = valid["window_pdr"].values

            # Identify burst bin centres
            burst_edges = np.arange(0, MAX_T_S + burst_window_s, burst_window_s)
            d_hist, _ = np.histogram(de["t_rel_s"].values, bins=burst_edges)
            burst_centers = [
                (b0 + b1) / 2
                for b0, b1, cnt in zip(burst_edges[:-1], burst_edges[1:], d_hist)
                if cnt >= 1
            ]

            for tc in burst_centers:
                # Vectorised: shift t_pdr by -tc so epoch_bins can be used directly
                t_rel = t_pdr - tc
                in_window = (t_rel >= epoch_bins[0]) & (t_rel < epoch_bins[-1])
                if not in_window.any():
                    continue
                epoch_pdr = _bin_mean(
                    t_rel[in_window], pdr_v[in_window], epoch_bins
                )
                proto_epochs.append(epoch_pdr)
                all_epochs.append(epoch_pdr)

        if not proto_epochs:
            ax.set_visible(False)
            continue

        epoch_arr = np.array(proto_epochs)
        mean_ep = np.nanmean(epoch_arr, axis=0)
        std_ep = np.nanstd(epoch_arr, axis=0)
        t_min = (epoch_bins[:-1] + epoch_bins[1:]) / 2 / 60

        ax.axvline(0, color="red", linewidth=1.2, linestyle="--", alpha=0.8)
        ax.fill_between(t_min, mean_ep - std_ep, mean_ep + std_ep,
                        alpha=0.2, color=PROTO_COLORS[proto])
        ax.plot(t_min, mean_ep, color=PROTO_COLORS[proto], linewidth=2)
        ax.set_title(
            f"{PROTO_LABELS[proto]}\n(n = {len(proto_epochs)} bursts)",
            fontsize=9, fontweight="bold",
        )
        ax.set_ylim(0, 1)
        ax.axhspan(0, 0.5, alpha=0.04, color="red")
        ax.grid(alpha=0.3)
        if ax_idx >= 4:
            ax.set_xlabel("Time relative to depletion (min)", fontsize=8)
        if ax_idx % 4 == 0:
            ax.set_ylabel("Mean PDR", fontsize=8)

    # Last panel: cross-protocol average
    ax_all = axes_flat[7]
    if all_epochs:
        all_arr = np.array(all_epochs)
        mean_all = np.nanmean(all_arr, axis=0)
        std_all = np.nanstd(all_arr, axis=0)
        t_min = (epoch_bins[:-1] + epoch_bins[1:]) / 2 / 60
        ax_all.axvline(0, color="red", linewidth=1.5, linestyle="--", label="Depletion burst")
        ax_all.fill_between(t_min, mean_all - std_all, mean_all + std_all,
                            alpha=0.2, color="black")
        ax_all.plot(t_min, mean_all, color="black", linewidth=2.5, label="All-protocol mean")
        ax_all.set_title(
            f"All protocols\n(n = {len(all_epochs)} bursts total)",
            fontsize=9, fontweight="bold",
        )
        ax_all.set_ylim(0, 1)
        ax_all.legend(fontsize=7)
        ax_all.grid(alpha=0.3)
        ax_all.set_xlabel("Time relative to depletion (min)", fontsize=8)

    fig.suptitle(
        "§4.4 — Epoch-Aligned PDR Around Battery Depletion Bursts\n"
        "(mean ± 1σ; red dashed = depletion event; window = ±15 min; 5-min burst bins)",
        fontsize=12, fontweight="bold",
    )
    plt.tight_layout()
    _save(fig, out_dir / "CL_I_epoch_aligned_pdr.png")


def plot_cl_j_ugv_edf3_cascade(data_root: Path, out_dir: Path) -> None:
    """Annotated cascade timeseries for ugv_edf run 3 (§4.4 / §5 Anomaly A).

    Three overlaid series on dual Y-axes:
      - PDR (2-min bins, blue)          — left axis
      - Charge request ROUTING_DROP rate (2-min bins, red dashed) — left axis
      - Cumulative depletion events (step, black)                 — right axis

    Key events annotated:
      - t ≈ 6.6 min: simultaneous dual-CH depletion (cascade trigger)
      - Shaded region after fleet alive_count drops to 0 (≈ 33 min)

    Data: network_timeseries.csv, charge_events.csv, death_events.csv (ugv_edf_3).
    Output: CL_J_ugv_edf3_cascade.png
    """
    folder = data_root / "ugv_edf_3"
    net = _load_csv(folder, "network_timeseries.csv")
    ce = _load_csv(folder, "charge_events.csv")
    de = _load_csv(folder, "death_events.csv")

    if net.empty or ce.empty or de.empty:
        print("  WARNING: ugv_edf_3 data missing — skipping CL_J")
        return

    bin2 = np.arange(0, MAX_T_S + 120, 120)  # 2-min bins
    t_2min = (bin2[:-1] + bin2[1:]) / 2 / 60

    net_v = net[net["window_pdr"] >= 0].sort_values("t_rel_s")
    pdr_b = _bin_mean(net_v["t_rel_s"].values, net_v["window_pdr"].values, bin2)

    rd_b = np.full(len(bin2) - 1, np.nan)
    for i, (b0, b1) in enumerate(zip(bin2[:-1], bin2[1:])):
        seg = ce[(ce["t_rel_s"] >= b0) & (ce["t_rel_s"] < b1)]
        if len(seg) > 0:
            rd_b[i] = float((seg["outcome"] == "ROUTING_DROP").mean())

    death_t_min = np.sort(de["t_rel_s"].values) / 60
    cum_t = np.concatenate([[0], death_t_min, [180]])
    cum_v = np.concatenate([[0], np.arange(1, len(death_t_min) + 1), [len(death_t_min)]])

    fig, ax1 = plt.subplots(figsize=(14, 5))
    ax2 = ax1.twinx()
    ax3 = ax1.twinx()
    ax3.spines["right"].set_position(("axes", 1.10))

    l1, = ax1.plot(t_2min, pdr_b, "b-", linewidth=2, label="PDR (2-min bins)")
    l2, = ax1.plot(t_2min, rd_b, "r--", linewidth=1.5, alpha=0.85,
                   label="Charge-req. ROUTING_DROP rate")
    l3, = ax3.step(cum_t, cum_v, "k-", linewidth=1.5, alpha=0.7,
                   where="post", label="Cumulative depletions")

    # Annotate cascade trigger (first two CH deaths)
    trigger_t = float(de.sort_values("t_rel_s")["t_rel_s"].iloc[0]) / 60
    ax1.axvline(trigger_t, color="darkred", linewidth=1.5, linestyle=":", alpha=0.9)
    ax1.annotate(
        "2× CH depletions\n(cascade trigger)\nt ≈ 6.6 min",
        xy=(trigger_t, 0.65), xytext=(18, 0.77),
        fontsize=8, color="darkred",
        arrowprops=dict(arrowstyle="->", color="darkred", lw=1.3),
    )

    # Shade region where alive_count = 0
    zero_alive = de[de["alive_count"] == 0]
    if not zero_alive.empty:
        t_zero = float(zero_alive["t_rel_s"].min()) / 60
        ax1.axvspan(t_zero, 180, alpha=0.06, color="red",
                    label=f"Fleet alive = 0  (> {t_zero:.0f} min)")

    ax1.set_xlabel("Time (minutes)", fontsize=10)
    ax1.set_ylabel("PDR / ROUTING_DROP rate", fontsize=10, color="navy")
    ax3.set_ylabel("Cumulative depletion events", fontsize=10, color="gray")
    ax1.tick_params(axis="y", colors="navy")
    ax3.tick_params(axis="y", colors="gray")
    ax2.set_yticks([])       # hide middle axis ticks (merged with ax1)
    ax1.set_ylim(0, 1)
    ax1.grid(alpha=0.3)

    lines = [l1, l2, l3]
    ax1.legend(lines, [l.get_label() for l in lines], loc="upper right", fontsize=9)

    ax1.set_title(
        "§4.4 / §5 Anomaly A — ugv_edf run 3: Cascade Failure Timeseries\n"
        "PDR collapses (blue) → ROUTING_DROP surges (red dashed) "
        "→ depletion spiral accelerates (black steps)",
        fontsize=11, fontweight="bold",
    )
    plt.tight_layout()
    _save(fig, out_dir / "CL_J_ugv_edf3_cascade.png")


# ---------------------------------------------------------------------------
# CL_K — Conditioning-aware delay robustness panel
# ---------------------------------------------------------------------------

def plot_cl_k_delay_robustness_panel(data_root: Path, out_dir: Path) -> None:
    """Conditioning-aware delay robustness analysis (CL_K).

    Two panels:
      Left  — Scatter: window_delivered (sample size) vs window_delay_mean_ms,
              coloured by window_pdr.  Exposes the conditioning bias: windows
              with few delivered packets (low-PDR regimes) appear to have lower
              delay because only short-path survivors are measured.
      Right — DPR-weighted delay ranking: horizontal bar chart comparing the
              naïve mean delay to the PDR-weighted mean delay per protocol,
              sorted by weighted delay.  Weighted mean = Σ(delay·PDR)/Σ(PDR)
              so that high-PDR (healthy, full-path) windows dominate the average.

    Data: network_timeseries.csv (window_pdr, window_delay_mean_ms, window_delivered).
    Output: CL_K_delay_robustness_panel.png
    """
    records = []
    for proto in PROTOCOLS:
        for r in REPLICATES:
            net = _load_csv(_run_dir(data_root, proto, r), "network_timeseries.csv")
            if net.empty:
                continue
            needed = {"window_delay_mean_ms", "window_pdr", "window_delivered"}
            if not needed.issubset(net.columns):
                continue
            valid = net[
                (net["window_delay_mean_ms"] >= 0) &
                (net["window_pdr"] >= 0) &
                (net["window_delivered"] > 0)
            ].copy()
            if valid.empty:
                continue
            valid = valid.assign(protocol=proto)
            records.append(valid[["protocol", "window_pdr", "window_delay_mean_ms", "window_delivered"]])

    if not records:
        print("  WARNING: no valid window data for CL_K — skipping")
        return

    all_wins = pd.concat(records, ignore_index=True)

    # Per-protocol weighted and unweighted delay
    stats_rows = []
    for proto in PROTOCOLS:
        sub = all_wins[all_wins["protocol"] == proto]
        if sub.empty:
            continue
        unweighted = float(sub["window_delay_mean_ms"].mean())
        w = sub["window_pdr"].values
        d = sub["window_delay_mean_ms"].values
        weighted = float(np.dot(w, d) / w.sum()) if w.sum() > 0 else np.nan
        stats_rows.append({
            "protocol": proto,
            "label": PROTO_LABELS[proto],
            "color": PROTO_COLORS[proto],
            "unweighted": unweighted,
            "weighted": weighted,
        })
    stats_df = pd.DataFrame(stats_rows).sort_values("weighted").reset_index(drop=True)

    fig, (ax_scatter, ax_bar) = plt.subplots(1, 2, figsize=(14, 5))

    # --- Panel 1: scatter window_delivered vs delay, coloured by PDR ---
    sc = ax_scatter.scatter(
        all_wins["window_delivered"],
        all_wins["window_delay_mean_ms"],
        c=all_wins["window_pdr"],
        cmap="RdYlGn", vmin=0, vmax=1,
        s=6, alpha=0.35, rasterized=True,
    )
    plt.colorbar(sc, ax=ax_scatter, label="window PDR")

    sub_reg = all_wins[["window_delivered", "window_delay_mean_ms"]].dropna()
    if len(sub_reg) >= 10:
        m, b, r_val, p_val, _ = stats.linregress(
            sub_reg["window_delivered"], sub_reg["window_delay_mean_ms"]
        )
        xl = np.linspace(sub_reg["window_delivered"].min(), sub_reg["window_delivered"].max(), 200)
        ax_scatter.plot(
            xl, m * xl + b, "k--", linewidth=1.5, alpha=0.85,
            label=f"OLS  r = {r_val:.2f},  p = {p_val:.3f}",
        )
        ax_scatter.legend(fontsize=8)

    ax_scatter.set_xlabel("Delivered packets per window (sample size)", fontsize=10)
    ax_scatter.set_ylabel("Window mean E2E delay (ms)", fontsize=10)
    ax_scatter.set_title(
        "Delay vs Delivered Sample Size\n"
        "(colour = PDR;  low count → low delay conditioning bias)",
        fontsize=10, fontweight="bold",
    )
    ax_scatter.grid(alpha=0.3)
    ax_scatter.text(
        0.02, 0.97,
        "Survivorship bias: only packets that\n"
        "complete transit are timed → short\n"
        "paths dominate in low-PDR windows.",
        transform=ax_scatter.transAxes, fontsize=7.5, va="top",
        bbox=dict(boxstyle="round", facecolor="lightyellow", alpha=0.85),
    )

    # --- Panel 2: DPR-weighted vs unweighted horizontal bar chart ---
    y = np.arange(len(stats_df))
    h = 0.32
    ax_bar.barh(
        y + h / 2, stats_df["weighted"], h,
        color=stats_df["color"].values, alpha=0.88,
        label="PDR-weighted mean delay",
    )
    ax_bar.barh(
        y - h / 2, stats_df["unweighted"], h,
        color=stats_df["color"].values, alpha=0.38, hatch="//",
        label="Unweighted mean delay",
    )
    ax_bar.set_yticks(y)
    ax_bar.set_yticklabels(stats_df["label"].values, fontsize=9)
    ax_bar.set_xlabel("Mean E2E Delay (ms)", fontsize=10)
    ax_bar.set_title(
        "DPR-Weighted vs Unweighted Delay Ranking\n"
        "(sorted by weighted delay; weighted = Σ(delay·PDR) / Σ(PDR))",
        fontsize=10, fontweight="bold",
    )
    ax_bar.legend(fontsize=8, loc="lower right")
    ax_bar.grid(axis="x", alpha=0.3)
    ax_bar.text(
        0.98, 0.98,
        "Weighted delay upweights healthy\n"
        "(high-PDR) windows, correcting\n"
        "for survivorship bias.",
        transform=ax_bar.transAxes, fontsize=7.5, va="top", ha="right",
        bbox=dict(boxstyle="round", facecolor="lightyellow", alpha=0.85),
    )

    fig.suptitle(
        "CL_K — Conditioning-Aware Delay Robustness Panel\n"
        "(addresses survivorship bias in E2E delay measurement)",
        fontsize=12, fontweight="bold",
    )
    plt.tight_layout()
    _save(fig, out_dir / "CL_K_delay_robustness_panel.png")


# ---------------------------------------------------------------------------
# CL_L — Event-aligned composite (per protocol + merged)
# ---------------------------------------------------------------------------

def plot_cl_l_event_aligned_composite(
    data_root: Path, out_dir: Path, bin_sec: int = 600
) -> None:
    """Event-aligned 4-pane composite timeseries per protocol + merged (CL_L).

    Layout: 4 rows × 8 columns  (7 protocol columns + 1 merged column).
      Row 0: PDR            — window_pdr from network_timeseries.csv
      Row 1: Timeout count  — TIMEOUT events binned from charge_events.csv
      Row 2: Deaths / bin   — depletion events binned from death_events.csv
      Row 3: Dock util.     — ugv_dock_utilization from charge_queue_timeseries.csv

    All panes share the same x-axis (time in minutes).  Each row shares its
    y-axis across columns so magnitudes are directly comparable.  The rightmost
    column shows the cross-protocol mean ± 1σ (all 7 protocols × 3 replicates).

    Data: network_timeseries.csv, charge_events.csv, death_events.csv,
          charge_queue_timeseries.csv.
    Output: CL_L_event_aligned_composite.png
    """
    bins = np.arange(0, MAX_T_S + bin_sec, bin_sec)
    t_min = (bins[:-1] + bins[1:]) / 2 / 60
    n_bins = len(bins) - 1
    n_proto = len(PROTOCOLS)
    n_cols = n_proto + 1  # last column = merged

    ROW_LABELS = ["PDR", "Timeouts / bin", "Deaths / bin", "Dock utilisation"]
    ROW_COLORS = ["steelblue", "darkorange", "firebrick", "mediumseagreen"]
    N_ROWS = 4

    fig, axes = plt.subplots(
        N_ROWS, n_cols,
        figsize=(3.0 * n_cols, 2.3 * N_ROWS),
        sharex=True, sharey="row",
        gridspec_kw={"hspace": 0.10, "wspace": 0.05},
    )

    # store[row_idx][col_idx] = list of (n_bins,) arrays, one per replicate
    store = {row: [[] for _ in range(n_cols)] for row in range(N_ROWS)}

    for col_idx, proto in enumerate(PROTOCOLS):
        for r_rep in REPLICATES:
            folder = data_root / f"{proto}_{r_rep}"

            # Row 0: PDR
            net = _load_csv(folder, "network_timeseries.csv")
            if not net.empty and "window_pdr" in net.columns:
                valid = net[net["window_pdr"] >= 0]
                store[0][col_idx].append(
                    _bin_mean(valid["t_rel_s"].values, valid["window_pdr"].values, bins)
                )

            # Row 1: timeout count per bin
            ce = _load_csv(folder, "charge_events.csv")
            if not ce.empty and "outcome" in ce.columns:
                tmout = ce[ce["outcome"] == "TIMEOUT"]
                hist = (
                    np.histogram(tmout["t_rel_s"].values, bins=bins)[0].astype(float)
                    if len(tmout) > 0
                    else np.zeros(n_bins)
                )
                store[1][col_idx].append(hist)

            # Row 2: death count per bin
            de = _load_csv(folder, "death_events.csv")
            if not de.empty and "t_rel_s" in de.columns:
                store[2][col_idx].append(
                    np.histogram(de["t_rel_s"].values, bins=bins)[0].astype(float)
                )

            # Row 3: dock utilisation
            cq = _load_csv(folder, "charge_queue_timeseries.csv")
            if not cq.empty and "ugv_dock_utilization" in cq.columns:
                valid_cq = cq[cq["ugv_dock_utilization"] >= 0]
                store[3][col_idx].append(
                    _bin_mean(
                        valid_cq["t_rel_s"].values,
                        valid_cq["ugv_dock_utilization"].values,
                        bins,
                    )
                )

    # Build merged column from all protocol replicates
    for row_idx in range(N_ROWS):
        merged = []
        for col_idx in range(n_proto):
            merged.extend(store[row_idx][col_idx])
        store[row_idx][n_proto] = merged

    # --- Render ---
    for col_idx in range(n_cols):
        is_merged = col_idx == n_proto
        proto = None if is_merged else PROTOCOLS[col_idx]
        col_color = "black" if is_merged else PROTO_COLORS[proto]
        col_label = "All (merged)" if is_merged else PROTO_LABELS[proto]

        for row_idx in range(N_ROWS):
            ax = axes[row_idx, col_idx]
            reps = store[row_idx][col_idx]

            if reps:
                arr = np.array(reps)
                mean = np.nanmean(arr, axis=0)
                std = np.nanstd(arr, axis=0)
                ax.plot(t_min, mean, color=col_color, linewidth=1.5)
                ax.fill_between(
                    t_min, mean - std, mean + std,
                    alpha=0.20, color=col_color,
                )

            ax.grid(alpha=0.22, linewidth=0.5)
            ax.tick_params(labelsize=6)

            # Column header on top row
            if row_idx == 0:
                title_kw = dict(fontsize=8, fontweight="bold", pad=3)
                if is_merged:
                    title_kw["color"] = "black"
                    ax.set_title(col_label, **title_kw)
                    for spine in ax.spines.values():
                        spine.set_linewidth(1.5)
                        spine.set_edgecolor("dimgray")
                else:
                    ax.set_title(col_label, color=col_color, **title_kw)

            # Row label on leftmost column
            if col_idx == 0:
                ax.set_ylabel(ROW_LABELS[row_idx], fontsize=8,
                              color=ROW_COLORS[row_idx])

            # X-label on bottom row
            if row_idx == N_ROWS - 1:
                ax.set_xlabel("Time (min)", fontsize=7)

    fig.suptitle(
        "CL_L — Event-Aligned Composite (per Protocol + Merged)\n"
        "Row 0: PDR  ·  Row 1: Charge timeouts  ·  "
        "Row 2: Battery deaths  ·  Row 3: Dock utilisation\n"
        "(mean ± 1σ across 3 replicates; rightmost column = all-protocol mean)",
        fontsize=11, fontweight="bold", y=1.02,
    )
    plt.tight_layout()
    _save(fig, out_dir / "CL_L_event_aligned_composite.png")


# ---------------------------------------------------------------------------
# CL_M — Role-stratified scheduling audit (the CH priority paradox)
# ---------------------------------------------------------------------------

def _collect_role_kpis(data_root: Path) -> pd.DataFrame:
    """Per-run, per-role charge outcome and latency KPIs from charge_events.csv.

    Returns a DataFrame with columns:
        protocol, run, role ("CH" | "member"), n_requests,
        success_rate, timeout_rate, routing_drop_rate,
        decision_latency_mean, effective_wait_mean
    """
    rows = []
    for proto in PROTOCOLS:
        for r in REPLICATES:
            ce = _load_csv(_run_dir(data_root, proto, r), "charge_events.csv")
            if ce.empty or "outcome" not in ce.columns or "role" not in ce.columns:
                continue
            for role_val, role_label in [(1, "CH"), (0, "member")]:
                sub = ce[ce["role"] == role_val]
                if sub.empty:
                    continue
                total = len(sub)
                dl = sub[sub["decision_latency_ms"] > 0]["decision_latency_ms"]
                ew = sub[sub["effective_wait_ms"] > 0]["effective_wait_ms"]
                rows.append({
                    "protocol": proto, "run": r, "role": role_label,
                    "n_requests": total,
                    "success_rate": (sub["outcome"] == "STARTED").sum() / total,
                    "timeout_rate": (sub["outcome"] == "TIMEOUT").sum() / total,
                    "routing_drop_rate": (sub["outcome"] == "ROUTING_DROP").sum() / total,
                    "decision_latency_mean": dl.mean() if len(dl) > 0 else np.nan,
                    "effective_wait_mean": ew.mean() if len(ew) > 0 else np.nan,
                })
    return pd.DataFrame(rows)


def plot_cl_m_role_scheduling_audit(data_root: Path, out_dir: Path) -> None:
    """Role-stratified scheduling audit: the CH priority paradox (CL_M).

    Four grouped-bar panels (CH vs member per protocol):
      A — Mean decision latency (ms): shows CH decision latency is HIGHER
          in role-based protocols than in urgency-based ones, despite CH
          priority — the "priority paradox" caused by synchronized CH
          requests queuing behind each other.
      B — Mean effective wait at dock (ms).
      C — Charge success rate: shows member starvation in role-based
          protocols (members rarely reach the dock).
      D — Charge routing-drop rate: shows members' requests are more
          often lost in role-based runs because the network is degraded
          while CHs dominate the dock.

    Data: charge_events.csv — role, outcome, decision_latency_ms,
          effective_wait_ms.
    Output: CL_M_role_scheduling_audit.png
    """
    rdf = _collect_role_kpis(data_root)
    if rdf.empty:
        print("  WARNING: no role KPI data for CL_M — skipping")
        return

    short = [PROTO_LABELS[p] for p in PROTOCOLS]
    x = np.arange(len(PROTOCOLS))
    w = 0.35
    ROLE_COLORS = {"CH": "#2166ac", "member": "#d6604d"}

    panels = [
        ("decision_latency_mean", "Decision latency (ms)",
         "A — Decision Latency\n(CH blocks CH → paradox)"),
        ("effective_wait_mean", "Effective wait at dock (ms)",
         "B — Effective Wait at Dock"),
        ("success_rate", "Charge success rate",
         "C — Charge Success Rate\n(member starvation in role-based)"),
        ("routing_drop_rate", "Routing-drop rate",
         "D — Charge Request Routing-Drop Rate"),
    ]

    fig, axes = plt.subplots(2, 2, figsize=(15, 9))
    axes = axes.flatten()

    for ax, (metric, ylabel, title) in zip(axes, panels):
        for ri, (role_label, sign) in enumerate([("CH", -1), ("member", +1)]):
            sub = rdf[rdf["role"] == role_label]
            if sub.empty:
                continue
            agg = sub.groupby("protocol")[metric]
            means = [agg.mean()[p] if p in agg.mean() else np.nan for p in PROTOCOLS]
            stds = [agg.std()[p] if p in agg.std() else 0.0 for p in PROTOCOLS]
            ax.bar(
                x + sign * w / 2, means, w,
                yerr=stds, capsize=3,
                color=ROLE_COLORS[role_label], alpha=0.85,
                label=role_label,
            )
        ax.set_xticks(x)
        ax.set_xticklabels(short, rotation=40, ha="right", fontsize=8)
        ax.set_ylabel(ylabel, fontsize=9)
        ax.set_title(title, fontsize=9, fontweight="bold")
        ax.legend(fontsize=8)
        ax.grid(axis="y", alpha=0.3)

        # Highlight role-based protocols
        for xi, proto in enumerate(PROTOCOLS):
            if "role_priority" in proto:
                ax.axvspan(xi - 0.5, xi + 0.5, alpha=0.07, color="red", zorder=0)

    # Add shared annotation
    fig.text(
        0.5, 0.01,
        "Red shading = role-based protocols (ugv_role_priority, ugv_p_role_priority). "
        "Blue bar = CH (role=1); red bar = member (role=0).\n"
        "Error bars = 1 SD across 3 replicates.",
        ha="center", fontsize=8, style="italic",
    )
    fig.suptitle(
        "CL_M — Role-Stratified Scheduling Audit: The CH Priority Paradox\n"
        "Despite receiving priority, CHs in role-based protocols wait longer "
        "and members are starved of dock access",
        fontsize=12, fontweight="bold",
    )
    plt.tight_layout(rect=[0, 0.04, 1, 1])
    _save(fig, out_dir / "CL_M_role_scheduling_audit.png")


# ---------------------------------------------------------------------------
# CL_N — CH depletion synchronization → cascade analysis
# ---------------------------------------------------------------------------

def _compute_ch_request_gaps(data_root: Path) -> pd.DataFrame:
    """For each run, compute nearest cross-CH request time gaps.

    For each pair of distinct CH UAVs within the same run, and for each
    request submitted by one CH, find the minimum absolute time distance
    to any request from the other CH.  Returns a DataFrame with columns:
        protocol, run, gap_s
    """
    rows = []
    for proto in PROTOCOLS:
        for r in REPLICATES:
            ce = _load_csv(_run_dir(data_root, proto, r), "charge_events.csv")
            if ce.empty or "role" not in ce.columns or "uav_id" not in ce.columns:
                continue
            ch = ce[ce["role"] == 1][["uav_id", "t_rel_s"]].dropna()
            uavs = ch["uav_id"].unique()
            if len(uavs) < 2:
                continue
            times = {u: ch[ch["uav_id"] == u]["t_rel_s"].values for u in uavs}
            uav_list = list(uavs)
            for i in range(len(uav_list)):
                for j in range(i + 1, len(uav_list)):
                    t1 = np.array(sorted(times[uav_list[i]]))
                    t2 = np.array(sorted(times[uav_list[j]]))
                    for req in t1:
                        if len(t2) > 0:
                            gap = float(np.min(np.abs(t2 - req)))
                            rows.append({"protocol": proto, "run": r, "gap_s": gap})
    return pd.DataFrame(rows)


def plot_cl_n_ch_sync_cascade(data_root: Path, out_dir: Path) -> None:
    """CH depletion synchronization → cascade analysis (CL_N).

    Two panels:
      Left — KDE of the nearest cross-CH request time gap (seconds) per
             protocol.  All protocols show a peak near 0–60 s, confirming
             that CHs deplete in sync regardless of scheduling policy
             (driven by identical battery capacity and mission duty cycle).
             The area under the curve for gaps < 120 s is annotated per
             protocol.
      Right — Scatter: per-run mean CH decision latency vs member charge
              success rate, coloured by protocol.  Expected pattern:
              role-based protocols appear in the upper-left (high CH wait,
              low member success) while urgency-based protocols cluster in
              the lower-right (fast CH decisions, healthy member service).
              A regression line highlights the negative relationship.

    Data: charge_events.csv — role, uav_id, t_rel_s, decision_latency_ms,
          outcome.
    Output: CL_N_ch_sync_cascade.png
    """
    gaps_df = _compute_ch_request_gaps(data_root)
    role_df = _collect_role_kpis(data_root)

    fig, (ax_kde, ax_scatter) = plt.subplots(1, 2, figsize=(14, 5))

    # --- Left: KDE of cross-CH gap per protocol ---
    CLIP = 600  # show 0–600 s
    synced_fracs = {}
    for proto in PROTOCOLS:
        sub = gaps_df[gaps_df["protocol"] == proto]["gap_s"].dropna()
        if sub.empty:
            continue
        sub_clip = sub[sub <= CLIP]
        # Simple KDE via np.histogram for robustness
        hist, edges = np.histogram(sub_clip, bins=60, range=(0, CLIP), density=True)
        centres = (edges[:-1] + edges[1:]) / 2
        ax_kde.plot(centres, hist, color=PROTO_COLORS[proto],
                    label=PROTO_LABELS[proto], linewidth=1.8, alpha=0.85)
        # Fraction of gaps < 120 s
        synced_fracs[proto] = float((sub <= 120).mean())

    ax_kde.set_xlabel("Nearest cross-CH request gap (s)", fontsize=10)
    ax_kde.set_ylabel("Density", fontsize=10)
    ax_kde.set_title(
        "CH Request Temporal Proximity\n"
        "(all protocols: CHs deplete in sync due to identical battery capacity)",
        fontsize=10, fontweight="bold",
    )
    ax_kde.axvline(120, color="gray", linestyle="--", linewidth=1, alpha=0.7,
                   label="120 s threshold")
    ax_kde.legend(fontsize=7.5, title="Protocol", loc="upper right")
    ax_kde.grid(alpha=0.3)

    # Annotate fraction < 120 s per protocol on right margin
    for proto in PROTOCOLS:
        if proto in synced_fracs:
            sub_p = gaps_df[gaps_df["protocol"] == proto]["gap_s"]
            if sub_p.empty:
                continue
            # Find density at x=30 to position label
            ax_kde.text(
                125, 0,
                "",  # placeholder — use summary bar instead
                fontsize=6, color=PROTO_COLORS[proto],
            )

    # --- Right: scatter CH decision latency vs member success rate per run ---
    ch_lat = (
        role_df[role_df["role"] == "CH"]
        .set_index(["protocol", "run"])["decision_latency_mean"]
    )
    mem_suc = (
        role_df[role_df["role"] == "member"]
        .set_index(["protocol", "run"])["success_rate"]
    )
    scatter_rows = []
    for proto in PROTOCOLS:
        for r in REPLICATES:
            key = (proto, r)
            if key in ch_lat.index and key in mem_suc.index:
                scatter_rows.append({
                    "protocol": proto,
                    "ch_latency": float(ch_lat[key]),
                    "member_success": float(mem_suc[key]),
                })
    sdf = pd.DataFrame(scatter_rows).dropna()

    for proto in PROTOCOLS:
        pts = sdf[sdf["protocol"] == proto]
        ax_scatter.scatter(
            pts["ch_latency"], pts["member_success"],
            color=PROTO_COLORS[proto], label=PROTO_LABELS[proto],
            s=70, alpha=0.9, zorder=3,
        )
        # Label role-based protocols
        if "role_priority" in proto:
            for _, row in pts.iterrows():
                ax_scatter.annotate(
                    PROTO_LABELS[proto],
                    (row["ch_latency"], row["member_success"]),
                    fontsize=7, color=PROTO_COLORS[proto],
                    xytext=(4, 4), textcoords="offset points",
                )

    if len(sdf) >= 4:
        r_val, p_val = stats.pearsonr(sdf["ch_latency"], sdf["member_success"])
        m, b, *_ = stats.linregress(sdf["ch_latency"], sdf["member_success"])
        xl = np.linspace(sdf["ch_latency"].min(), sdf["ch_latency"].max(), 100)
        ax_scatter.plot(xl, m * xl + b, "k--", linewidth=1.3, alpha=0.7,
                        label=f"OLS  r = {r_val:.2f}, p = {p_val:.3f}")

    ax_scatter.set_xlabel("Mean CH decision latency (ms)", fontsize=10)
    ax_scatter.set_ylabel("Member charge success rate", fontsize=10)
    ax_scatter.set_title(
        "CH Wait Time vs Member Success Rate\n"
        "(role-based: long CH wait → member starvation)",
        fontsize=10, fontweight="bold",
    )
    ax_scatter.legend(fontsize=7.5, title="Protocol", loc="upper right")
    ax_scatter.grid(alpha=0.3)
    ax_scatter.text(
        0.02, 0.05,
        "Upper-left = role-based protocols\n"
        "(long CH wait, starved members)\n"
        "Lower-right = urgency-based\n"
        "(fast CH decisions, healthy members)",
        transform=ax_scatter.transAxes, fontsize=7.5, va="bottom",
        bbox=dict(boxstyle="round", facecolor="lightyellow", alpha=0.85),
    )

    fig.suptitle(
        "CL_N — CH Depletion Synchronization → Priority Congestion → Member Cascade\n"
        "Left: CHs deplete in sync across all protocols (battery physics).\n"
        "Right: Role-based protocols mishandle synchronized pairs → CH queues block → members starved.",
        fontsize=11, fontweight="bold",
    )
    plt.tight_layout()
    _save(fig, out_dir / "CL_N_ch_sync_cascade.png")


# ---------------------------------------------------------------------------
# CL_O — §4.2 Mechanism narrative figure
# ---------------------------------------------------------------------------

def plot_cl_o_mechanism_narrative(df: pd.DataFrame, data_root: Path, out_dir: Path,
                                   bin_sec: int = 600) -> None:
    """§4.2 Cross-layer causal chain: narrative mechanism figure (CL_O).

    Layout  (2 rows):
    ──────────────────────────────────────────────────────────────────────────
    Row 0  [full width]:  Flow-diagram rendered with matplotlib patches.
           Four rounded boxes connected by annotated arrows:
               [Scheduling Policy]  →  [Charge Outcomes]  →  [Fleet Survival]
                                              ↑                      │
                                      (ROUTING_DROP             deaths ↓)
                                       feedback ↗)              [PDR / QoS ↓]
           Arrows are annotated with Pearson r computed from the 21-run KPI
           DataFrame.  A dashed feedback arrow from PDR → Charge Outcomes
           closes the loop.

    Row 1  [4 sub-panels, shared x-axis]:  Temporal evidence — per-protocol
           mean timeseries (bin_sec bins).  Two protocol groups highlighted:
               • Best performers  (ugv_p_edf, ugv_edf) in blue shades
               • Role-based group (ugv_p_role_priority, ugv_role_priority) in red shades
           Panels:
               1 — Dock utilisation (scheduling load proxy)
               2 — Charge timeout rate per bin   (charge outcomes)
               3 — Battery depletions per bin    (fleet survivability)
               4 — Window PDR                   (network quality)

    Data:
        df                    — per-run scalar KPIs (from collect_per_run_kpis)
        charge_queue_timeseries.csv  → ugv_dock_utilization
        charge_events.csv            → outcome (TIMEOUT per bin)
        death_events.csv             → t_rel_s (deaths per bin)
        network_timeseries.csv       → window_pdr, t_rel_s

    Output: CL_O_mechanism_narrative.png + .pdf
    """
    import matplotlib.patches as mpatches
    from matplotlib.patches import FancyArrowPatch, FancyBboxPatch
    from matplotlib.lines import Line2D

    # ── Compute correlations for arrow annotations ──────────────────────────
    def _r(a: str, b: str) -> str:
        sub = df[[a, b]].dropna()
        if len(sub) < 4:
            return "n/a"
        r, _ = stats.pearsonr(sub[a], sub[b])
        return f"r = {r:+.2f}"

    r_sched_charge  = _r("charge_success_rate", "total_deaths")   # success ↑ → deaths ↓
    r_charge_surv   = _r("total_deaths", "pdr")                    # deaths ↑ → PDR ↓
    r_surv_net      = _r("charge_routing_drop_rate", "pdr")        # routing_drop ↑ → PDR ↓

    # ── Load binned timeseries for bottom row ────────────────────────────────
    bins = np.arange(0, MAX_T_S + bin_sec, bin_sec)
    bin_mids = (bins[:-1] + bins[1:]) / 2 / 60  # minutes
    n_bins = len(bin_mids)

    HIGHLIGHT = {
        "best":      ["ugv_p_edf", "ugv_edf"],
        "role_based": ["ugv_p_role_priority", "ugv_role_priority"],
    }
    GROUP_COLORS = {
        "ugv_p_edf":           "#1f77b4",
        "ugv_edf":             "#aec7e8",
        "ugv_p_role_priority": "#d62728",
        "ugv_role_priority":   "#f5a5a5",
    }
    HIGHLIGHT_PROTOS = HIGHLIGHT["best"] + HIGHLIGHT["role_based"]

    # Containers: proto → (n_bins,) mean across replicates
    dock_ts:    dict = {}
    timeout_ts: dict = {}
    death_ts:   dict = {}
    pdr_ts:     dict = {}

    for proto in PROTOCOLS:
        d_rep, t_rep, de_rep, p_rep = [], [], [], []
        for r in REPLICATES:
            folder = _run_dir(data_root, proto, r)

            # Dock utilisation (mean per bin from timeseries)
            cq = _load_csv(folder, "charge_queue_timeseries.csv")
            if not cq.empty and "ugv_dock_utilization" in cq.columns and "t_rel_s" in cq.columns:
                t = cq["t_rel_s"].values
                v = cq["ugv_dock_utilization"].values
                idx = np.searchsorted(bins[1:], t)
                idx = np.clip(idx, 0, n_bins - 1)
                sums = np.bincount(idx, weights=v, minlength=n_bins).astype(float)
                cnts = np.bincount(idx, minlength=n_bins).astype(float)
                cnts[cnts == 0] = np.nan
                d_rep.append(sums / cnts)

            # Charge timeout rate per bin (from charge_events)
            ce = _load_csv(folder, "charge_events.csv")
            if not ce.empty and "outcome" in ce.columns and "t_rel_s" in ce.columns:
                t = ce["t_rel_s"].values
                is_timeout = (ce["outcome"] == "TIMEOUT").values.astype(float)
                idx = np.searchsorted(bins[1:], t)
                idx = np.clip(idx, 0, n_bins - 1)
                to_sum = np.bincount(idx, weights=is_timeout, minlength=n_bins).astype(float)
                cnt = np.bincount(idx, minlength=n_bins).astype(float)
                cnt[cnt == 0] = np.nan
                t_rep.append(to_sum / cnt)

            # Deaths per bin
            de = _load_csv(folder, "death_events.csv")
            if not de.empty and "t_rel_s" in de.columns:
                t = de["t_rel_s"].values
                idx = np.searchsorted(bins[1:], t)
                idx = np.clip(idx, 0, n_bins - 1)
                de_rep.append(np.bincount(idx, minlength=n_bins).astype(float))

            # PDR per bin
            net = _load_csv(folder, "network_timeseries.csv")
            if not net.empty and "window_pdr" in net.columns and "t_rel_s" in net.columns:
                t = net["t_rel_s"].values
                v = net["window_pdr"].values
                idx = np.searchsorted(bins[1:], t)
                idx = np.clip(idx, 0, n_bins - 1)
                sums = np.bincount(idx, weights=v, minlength=n_bins).astype(float)
                cnts = np.bincount(idx, minlength=n_bins).astype(float)
                cnts[cnts == 0] = np.nan
                p_rep.append(sums / cnts)

        dock_ts[proto]    = np.nanmean(d_rep,  axis=0) if d_rep  else np.full(n_bins, np.nan)
        timeout_ts[proto] = np.nanmean(t_rep,  axis=0) if t_rep  else np.full(n_bins, np.nan)
        death_ts[proto]   = np.nanmean(de_rep, axis=0) if de_rep else np.full(n_bins, np.nan)
        pdr_ts[proto]     = np.nanmean(p_rep,  axis=0) if p_rep  else np.full(n_bins, np.nan)

    # ── Figure layout ────────────────────────────────────────────────────────
    fig = plt.figure(figsize=(16, 12))
    gs = fig.add_gridspec(
        2, 4,
        height_ratios=[1.2, 1.8],
        hspace=0.45, wspace=0.35,
        top=0.93, bottom=0.07, left=0.07, right=0.97,
    )

    # ── Row 0: flow diagram spanning all 4 columns ───────────────────────────
    ax_flow = fig.add_subplot(gs[0, :])
    ax_flow.set_xlim(0, 10)
    ax_flow.set_ylim(0, 4)
    ax_flow.axis("off")

    BOX_W, BOX_H = 1.7, 0.85
    BOXES = [
        (0.5,  1.575, "Scheduling\nPolicy",   "#4393c3"),
        (2.85, 1.575, "Charge\nOutcomes",     "#74c476"),
        (5.4,  1.575, "Fleet\nSurvivability", "#fd8d3c"),
        (7.95, 1.575, "Network\nQuality",     "#9e9ac8"),
    ]
    box_centers = []
    for (bx, by, label, facecolor) in BOXES:
        patch = FancyBboxPatch(
            (bx, by), BOX_W, BOX_H,
            boxstyle="round,pad=0.08",
            linewidth=1.8, edgecolor="white",
            facecolor=facecolor, alpha=0.88,
            transform=ax_flow.transData, zorder=3,
        )
        ax_flow.add_patch(patch)
        ax_flow.text(
            bx + BOX_W / 2, by + BOX_H / 2, label,
            ha="center", va="center", fontsize=10, fontweight="bold",
            color="white", zorder=4,
        )
        box_centers.append((bx + BOX_W / 2, by + BOX_H / 2))

    # Forward arrows with correlation annotations
    arrow_specs = [
        (0, 1, f"Charge success ↑\n→ fewer deaths\n({r_sched_charge})", 2.8),
        (1, 2, f"Depletions ↑\n→ PDR ↓\n({r_charge_surv})",            5.35),
        (2, 3, f"Routing drops ↑\n→ PDR ↓\n({r_surv_net})",            7.9),
    ]
    for (src, dst, label, lx) in arrow_specs:
        x0 = box_centers[src][0] + BOX_W / 2 - 0.05
        x1 = box_centers[dst][0] - BOX_W / 2 + 0.05
        y  = box_centers[src][1]
        ax_flow.annotate(
            "", xy=(x1, y), xytext=(x0, y),
            arrowprops=dict(arrowstyle="-|>", lw=1.8, color="#333333"),
            zorder=5,
        )
        ax_flow.text(lx, y + 0.62, label, ha="center", va="bottom",
                     fontsize=7.5, color="#333333", style="italic")

    # Feedback arrow: PDR ↓ → Charge Outcomes (ROUTING_DROP loop)
    fb_x0 = box_centers[3][0]        # Network Quality centre-x
    fb_x1 = box_centers[1][0]        # Charge Outcomes centre-x
    fb_y  = box_centers[0][1] - 1.05
    ax_flow.annotate(
        "", xy=(fb_x1, box_centers[1][1] - BOX_H / 2),
        xytext=(fb_x0, box_centers[3][1] - BOX_H / 2),
        arrowprops=dict(
            arrowstyle="-|>", lw=1.5, color="#b2182b",
            connectionstyle=f"arc3,rad=-0.35",
        ),
        zorder=5,
    )
    ax_flow.text(
        (fb_x0 + fb_x1) / 2, box_centers[0][1] - 1.4,
        "Feedback: PDR ↓ → more ROUTING_DROPs → more deaths\n(reinforcing loop)",
        ha="center", va="top", fontsize=8, color="#b2182b", style="italic",
    )

    ax_flow.set_title(
        "§4.2 Cross-Layer Causal Chain  —  Scheduling Policy → Network Quality",
        fontsize=12, fontweight="bold", pad=8,
    )

    # ── Row 1: 4 timeseries panels ───────────────────────────────────────────
    panel_data = [
        (dock_ts,    "Dock utilisation",         "A  Scheduling load proxy"),
        (timeout_ts, "Charge timeout rate",       "B  Charge outcomes"),
        (death_ts,   "Battery depletions / bin",  "C  Fleet survivability"),
        (pdr_ts,     "Window PDR",                "D  Network quality"),
    ]

    axes_ts = [fig.add_subplot(gs[1, c]) for c in range(4)]

    legend_handles = []
    for ax, (ts_dict, ylabel, panel_title) in zip(axes_ts, panel_data):
        # Background grey lines for non-highlighted protocols
        for proto in PROTOCOLS:
            if proto not in HIGHLIGHT_PROTOS:
                y = ts_dict[proto]
                ax.plot(bin_mids, y, color="lightgrey", lw=1, alpha=0.7)

        # Highlighted protocols
        for proto in HIGHLIGHT_PROTOS:
            y = ts_dict[proto]
            clr = GROUP_COLORS[proto]
            lbl = PROTO_LABELS[proto]
            ax.plot(bin_mids, y, color=clr, lw=2.0, label=lbl)
            if panel_title == "A  Scheduling load proxy":
                legend_handles.append(Line2D([0], [0], color=clr, lw=2, label=lbl))

        ax.set_xlabel("Time (min)", fontsize=9)
        ax.set_ylabel(ylabel, fontsize=8)
        ax.set_title(panel_title, fontsize=9, fontweight="bold")
        ax.grid(alpha=0.3)
        ax.set_xlim(0, MAX_T_S / 60)

    # Shared legend below timeseries panels
    legend_handles += [
        Line2D([0], [0], color="lightgrey", lw=1.5, label="Other protocols"),
    ]
    axes_ts[0].legend(handles=legend_handles, fontsize=7.5, loc="upper left",
                      title="Protocol", title_fontsize=8)

    fig.suptitle(
        "CL_O — §4.2 Mechanism Narrative\n"
        "Top: causal chain with Pearson r on each link  ·  "
        "Bottom: temporal evidence (highlighted = best & role-based groups)",
        fontsize=11, fontweight="bold",
    )

    _save(fig, out_dir / "CL_O_mechanism_narrative.png")


# ---------------------------------------------------------------------------
# Utilities
# ---------------------------------------------------------------------------

def _save(fig: plt.Figure, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(path, dpi=150, bbox_inches="tight")
    pdf_path = path.with_suffix(".pdf")
    fig.savefig(pdf_path, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved: {path} + {pdf_path.name}")


# ---------------------------------------------------------------------------
# Main entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(description="Cross-layer analysis plots for Test Round 2")
    parser.add_argument(
        "--input", default="experiment_data_collection/test_round2",
        help="Root folder containing per-run subfolders",
    )
    parser.add_argument(
        "--output", default="analysis/test_round2",
        help="Root output folder; figures saved to <output>/figures/cross_layer/",
    )
    parser.add_argument(
        "--bin-seconds", type=int, default=600,
        help="Bin width in seconds for timeseries plots (default: 600 = 10 min)",
    )
    args = parser.parse_args()

    data_root = Path(args.input)
    out_dir = Path(args.output) / "figures" / "cross_layer"
    out_dir.mkdir(parents=True, exist_ok=True)

    print("[Cross-Layer Analysis] Collecting per-run KPIs …")
    df = collect_per_run_kpis(data_root)
    print(f"  Collected {len(df)} rows from {df['protocol'].nunique()} protocols.")

    bin_sec = args.bin_seconds

    print("\n[CL-A] Protocol KPI overview …")
    plot_cl_a_kpi_overview(df, out_dir)

    print("[CL-B] Charging KPIs vs PDR scatter …")
    plot_cl_b_charging_vs_pdr_scatter(df, out_dir)

    print("[CL-C] E2E delay conditioning analysis …")
    plot_cl_c_conditioning_analysis(df, out_dir)

    print("[CL-D] Mechanism chain scatter …")
    plot_cl_d_mechanism_chain(df, out_dir)

    print("[CL-E] Correlation bar chart …")
    plot_cl_e_correlation_bar_chart(df, out_dir)

    print("[CL-F] PDR + depletion rate timeseries …")
    plot_cl_f_pdr_vs_depletions_timeseries(data_root, out_dir, bin_sec)

    print("[CL-G] Routing drop rate timeseries …")
    plot_cl_g_routing_drop_timeseries(data_root, out_dir, bin_sec)

    print("[CL-H] Lagged correlation summary (§4.4) …")
    try:
        plot_cl_h_lagged_corr_summary(data_root, out_dir)
    except Exception:
        import traceback; traceback.print_exc()

    print("[CL-I] Epoch-aligned PDR around depletion bursts (§4.4) …")
    try:
        plot_cl_i_epoch_aligned_pdr(data_root, out_dir)
    except Exception:
        import traceback; traceback.print_exc()

    print("[CL-J] ugv_edf_3 cascade annotated timeseries (§4.4 / Anomaly A) …")
    try:
        plot_cl_j_ugv_edf3_cascade(data_root, out_dir)
    except Exception:
        import traceback; traceback.print_exc()

    print("[CL-K] Conditioning-aware delay robustness panel …")
    try:
        plot_cl_k_delay_robustness_panel(data_root, out_dir)
    except Exception:
        import traceback; traceback.print_exc()

    print("[CL-L] Event-aligned composite (per protocol + merged) …")
    try:
        plot_cl_l_event_aligned_composite(data_root, out_dir, bin_sec)
    except Exception:
        import traceback; traceback.print_exc()

    print("[CL-M] Role-stratified scheduling audit (CH priority paradox) …")
    try:
        plot_cl_m_role_scheduling_audit(data_root, out_dir)
    except Exception:
        import traceback; traceback.print_exc()

    print("[CL-N] CH depletion synchronization → cascade analysis …")
    try:
        plot_cl_n_ch_sync_cascade(data_root, out_dir)
    except Exception:
        import traceback; traceback.print_exc()

    print("[CL-O] §4.2 Mechanism narrative figure …")
    try:
        plot_cl_o_mechanism_narrative(df, data_root, out_dir, bin_sec)
    except Exception:
        import traceback; traceback.print_exc()

    print(f"\nDone. All cross-layer figures saved to: {out_dir}/")


if __name__ == "__main__":
    main()
