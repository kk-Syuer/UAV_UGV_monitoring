#!/usr/bin/env python3
"""
plot_cross_layer_analysis.py
----------------------------
Produces cross-layer analysis figures that link UAV charging scheduling KPIs
to network quality metrics (PDR, E2E delay) and fleet survivability.

This script is part of the Group 09 (cross-layer) analysis for Test Round 2.

Outputs (saved to <output_root>/figures/cross_layer/):
    CL_A_protocol_kpi_overview.png        -- Bar chart: PDR, depletions, success rate
    CL_B_charging_vs_pdr_scatter.png      -- Scatter matrix: charging KPIs vs PDR
    CL_C_e2e_delay_conditioning_analysis.png -- Conditioning bias analysis
    CL_D_mechanism_chain.png              -- Three-step mechanism chain scatter
    CL_E_correlation_bar_chart.png        -- Pearson + Spearman bar chart
    CL_F_pdr_vs_depletions_timeseries.png -- PDR timeseries + depletion rate overlay
    CL_G_routing_drop_timeseries.png      -- Routing drop rate over time per protocol

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
            folder = data_root / f"{proto}_{r}"
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
    mean_df = df.groupby("protocol").mean(numeric_only=True)
    std_df = df.groupby("protocol").std(numeric_only=True)
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
            net = _load_csv(data_root / f"{proto}_{r}", "network_timeseries.csv")
            if not net.empty and "window_pdr" in net.columns:
                net = net[net["window_pdr"] >= 0]
                pdr_binned = []
                for b0, b1 in zip(bins[:-1], bins[1:]):
                    seg = net[(net["t_rel_s"] >= b0) & (net["t_rel_s"] < b1)]
                    pdr_binned.append(seg["window_pdr"].mean() if len(seg) > 0 else np.nan)
                pdr_grids.append(pdr_binned)

            de = _load_csv(data_root / f"{proto}_{r}", "death_events.csv")
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
            ce = _load_csv(data_root / f"{proto}_{r}", "charge_events.csv")
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
# Utilities
# ---------------------------------------------------------------------------

def _save(fig: plt.Figure, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved: {path}")


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

    print(f"\nDone. All cross-layer figures saved to: {out_dir}/")


if __name__ == "__main__":
    main()
