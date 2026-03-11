#!/usr/bin/env python3
"""
generate_r2_vs_r3_comparison.py
--------------------------------
Generates Round 2 vs Round 3 side-by-side comparison figures for all
key metrics, saving outputs to analysis/test_round2vs3/.

Figures produced (prefixed with "cmp_"):
    cmp_01_mean_pdr_merged                    -- grouped bar (R2 solid / R3 hatched)
    cmp_01_network_pdr_over_time_merged       -- two-panel timeseries (no target line)
    cmp_02_charge_success_rate_merged         -- grouped bar
    cmp_03_charge_outcome_breakdown_merged    -- interleaved proportional stacked bars
    cmp_CL_G_routing_drop_timeseries          -- two-panel timeseries
    cmp_03_dead_uav_cumulative_merged         -- two-panel timeseries
    cmp_CL_F_pdr_vs_depletions_timeseries     -- 7 rows × 2 cols
    cmp_CL_B_charging_vs_pdr_scatter          -- 4×3 scatter (R2 top / R3 bottom)
    cmp_CL_E_correlation_bar_chart            -- grouped correlation bars
    cmp_CL_D_mechanism_chain                  -- 2×3 scatter (R2 top / R3 bottom)
    cmp_06_mean_e2e_delay_merged              -- grouped bar
    cmp_06_pdr_vs_e2e_scatter                 -- two-panel scatter
    cmp_CL_C_e2e_delay_conditioning_analysis  -- 2×2 (R2 top / R3 bottom)
    cmp_CL_K_delay_robustness                 -- 2×2 (R2 top / R3 bottom)
    cmp_04_policy_radar_merged                -- two-panel polar radar
    cmp_02_energy_recovered_merged            -- grouped boxplot

Usage
-----
    python scripts/generate_r2_vs_r3_comparison.py
"""

import re
import sys
import warnings
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np
import pandas as pd
from scipy import stats

SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR))

from plotting_utils import discover_runs, load_csv_if_exists, ensure_t_rel

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
R2_ROOT  = Path("experiment_data_collection/test_round2")
R3_ROOT  = Path("experiment_data_collection/test_round3")
OUT_ROOT = Path("analysis/test_round2vs3")
BIN_SEC  = 60.0

# ---------------------------------------------------------------------------
# Protocol styling (consistent across both rounds)
# ---------------------------------------------------------------------------
PROTOCOLS = [
    "ugv_dynamic", "ugv_edf", "ugv_fcfs", "ugv_p_dynamic_score",
    "ugv_p_edf", "ugv_p_role_priority", "ugv_role_priority",
]
PROTO_LABELS = {
    "ugv_dynamic":         "Dynamic",
    "ugv_edf":             "EDF",
    "ugv_fcfs":            "FCFS",
    "ugv_p_dynamic_score": "P-Dynamic",
    "ugv_p_edf":           "P-EDF",
    "ugv_p_role_priority": "P-RolePrio",
    "ugv_role_priority":   "RolePrio",
}
_CMAP = plt.get_cmap("tab10").colors
PROTO_COLORS = {p: _CMAP[i] for i, p in enumerate(PROTOCOLS)}


# ---------------------------------------------------------------------------
# Run discovery
# ---------------------------------------------------------------------------
def _discover_r2():
    return discover_runs(R2_ROOT)


def _discover_r3():
    raw = discover_runs(R3_ROOT)
    runs = []
    for run in raw:
        if "stormy" in run["folder"].name.lower():
            continue
        proto = re.sub(r"_sunny$", "", run["protocol"])
        run["protocol"] = proto
        run["label"] = f"{proto} r{run['replicate']}"
        runs.append(run)
    return runs


# ---------------------------------------------------------------------------
# Shared data helpers
# ---------------------------------------------------------------------------
def _group(runs: list) -> dict:
    groups: dict = {}
    for run in runs:
        groups.setdefault(run["protocol"], []).append(run)
    return groups


def _load(run: dict, filename: str) -> "pd.DataFrame | None":
    path = run["folder"] / filename
    df = load_csv_if_exists(path, missing_report=[])
    if df is not None:
        ensure_t_rel(df)
    return df


def _ts_interp(t_arr, y_arr, t_grid):
    mask = ~np.isnan(y_arr)
    if mask.sum() < 2:
        return np.full(len(t_grid), np.nan)
    return np.interp(t_grid, t_arr[mask], y_arr[mask], left=np.nan, right=np.nan)


def _merge_ts(run_list, filename, col):
    """Return (t_grid, mean, std) for a timeseries column across replicates."""
    series, t_max = [], 0.0
    for run in run_list:
        df = _load(run, filename)
        if df is None or col not in df.columns or "t_rel_s" not in df.columns:
            continue
        df = df[df[col] >= 0].dropna(subset=[col, "t_rel_s"])
        if df.empty:
            continue
        t, y = df["t_rel_s"].values, df[col].values
        t_max = max(t_max, t.max())
        series.append((t, y))
    if not series:
        return None, None, None
    t_grid = np.arange(0, t_max + BIN_SEC, BIN_SEC)
    arr = np.array([_ts_interp(t, y, t_grid) for t, y in series])
    with warnings.catch_warnings():
        warnings.simplefilter("ignore", RuntimeWarning)
        mean = np.nanmean(arr, axis=0)
        std = np.nan_to_num(np.nanstd(arr, axis=0), nan=0.0)
    return t_grid, mean, std


def _pdr_per_proto(runs):
    result = {}
    for proto, run_list in _group(runs).items():
        pdrs = []
        for run in run_list:
            df = _load(run, "qos_metrics.csv")
            if df is None or "generated" not in df.columns:
                continue
            g, d = df["generated"].sum(), df["delivered"].sum()
            if g > 0:
                pdrs.append(d / g)
        if pdrs:
            result[proto] = pdrs
    return result


def _success_rate_per_proto(runs):
    result = {}
    for proto, run_list in _group(runs).items():
        rates = []
        for run in run_list:
            df = _load(run, "charge_events.csv")
            if df is None or "outcome" not in df.columns:
                continue
            total = len(df)
            if total > 0:
                rates.append((df["outcome"] == "STARTED").sum() / total)
        if rates:
            result[proto] = rates
    return result


def _e2e_delay_per_proto(runs):
    result = {}
    for proto, run_list in _group(runs).items():
        means = []
        for run in run_list:
            df = _load(run, "network_timeseries.csv")
            if df is None or "window_delay_mean_ms" not in df.columns:
                continue
            pos = df[df["window_delay_mean_ms"] >= 0]["window_delay_mean_ms"]
            if len(pos) > 0:
                means.append(float(pos.mean()))
        if means:
            result[proto] = means
    return result


def _energy_per_proto(runs):
    result = {}
    for proto, run_list in _group(runs).items():
        all_vals = []
        for run in run_list:
            df = _load(run, "charge_session_events.csv")
            if (df is not None and "event_type" in df.columns
                    and "energy_charged_wh" in df.columns):
                ended = df[df["event_type"] == "DOCK_END"]
                vals = ended["energy_charged_wh"].replace(-1, np.nan).dropna()
                all_vals.extend(vals[vals > 0].tolist())
            else:
                df2 = _load(run, "charge_events.csv")
                if df2 is not None:
                    col = next(
                        (c for c in ("energy_recovered_pct", "energy_recovered")
                         if c in df2.columns), None)
                    if col:
                        vals = df2[col].replace(-1, np.nan).dropna()
                        all_vals.extend(vals.tolist())
        if all_vals:
            result[proto] = all_vals
    return result


def _kpi_df(runs) -> pd.DataFrame:
    """Build per-run KPI DataFrame."""
    rows = []
    for run in runs:
        row = {"protocol": run["protocol"], "run": run["replicate"]}

        qos = _load(run, "qos_metrics.csv")
        if qos is not None and "generated" in qos.columns:
            g, d = qos["generated"].sum(), qos["delivered"].sum()
            row["pdr"] = d / g if g > 0 else np.nan
        else:
            row["pdr"] = np.nan

        net = _load(run, "network_timeseries.csv")
        if net is not None and "window_delay_mean_ms" in net.columns:
            pos = net[net["window_delay_mean_ms"] >= 0]["window_delay_mean_ms"]
            row["e2e_delay_mean"] = float(pos.mean()) if len(pos) > 0 else np.nan
        else:
            row["e2e_delay_mean"] = np.nan

        ce = _load(run, "charge_events.csv")
        if ce is not None and "outcome" in ce.columns:
            total = len(ce)
            row["charge_success_rate"]      = (ce["outcome"] == "STARTED").sum() / total
            row["charge_timeout_rate"]      = (ce["outcome"] == "TIMEOUT").sum() / total
            row["charge_routing_drop_rate"] = (ce["outcome"] == "ROUTING_DROP").sum() / total
            if "decision_latency_ms" in ce.columns:
                dl = ce[ce["decision_latency_ms"] > 0]["decision_latency_ms"]
                row["decision_latency_mean"] = float(dl.mean()) if len(dl) > 0 else np.nan
            else:
                row["decision_latency_mean"] = np.nan
            if "effective_wait_ms" in ce.columns:
                ew = ce[ce["effective_wait_ms"] > 0]["effective_wait_ms"]
                row["effective_wait_mean"] = float(ew.mean()) if len(ew) > 0 else np.nan
            else:
                row["effective_wait_mean"] = np.nan
        else:
            for k in ["charge_success_rate", "charge_timeout_rate",
                      "charge_routing_drop_rate", "decision_latency_mean",
                      "effective_wait_mean"]:
                row[k] = np.nan

        de = _load(run, "death_events.csv")
        row["total_deaths"] = float(len(de)) if de is not None else np.nan

        cse = _load(run, "charge_session_events.csv")
        if (cse is not None and "event_type" in cse.columns
                and "energy_charged_wh" in cse.columns):
            ended = cse[cse["event_type"] == "DOCK_END"]
            vals = ended["energy_charged_wh"].replace(-1, np.nan).dropna()
            good = vals[vals > 0]
            row["energy_per_session"] = float(good.mean()) if len(good) > 0 else np.nan
        else:
            row["energy_per_session"] = np.nan

        cq = _load(run, "charge_queue_timeseries.csv")
        if cq is not None and "ugv_dock_utilization" in cq.columns:
            du = cq["ugv_dock_utilization"].replace(-1, np.nan).dropna()
            row["dock_util_mean"] = float(du.mean()) if len(du) > 0 else np.nan
        else:
            row["dock_util_mean"] = np.nan

        rows.append(row)
    return pd.DataFrame(rows)


def _save(fig, name: str):
    OUT_ROOT.mkdir(parents=True, exist_ok=True)
    fig.savefig(OUT_ROOT / f"{name}.png", dpi=150, bbox_inches="tight")
    fig.savefig(OUT_ROOT / f"{name}.pdf", bbox_inches="tight")
    plt.close(fig)
    print(f"  [OK] {name}")


# ---------------------------------------------------------------------------
# Grouped-bar helper (R2 solid | R3 hatched)
# ---------------------------------------------------------------------------
def _grouped_bar(ax, protos, r2_means, r2_errs, r3_means, r3_errs, ylabel, title,
                 fmt=".3f"):
    x, w = np.arange(len(protos)), 0.35
    colors = [PROTO_COLORS[p] for p in protos]
    ax.bar(x - w / 2, r2_means, w, color=colors, edgecolor="white", linewidth=0.5,
           yerr=r2_errs, capsize=4,
           error_kw={"elinewidth": 1.2, "ecolor": "black"})
    ax.bar(x + w / 2, r3_means, w, color=colors, edgecolor="white", linewidth=0.5,
           hatch="///", alpha=0.75,
           yerr=r3_errs, capsize=4,
           error_kw={"elinewidth": 1.2, "ecolor": "black"})
    # Value labels above each bar (offset by error bar height + 1% of data range)
    all_tops = [v + e for v, e in zip(r2_means + r3_means, r2_errs + r3_errs)
                if not np.isnan(v)]
    pad = (max(all_tops) - min(r2_means + r3_means, default=0)) * 0.02 if all_tops else 0
    for xi, (m2, e2, m3, e3) in enumerate(zip(r2_means, r2_errs, r3_means, r3_errs)):
        if not np.isnan(m2):
            ax.text(xi - w / 2, m2 + (e2 or 0) + pad,
                    format(m2, fmt), ha="center", va="bottom", fontsize=7,
                    fontweight="bold", color="black")
        if not np.isnan(m3):
            ax.text(xi + w / 2, m3 + (e3 or 0) + pad,
                    format(m3, fmt), ha="center", va="bottom", fontsize=7,
                    fontweight="bold", color="dimgray")
    ax.set_xticks(x)
    ax.set_xticklabels([PROTO_LABELS.get(p, p) for p in protos], rotation=25, ha="right")
    ax.set_ylabel(ylabel)
    ax.set_title(title)


def _round_legend(ax):
    """Append Round 2 / Round 3 hatch legend to existing handles."""
    h, l = ax.get_legend_handles_labels()
    rp = [mpatches.Patch(facecolor="grey", label="Round 2"),
          mpatches.Patch(facecolor="grey", hatch="///", alpha=0.75, label="Round 3")]
    ax.legend(handles=h + rp, fontsize=8, bbox_to_anchor=(1.01, 1), loc="upper left")


def _proto_legend(ax, protos):
    cp = [mpatches.Patch(color=PROTO_COLORS[p], label=PROTO_LABELS.get(p, p))
          for p in protos]
    rp = [mpatches.Patch(facecolor="grey", label="Round 2"),
          mpatches.Patch(facecolor="grey", hatch="///", alpha=0.75, label="Round 3")]
    ax.legend(handles=cp + rp, fontsize=8, bbox_to_anchor=(1.01, 1), loc="upper left")


# ---------------------------------------------------------------------------
# Two-panel timeseries helper
# ---------------------------------------------------------------------------
def _ts_panel(ax, runs, filename, col, title, ylim=None):
    groups = _group(runs)
    for proto in PROTOCOLS:
        run_list = groups.get(proto)
        if not run_list:
            continue
        t_grid, mean, _ = _merge_ts(run_list, filename, col)
        if t_grid is None:
            continue
        valid = ~np.isnan(mean)
        ax.plot(t_grid[valid] / 60, mean[valid],
                color=PROTO_COLORS[proto], linewidth=1.6,
                label=PROTO_LABELS.get(proto, proto))
    ax.set_xlabel("Time (min)")
    ax.set_title(title, fontweight="bold")
    if ylim:
        ax.set_ylim(*ylim)
    ax.grid(alpha=0.3)


def _divider(fig):
    fig.add_artist(
        plt.Line2D([0.05, 0.95], [0.51, 0.51],
                   transform=fig.transFigure,
                   color="black", linewidth=1.5, linestyle="--"))


# ===========================================================================
# 1.  Mean PDR — grouped bar
# ===========================================================================
def cmp_01_mean_pdr_merged(r2_runs, r3_runs):
    r2d, r3d = _pdr_per_proto(r2_runs), _pdr_per_proto(r3_runs)
    protos = [p for p in PROTOCOLS if p in r2d or p in r3d]

    def _stats(d, p):
        v = d.get(p, [])
        return (float(np.mean(v)) if v else np.nan,
                float(np.std(v)) if len(v) > 1 else 0.0)

    r2_m, r2_e, r3_m, r3_e = [], [], [], []
    for p in protos:
        m2, e2 = _stats(r2d, p)
        m3, e3 = _stats(r3d, p)
        r2_m.append(m2); r2_e.append(e2)
        r3_m.append(m3); r3_e.append(e3)

    fig, ax = plt.subplots(figsize=(max(8, len(protos) * 2), 5))
    _grouped_bar(ax, protos, r2_m, r2_e, r3_m, r3_e,
                 "Mean overall PDR (mean ± std across replicates)",
                 "Mean PDR Comparison — Round 2 vs Round 3",
                 fmt=".3f")
    ax.set_ylim(0, 1.0)
    _proto_legend(ax, protos)
    fig.tight_layout()
    _save(fig, "cmp_01_mean_pdr_merged")


# ===========================================================================
# 2.  Network PDR over time — two-panel (NO red target line)
# ===========================================================================
def cmp_01_network_pdr_over_time_merged(r2_runs, r3_runs):
    fig, (ax2, ax3) = plt.subplots(1, 2, figsize=(18, 5), sharey=True)
    _ts_panel(ax2, r2_runs, "network_timeseries.csv", "window_pdr",
              "Round 2", ylim=(-0.05, 1.05))
    _ts_panel(ax3, r3_runs, "network_timeseries.csv", "window_pdr",
              "Round 3", ylim=(-0.05, 1.05))
    ax2.set_ylabel("Window PDR (mean across replicates)")
    handles, labels = ax2.get_legend_handles_labels()
    if not handles:
        handles, labels = ax3.get_legend_handles_labels()
    fig.legend(handles, labels, title="Protocol", loc="center right",
               fontsize=8, bbox_to_anchor=(1.0, 0.5))
    fig.suptitle("Network PDR Over Time — Round 2 (left) vs Round 3 (right)",
                 fontsize=13, fontweight="bold")
    fig.tight_layout()
    _save(fig, "cmp_01_network_pdr_over_time_merged")


# ===========================================================================
# 3.  Charge success rate — grouped bar
# ===========================================================================
def cmp_02_charge_success_rate_merged(r2_runs, r3_runs):
    r2d, r3d = _success_rate_per_proto(r2_runs), _success_rate_per_proto(r3_runs)
    protos = [p for p in PROTOCOLS if p in r2d or p in r3d]

    def _stats(d, p):
        v = d.get(p, [])
        return (float(np.nanmean(v)) if v else np.nan,
                float(np.nanstd(v)) if len(v) > 1 else 0.0)

    r2_m, r2_e, r3_m, r3_e = [], [], [], []
    for p in protos:
        m2, e2 = _stats(r2d, p)
        m3, e3 = _stats(r3d, p)
        r2_m.append(m2); r2_e.append(e2)
        r3_m.append(m3); r3_e.append(e3)

    fig, ax = plt.subplots(figsize=(max(8, len(protos) * 2), 5))
    _grouped_bar(ax, protos, r2_m, r2_e, r3_m, r3_e,
                 "Charge success rate (mean ± std)",
                 "Charge Success Rate — Round 2 vs Round 3",
                 fmt=".1%")
    ax.set_ylim(0, 1.0)
    _proto_legend(ax, protos)
    fig.tight_layout()
    _save(fig, "cmp_02_charge_success_rate_merged")


# ===========================================================================
# 4.  Charge outcome breakdown — interleaved proportional stacked bars
# ===========================================================================
def cmp_03_charge_outcome_breakdown_merged(r2_runs, r3_runs):
    ALL_OUTCOMES = ["STARTED", "REJECTED", "DROPPED", "TIMEOUT",
                    "PREEMPTED", "ENERGY_DEPLETED"]
    PALETTE = {
        "STARTED":         "#2ca02c",
        "REJECTED":        "#d62728",
        "DROPPED":         "#ff7f0e",
        "TIMEOUT":         "#9467bd",
        "PREEMPTED":       "#8c564b",
        "ENERGY_DEPLETED": "#1f77b4",
    }
    OUT_LABELS = {
        "STARTED": "STARTED", "REJECTED": "REJECTED", "DROPPED": "ROUTING_DROP",
        "TIMEOUT": "TIMEOUT", "PREEMPTED": "PREEMPTED",
        "ENERGY_DEPLETED": "ENERGY_DEPLETED",
    }

    def _outcome_props(runs):
        groups = _group(runs)
        any_pre = any("_p_" in p for p in groups)
        rows = {}
        for proto, run_list in groups.items():
            is_pre = "_p_" in proto
            cnt = {o: 0 for o in ALL_OUTCOMES}
            for run in run_list:
                df = _load(run, "charge_events.csv")
                if df is None or "outcome" not in df.columns:
                    continue
                vc = df["outcome"].value_counts()
                for o in ALL_OUTCOMES:
                    cnt[o] += int(vc.get(o, 0)) if (o != "PREEMPTED" or is_pre) else 0
            total = sum(cnt[o] for o in ALL_OUTCOMES
                        if o != "PREEMPTED" or any_pre)
            rows[proto] = {o: cnt[o] / total if total > 0 else 0.0
                           for o in ALL_OUTCOMES}
        return rows, any_pre

    r2_props, r2_pre = _outcome_props(r2_runs)
    r3_props, r3_pre = _outcome_props(r3_runs)
    any_pre = r2_pre or r3_pre
    outcomes = [o for o in ALL_OUTCOMES if o != "PREEMPTED" or any_pre]
    protos = [p for p in PROTOCOLS if p in r2_props or p in r3_props]
    n = len(protos)

    # interleaved x: per protocol two bars (R2 then R3)
    w = 0.38
    x_r2 = np.arange(n) * 2.2
    x_r3 = x_r2 + w + 0.05

    fig, ax = plt.subplots(figsize=(max(12, n * 3), 7))
    bottom_r2 = np.zeros(n)
    bottom_r3 = np.zeros(n)
    drawn = set()

    for outcome in outcomes:
        v2 = np.array([r2_props.get(p, {}).get(outcome, 0.0) for p in protos])
        v3 = np.array([r3_props.get(p, {}).get(outcome, 0.0) for p in protos])
        lbl = OUT_LABELS[outcome]
        col = PALETTE[outcome]
        kw = dict(label=lbl) if lbl not in drawn else {}
        ax.bar(x_r2, v2, w, bottom=bottom_r2, color=col, edgecolor="white", **kw)
        if kw:
            drawn.add(lbl)
        ax.bar(x_r3, v3, w, bottom=bottom_r3, color=col, edgecolor="white",
               hatch="///", alpha=0.85)
        for bar, v, b in zip(
                ax.patches[-2 * n: -n], v2, bottom_r2):  # annotate R2 bars
            if v >= 0.05:
                ax.text(bar.get_x() + w / 2, b + v / 2, f"{v:.0%}",
                        ha="center", va="center",
                        fontsize=6, color="white", fontweight="bold")
        for bar, v, b in zip(ax.patches[-n:], v3, bottom_r3):
            if v >= 0.05:
                ax.text(bar.get_x() + w / 2, b + v / 2, f"{v:.0%}",
                        ha="center", va="center",
                        fontsize=6, color="white", fontweight="bold")
        bottom_r2 += v2
        bottom_r3 += v3

    ax.set_xticks((x_r2 + x_r3) / 2)
    ax.set_xticklabels([PROTO_LABELS.get(p, p) for p in protos],
                       rotation=20, ha="right")
    ax.set_ylim(0, 1)
    ax.yaxis.set_major_formatter(plt.FuncFormatter(lambda y, _: f"{y:.0%}"))
    ax.set_ylabel("Fraction of total charge requests")
    ax.set_title("Charge Request Outcomes — Round 2 (solid) vs Round 3 (hatched)",
                 fontsize=12, fontweight="bold")
    outcome_handles, outcome_labels = ax.get_legend_handles_labels()
    rp = [mpatches.Patch(facecolor="grey", label="Round 2"),
          mpatches.Patch(facecolor="grey", hatch="///", alpha=0.75, label="Round 3")]
    ax.legend(handles=outcome_handles + rp,
              labels=outcome_labels + ["Round 2", "Round 3"],
              fontsize=8, loc="upper right")
    fig.tight_layout()
    _save(fig, "cmp_03_charge_outcome_breakdown_merged")


# ===========================================================================
# 5.  CL_G — routing drop rate timeseries, two-panel
# ===========================================================================
def cmp_CL_G_routing_drop_timeseries(r2_runs, r3_runs):
    fig, (ax2, ax3) = plt.subplots(1, 2, figsize=(18, 5), sharey=True)

    def _plot(ax, runs, title):
        groups = _group(runs)
        bins = np.arange(0, 180 * 60 + BIN_SEC, BIN_SEC)
        centers = (bins[:-1] + bins[1:]) / 2 / 60
        for proto in PROTOCOLS:
            run_list = groups.get(proto)
            if not run_list:
                continue
            all_rates = []
            for run in run_list:
                df = _load(run, "charge_events.csv")
                if df is None or "outcome" not in df.columns or "t_rel_s" not in df.columns:
                    continue
                bin_rates = [
                    (df[(df["t_rel_s"] >= b0) & (df["t_rel_s"] < b1)]["outcome"]
                     == "ROUTING_DROP").mean()
                    if len(df[(df["t_rel_s"] >= b0) & (df["t_rel_s"] < b1)]) > 0
                    else np.nan
                    for b0, b1 in zip(bins[:-1], bins[1:])
                ]
                all_rates.append(bin_rates)
            if all_rates:
                mean_rd = np.nanmean(all_rates, axis=0)
                ax.plot(centers, mean_rd,
                        color=PROTO_COLORS[proto], linewidth=1.8,
                        label=PROTO_LABELS.get(proto, proto))
        ax.set_xlabel("Time (min)")
        ax.set_title(title, fontweight="bold")
        ax.set_ylim(0, 1.0)
        ax.grid(alpha=0.3)

    _plot(ax2, r2_runs, "Round 2")
    _plot(ax3, r3_runs, "Round 3")
    ax2.set_ylabel("Charge Request ROUTING_DROP Rate")
    handles, labels = ax2.get_legend_handles_labels()
    fig.legend(handles, labels, title="Protocol", loc="center right",
               fontsize=8, bbox_to_anchor=(1.0, 0.5))
    fig.suptitle("Routing Drop Rate Over Time — Round 2 (left) vs Round 3 (right)",
                 fontsize=13, fontweight="bold")
    fig.tight_layout()
    _save(fig, "cmp_CL_G_routing_drop_timeseries")


# ===========================================================================
# 6.  Cumulative dead UAVs — two-panel timeseries
# ===========================================================================
def cmp_03_dead_uav_cumulative_merged(r2_runs, r3_runs):
    fig, (ax2, ax3) = plt.subplots(1, 2, figsize=(18, 5), sharey=True)

    def _plot(ax, runs, title):
        groups = _group(runs)
        for proto in PROTOCOLS:
            run_list = groups.get(proto)
            if not run_list:
                continue
            series, t_max = [], 0.0
            for run in run_list:
                df = _load(run, "death_events.csv")
                if df is not None and "t_rel_s" in df.columns and len(df) > 0:
                    df = df.sort_values("t_rel_s")
                    t = df["t_rel_s"].values
                    y = np.arange(1, len(df) + 1, dtype=float)
                    t_max = max(t_max, t[-1])
                    series.append((t, y))
            if not series:
                continue
            t_grid = np.arange(0, t_max + BIN_SEC, BIN_SEC)
            interped = []
            for t, y in series:
                idx = np.searchsorted(t, t_grid, side="right")
                y_ff = np.where(idx == 0, 0.0, y[np.minimum(idx - 1, len(y) - 1)])
                interped.append(y_ff)
            mean = np.mean(interped, axis=0)
            ax.plot(t_grid / 60, mean,
                    color=PROTO_COLORS[proto], linewidth=1.8,
                    label=PROTO_LABELS.get(proto, proto))
        ax.set_xlabel("Time (min)")
        ax.set_title(title, fontweight="bold")
        ax.grid(alpha=0.3)

    _plot(ax2, r2_runs, "Round 2")
    _plot(ax3, r3_runs, "Round 3")
    ax2.set_ylabel("Cumulative dead UAVs (mean across replicates)")
    handles, labels = ax2.get_legend_handles_labels()
    if not handles:
        handles, labels = ax3.get_legend_handles_labels()
    fig.legend(handles, labels, title="Protocol", loc="center right",
               fontsize=8, bbox_to_anchor=(1.0, 0.5))
    fig.suptitle("Cumulative UAV Deaths — Round 2 (left) vs Round 3 (right)",
                 fontsize=13, fontweight="bold")
    fig.tight_layout()
    _save(fig, "cmp_03_dead_uav_cumulative_merged")


# ===========================================================================
# 7.  CL_F — PDR + depletion timeseries per protocol, 7 rows × 2 cols
# ===========================================================================
def cmp_CL_F_pdr_vs_depletions_timeseries(r2_runs, r3_runs):
    n = len(PROTOCOLS)
    fig, axes = plt.subplots(n, 2, figsize=(20, 3 * n), sharex=True)
    fig.text(0.27, 1.001, "Round 2", ha="center", fontsize=13, fontweight="bold")
    fig.text(0.73, 1.001, "Round 3", ha="center", fontsize=13, fontweight="bold")

    bins = np.arange(0, 180 * 60 + BIN_SEC, BIN_SEC)
    centers = (bins[:-1] + bins[1:]) / 2 / 60

    for ri, proto in enumerate(PROTOCOLS):
        for ci, (runs, lbl) in enumerate([(r2_runs, "R2"), (r3_runs, "R3")]):
            ax = axes[ri][ci]
            ax_twin = ax.twinx()
            run_list = _group(runs).get(proto, [])
            color = PROTO_COLORS[proto]

            pdr_grids, death_bins = [], []
            for run in run_list:
                net = _load(run, "network_timeseries.csv")
                if net is not None and "window_pdr" in net.columns:
                    pos = net[net["window_pdr"] >= 0]
                    pdr_grids.append([
                        pos[(pos["t_rel_s"] >= b0) & (pos["t_rel_s"] < b1)]["window_pdr"].mean()
                        if len(pos[(pos["t_rel_s"] >= b0) & (pos["t_rel_s"] < b1)]) > 0
                        else np.nan
                        for b0, b1 in zip(bins[:-1], bins[1:])
                    ])
                de = _load(run, "death_events.csv")
                if de is not None and "t_rel_s" in de.columns and len(de) > 0:
                    death_bins.append(np.histogram(de["t_rel_s"].values, bins=bins)[0])

            if pdr_grids:
                with warnings.catch_warnings():
                    warnings.simplefilter("ignore", RuntimeWarning)
                    mean_pdr = np.nanmean(pdr_grids, axis=0)
                    std_pdr = np.nanstd(pdr_grids, axis=0)
                ax.plot(centers, mean_pdr, color=color, linewidth=2)
                ax.fill_between(centers, mean_pdr - std_pdr, mean_pdr + std_pdr,
                                alpha=0.2, color=color)
            ax.set_ylim(-0.05, 1.1)
            ax.set_ylabel("PDR", fontsize=8)
            ax.set_title(f"{PROTO_LABELS.get(proto, proto)} ({lbl})",
                         fontsize=9, fontweight="bold", loc="left")
            if death_bins:
                mean_d = np.mean(death_bins, axis=0)
                ax_twin.bar(centers, mean_d,
                            width=BIN_SEC / 60 * 0.8, alpha=0.35, color="firebrick")
                ax_twin.set_ylim(0, max(10.0, float(mean_d.max()) * 3))
            ax_twin.set_ylabel("Depletions/bin", fontsize=7, color="firebrick")
            ax_twin.tick_params(axis="y", colors="firebrick")

    for ci in range(2):
        axes[-1][ci].set_xlabel("Time (minutes)", fontsize=9)
    fig.suptitle(
        "PDR Timeseries + Battery Depletion Rate — "
        "Round 2 (left) vs Round 3 (right)",
        fontsize=13, fontweight="bold", y=1.002)
    fig.tight_layout()
    _save(fig, "cmp_CL_F_pdr_vs_depletions_timeseries")


# ===========================================================================
# 8.  CL_B — charging KPI vs PDR scatter, 4 rows × 3 cols
# ===========================================================================
def cmp_CL_B_charging_vs_pdr_scatter(r2_runs, r3_runs):
    pairs = [
        ("charge_success_rate",      "pdr", "Charge Success Rate",   "PDR"),
        ("charge_routing_drop_rate", "pdr", "Routing Drop Rate",     "PDR"),
        ("total_deaths",             "pdr", "Battery Depletions",    "PDR"),
        ("energy_per_session",       "pdr", "Energy/Session (Wh)",   "PDR"),
        ("dock_util_mean",           "pdr", "Mean Dock Utilization", "PDR"),
        ("effective_wait_mean",      "pdr", "Mean Effective Wait (ms)", "PDR"),
    ]
    df2, df3 = _kpi_df(r2_runs), _kpi_df(r3_runs)
    fig, axes = plt.subplots(4, 3, figsize=(16, 18))

    # Rows 0-1 = R2 (6 panels), rows 2-3 = R3 (6 panels)
    for rnd_off, (df, rnd_label) in enumerate([(df2, "Round 2"), (df3, "Round 3")]):
        for pi, (xvar, yvar, xlabel, ylabel) in enumerate(pairs):
            row = rnd_off * 2 + pi // 3
            col = pi % 3
            ax = axes[row][col]
            sub = df[[xvar, yvar, "protocol"]].dropna()
            for proto in PROTOCOLS:
                pts = sub[sub["protocol"] == proto]
                ax.scatter(pts[xvar], pts[yvar],
                           color=PROTO_COLORS[proto],
                           label=PROTO_LABELS.get(proto, proto),
                           s=60, alpha=0.85)
            if len(sub) >= 4:
                pr, _ = stats.pearsonr(sub[xvar], sub[yvar])
                m, b, *_ = stats.linregress(sub[xvar], sub[yvar])
                xl = np.linspace(sub[xvar].min(), sub[xvar].max(), 100)
                ax.plot(xl, m * xl + b, "k--", alpha=0.5, linewidth=1.2)
                ax.set_title(f"[{rnd_label}] r = {pr:.2f}", fontsize=9, fontweight="bold")
            ax.set_xlabel(xlabel, fontsize=8)
            ax.set_ylabel(ylabel, fontsize=8)
            ax.grid(alpha=0.3)

    handles = [plt.scatter([], [], color=PROTO_COLORS[p],
                           label=PROTO_LABELS.get(p, p)) for p in PROTOCOLS]
    fig.legend(handles=handles, title="Protocol", fontsize=8, title_fontsize=9,
               loc="center right", bbox_to_anchor=(1.02, 0.5))
    _divider(fig)
    fig.text(0.5, 0.98, "Round 2", ha="center", fontsize=12, fontweight="bold")
    fig.text(0.5, 0.50, "Round 3", ha="center", fontsize=12, fontweight="bold")
    fig.suptitle("Charging KPIs vs PDR Scatter — Round 2 (top) vs Round 3 (bottom)",
                 fontsize=13, fontweight="bold", y=1.01)
    plt.tight_layout()
    _save(fig, "cmp_CL_B_charging_vs_pdr_scatter")


# ===========================================================================
# 9.  CL_E — correlation bar chart comparison
# ===========================================================================
def cmp_CL_E_correlation_bar_chart(r2_runs, r3_runs):
    kpi_keys = [
        "charge_success_rate", "charge_timeout_rate", "charge_routing_drop_rate",
        "decision_latency_mean", "effective_wait_mean", "total_deaths",
        "energy_per_session", "dock_util_mean",
    ]
    kpi_display = [
        "Success Rate", "Timeout Rate", "Routing Drop",
        "Dec. Latency", "Eff. Wait", "Depletions",
        "Energy/Sess.", "Dock Util",
    ]
    net_vars = [("pdr", "PDR"), ("e2e_delay_mean", "E2E Delay")]
    df2, df3 = _kpi_df(r2_runs), _kpi_df(r3_runs)

    fig, axes = plt.subplots(1, 2, figsize=(16, 6))
    x, w = np.arange(len(kpi_keys)), 0.20

    for ax_idx, (nv, nv_label) in enumerate(net_vars):
        ax = axes[ax_idx]
        for rnd_i, (df, rnd_label, base_col, light_col) in enumerate([
            (df2, "R2", "steelblue",  "lightsteelblue"),
            (df3, "R3", "coral",      "lightsalmon"),
        ]):
            p_rs, s_rs = [], []
            for kv in kpi_keys:
                sub = df[[nv, kv]].dropna()
                if len(sub) < 5:
                    p_rs.append(np.nan); s_rs.append(np.nan)
                else:
                    pr, _ = stats.pearsonr(sub[nv], sub[kv])
                    sr, _ = stats.spearmanr(sub[nv], sub[kv])
                    p_rs.append(pr); s_rs.append(sr)
            off_p = (rnd_i * 2 - 1.5) * w
            off_s = (rnd_i * 2 - 0.5) * w
            ax.bar(x + off_p, p_rs, w, label=f"Pearson r ({rnd_label})",
                   alpha=0.9, color=base_col)
            ax.bar(x + off_s, s_rs, w, label=f"Spearman ρ ({rnd_label})",
                   alpha=0.9, color=light_col, edgecolor="grey", linewidth=0.4)
        ax.axhline(0, color="black", linewidth=0.8)
        ax.axhline(0.3, color="gray", linewidth=0.5, linestyle="--", alpha=0.5)
        ax.axhline(-0.3, color="gray", linewidth=0.5, linestyle="--", alpha=0.5)
        ax.set_xticks(x)
        ax.set_xticklabels(kpi_display, rotation=40, ha="right", fontsize=8)
        ax.set_ylabel("Correlation coefficient")
        ax.set_title(f"Correlations with {nv_label}", fontsize=10, fontweight="bold")
        ax.legend(fontsize=7, ncol=2)
        ax.set_ylim(-1.0, 1.0)
        ax.grid(axis="y", alpha=0.3)

    fig.suptitle(
        "Cross-Layer Correlations: R2 (blue) vs R3 (coral)\n"
        "(solid = Pearson r,  lighter = Spearman ρ;  dashed = |r| = 0.3)",
        fontsize=12, fontweight="bold")
    plt.tight_layout()
    _save(fig, "cmp_CL_E_correlation_bar_chart")


# ===========================================================================
# 10.  CL_D — mechanism chain, 2 rows × 3 cols
# ===========================================================================
def cmp_CL_D_mechanism_chain(r2_runs, r3_runs):
    panel_specs = [
        ("charge_success_rate", "total_deaths",
         "Charge Success Rate", "Battery Depletions", "Charging → Fleet Survival"),
        ("total_deaths", "pdr",
         "Battery Depletions", "PDR", "Fleet Survival → Network PDR"),
        ("charge_routing_drop_rate", "pdr",
         "Routing Drop Rate", "PDR", "Routing Drops → Network PDR"),
    ]
    df2, df3 = _kpi_df(r2_runs), _kpi_df(r3_runs)
    fig, axes = plt.subplots(2, 3, figsize=(17, 10))

    for rnd_i, (df, rnd_label) in enumerate([(df2, "Round 2"), (df3, "Round 3")]):
        for col_i, (xvar, yvar, xlabel, ylabel, title) in enumerate(panel_specs):
            ax = axes[rnd_i][col_i]
            sub = df[[xvar, yvar, "protocol"]].dropna()
            for proto in PROTOCOLS:
                pts = sub[sub["protocol"] == proto]
                ax.scatter(pts[xvar], pts[yvar],
                           color=PROTO_COLORS[proto],
                           label=PROTO_LABELS.get(proto, proto),
                           s=70, alpha=0.85)
            if len(sub) >= 4:
                pr, _ = stats.pearsonr(sub[xvar], sub[yvar])
                sr, _ = stats.spearmanr(sub[xvar], sub[yvar])
                m, b, *_ = stats.linregress(sub[xvar], sub[yvar])
                xl = np.linspace(sub[xvar].min(), sub[xvar].max(), 100)
                ax.plot(xl, m * xl + b, "k--", alpha=0.5, linewidth=1.2)
                ax.set_title(f"[{rnd_label}] {title}\nr={pr:.2f}, ρ={sr:.2f}",
                             fontsize=9, fontweight="bold")
            ax.set_xlabel(xlabel, fontsize=9)
            ax.set_ylabel(ylabel, fontsize=9)
            ax.grid(alpha=0.3)

    handles = [plt.scatter([], [], color=PROTO_COLORS[p],
                           label=PROTO_LABELS.get(p, p)) for p in PROTOCOLS]
    axes[0][0].legend(handles=handles, title="Protocol", fontsize=7, title_fontsize=8)
    _divider(fig)
    fig.suptitle(
        "Mechanism Chain: Charging → Survivability → Network\n"
        "Round 2 (top) vs Round 3 (bottom)",
        fontsize=12, fontweight="bold")
    plt.tight_layout()
    _save(fig, "cmp_CL_D_mechanism_chain")


# ===========================================================================
# 11.  Mean E2E delay — grouped bar
# ===========================================================================
def cmp_06_mean_e2e_delay_merged(r2_runs, r3_runs):
    r2d, r3d = _e2e_delay_per_proto(r2_runs), _e2e_delay_per_proto(r3_runs)
    protos = [p for p in PROTOCOLS if p in r2d or p in r3d]

    def _stats(d, p):
        v = d.get(p, [])
        return (float(np.mean(v)) if v else np.nan,
                float(np.std(v)) if len(v) > 1 else 0.0)

    r2_m, r2_e, r3_m, r3_e = [], [], [], []
    for p in protos:
        m2, e2 = _stats(r2d, p)
        m3, e3 = _stats(r3d, p)
        r2_m.append(m2); r2_e.append(e2)
        r3_m.append(m3); r3_e.append(e3)

    fig, ax = plt.subplots(figsize=(max(8, len(protos) * 2), 5))
    _grouped_bar(ax, protos, r2_m, r2_e, r3_m, r3_e,
                 "Mean E2E delay (ms) (mean ± std across replicates)",
                 "Mean E2E Delay — Round 2 vs Round 3",
                 fmt=".1f")
    _proto_legend(ax, protos)
    fig.tight_layout()
    _save(fig, "cmp_06_mean_e2e_delay_merged")


# ===========================================================================
# 12.  PDR vs E2E scatter — two-panel
# ===========================================================================
def cmp_06_pdr_vs_e2e_scatter(r2_runs, r3_runs):
    df2, df3 = _kpi_df(r2_runs), _kpi_df(r3_runs)
    fig, (ax2, ax3) = plt.subplots(1, 2, figsize=(16, 6))

    def _plot(ax, df, title):
        sub = df[["protocol", "pdr", "e2e_delay_mean"]].dropna(
            subset=["pdr", "e2e_delay_mean"])
        for proto in PROTOCOLS:
            pts = sub[sub["protocol"] == proto]
            ax.scatter(pts["e2e_delay_mean"], pts["pdr"],
                       color=PROTO_COLORS[proto],
                       label=PROTO_LABELS.get(proto, proto), s=65)
        if len(sub) >= 4:
            m, b, r, p, _ = stats.linregress(sub["e2e_delay_mean"], sub["pdr"])
            xl = np.array([sub["e2e_delay_mean"].min(), sub["e2e_delay_mean"].max()])
            ax.plot(xl, m * xl + b, "k--", linewidth=1.2, alpha=0.7)
            p_str = f"p={p:.3f}" if p >= 0.001 else "p<0.001"
            ax.annotate(f"r={r:.3f}\n{p_str}", xy=(0.97, 0.97),
                        xycoords="axes fraction", ha="right", va="top", fontsize=9,
                        bbox=dict(boxstyle="round,pad=0.3",
                                  facecolor="white", edgecolor="grey", alpha=0.8))
        ax.set_xlabel("Mean E2E Delay (ms)")
        ax.set_ylabel("Mean PDR")
        ax.set_title(title, fontweight="bold")
        ax.grid(alpha=0.3)

    _plot(ax2, df2, "Round 2")
    _plot(ax3, df3, "Round 3")
    handles, labels = ax2.get_legend_handles_labels()
    fig.legend(handles, labels, title="Protocol", fontsize=8,
               bbox_to_anchor=(1.01, 0.5), loc="center left")
    fig.suptitle("PDR vs E2E Delay — Round 2 (left) vs Round 3 (right)",
                 fontsize=13, fontweight="bold")
    plt.tight_layout()
    _save(fig, "cmp_06_pdr_vs_e2e_scatter")


# ===========================================================================
# 13.  CL_C — E2E delay conditioning analysis, 2×2
# ===========================================================================
def cmp_CL_C_e2e_delay_conditioning_analysis(r2_runs, r3_runs):
    df2, df3 = _kpi_df(r2_runs), _kpi_df(r3_runs)
    fig, axes = plt.subplots(2, 2, figsize=(16, 12))

    def _plot(axes_row, df, rnd_label):
        ax_sc, ax_bb = axes_row
        sub = df[["protocol", "pdr", "e2e_delay_mean", "total_deaths"]].dropna(
            subset=["pdr", "e2e_delay_mean"])
        for proto in PROTOCOLS:
            pts = sub[sub["protocol"] == proto]
            ax_sc.scatter(pts["pdr"], pts["e2e_delay_mean"],
                          color=PROTO_COLORS[proto],
                          label=PROTO_LABELS.get(proto, proto), s=75, alpha=0.85)
        if len(sub) >= 4:
            pr, _ = stats.pearsonr(sub["pdr"], sub["e2e_delay_mean"])
            ax_sc.set_title(f"[{rnd_label}] E2E Delay vs PDR  (r = {pr:.2f})",
                            fontsize=10, fontweight="bold")
        ax_sc.set_xlabel("PDR")
        ax_sc.set_ylabel("Mean E2E Delay (ms)")
        ax_sc.legend(fontsize=7)
        ax_sc.grid(alpha=0.3)

        proto_means = df.groupby("protocol")[
            ["pdr", "e2e_delay_mean", "total_deaths"]].mean()
        for proto in PROTOCOLS:
            if proto not in proto_means.index:
                continue
            row = proto_means.loc[proto]
            size = max(10, float(row["total_deaths"]) * 10)
            ax_bb.scatter(float(row["pdr"]), float(row["e2e_delay_mean"]),
                          s=size, color=PROTO_COLORS[proto], alpha=0.8)
            ax_bb.annotate(PROTO_LABELS.get(proto, proto),
                           (float(row["pdr"]), float(row["e2e_delay_mean"])),
                           fontsize=8, xytext=(3, 3), textcoords="offset points")
        ax_bb.set_xlabel("Mean PDR")
        ax_bb.set_ylabel("Mean E2E Delay (ms)")
        ax_bb.set_title(f"[{rnd_label}] Protocol Means  (bubble ∝ depletions)",
                        fontsize=10, fontweight="bold")
        ax_bb.grid(alpha=0.3)

    _plot(axes[0], df2, "Round 2")
    _plot(axes[1], df3, "Round 3")
    _divider(fig)
    fig.suptitle("E2E Delay Conditioning Bias — Round 2 (top) vs Round 3 (bottom)",
                 fontsize=13, fontweight="bold")
    plt.tight_layout()
    _save(fig, "cmp_CL_C_e2e_delay_conditioning_analysis")


# ===========================================================================
# 14.  CL_K — delay robustness panel, 2×2
# ===========================================================================
def cmp_CL_K_delay_robustness(r2_runs, r3_runs):
    def _window_data(runs):
        records = []
        for run in runs:
            net = _load(run, "network_timeseries.csv")
            if net is None:
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
            valid = valid.assign(protocol=run["protocol"])
            records.append(
                valid[["protocol", "window_pdr",
                       "window_delay_mean_ms", "window_delivered"]])
        return pd.concat(records, ignore_index=True) if records else None

    def _bar_stats(all_wins):
        rows = []
        for proto in PROTOCOLS:
            sub = all_wins[all_wins["protocol"] == proto]
            if sub.empty:
                continue
            uw = float(sub["window_delay_mean_ms"].mean())
            w = sub["window_pdr"].values
            d = sub["window_delay_mean_ms"].values
            wt = float(np.dot(w, d) / w.sum()) if w.sum() > 0 else np.nan
            rows.append({"protocol": proto,
                         "label": PROTO_LABELS.get(proto, proto),
                         "color": PROTO_COLORS[proto],
                         "unweighted": uw, "weighted": wt})
        return pd.DataFrame(rows).sort_values("weighted").reset_index(drop=True)

    fig, axes = plt.subplots(2, 2, figsize=(18, 12))

    for row_i, (runs, rnd_label) in enumerate([(r2_runs, "Round 2"),
                                                (r3_runs, "Round 3")]):
        all_wins = _window_data(runs)
        ax_sc, ax_bar = axes[row_i]
        if all_wins is None:
            ax_sc.set_visible(False); ax_bar.set_visible(False)
            continue

        # scatter
        sc = ax_sc.scatter(all_wins["window_delivered"],
                           all_wins["window_delay_mean_ms"],
                           c=all_wins["window_pdr"], cmap="RdYlGn",
                           vmin=0, vmax=1, s=6, alpha=0.3, rasterized=True)
        plt.colorbar(sc, ax=ax_sc, label="window PDR")
        sub_reg = all_wins[["window_delivered", "window_delay_mean_ms"]].dropna()
        if len(sub_reg) >= 10:
            m, b, r_val, p_val, _ = stats.linregress(
                sub_reg["window_delivered"], sub_reg["window_delay_mean_ms"])
            xl = np.linspace(sub_reg["window_delivered"].min(),
                             sub_reg["window_delivered"].max(), 200)
            ax_sc.plot(xl, m * xl + b, "k--", linewidth=1.5,
                       label=f"r={r_val:.2f}")
            ax_sc.legend(fontsize=8)
        ax_sc.set_xlabel("Delivered packets per window (sample size)")
        ax_sc.set_ylabel("Window mean E2E delay (ms)")
        ax_sc.set_title(f"[{rnd_label}] Delay vs Sample Size",
                        fontsize=10, fontweight="bold")
        ax_sc.grid(alpha=0.3)

        # bar
        sdf = _bar_stats(all_wins)
        y, h = np.arange(len(sdf)), 0.32
        ax_bar.barh(y + h / 2, sdf["weighted"], h,
                    color=sdf["color"].values, alpha=0.88, label="PDR-weighted")
        ax_bar.barh(y - h / 2, sdf["unweighted"], h,
                    color=sdf["color"].values, alpha=0.38, hatch="//",
                    label="Unweighted")
        ax_bar.set_yticks(y)
        ax_bar.set_yticklabels(sdf["label"].values, fontsize=9)
        ax_bar.set_xlabel("Mean E2E Delay (ms)")
        ax_bar.set_title(f"[{rnd_label}] PDR-Weighted vs Unweighted Delay",
                         fontsize=10, fontweight="bold")
        ax_bar.legend(fontsize=8)
        ax_bar.grid(axis="x", alpha=0.3)

    _divider(fig)
    fig.suptitle("Delay Robustness Panel — Round 2 (top) vs Round 3 (bottom)",
                 fontsize=13, fontweight="bold")
    plt.tight_layout()
    _save(fig, "cmp_CL_K_delay_robustness")


# ===========================================================================
# 15.  Policy radar — two-panel polar
# ===========================================================================
def cmp_04_policy_radar_merged(r2_runs, r3_runs):
    AXES_DEF = [
        ("pdr",                  "PDR",           1.0,     True),
        ("charge_success_rate",  "Success rate",  1.0,     True),
        ("decision_latency_mean","1 / Latency",   100_000, False),
        ("energy_per_session",   "Energy recov.", 100.0,   True),
        ("total_deaths",         "1 / Depletions",100,     False),
    ]

    def _norm(val, max_val, hib):
        if np.isnan(val):
            return 0.0
        n = float(val) / max_val
        if not hib:
            n = 1.0 - min(n, 1.0)
        return float(np.clip(n, 0.0, 1.0))

    def _scores(runs):
        df = _kpi_df(runs)
        result = {}
        for proto in _group(runs):
            sub = df[df["protocol"] == proto]
            rep_sc = []
            for _, row in sub.iterrows():
                raw = [row.get(k, np.nan) for k, *_ in AXES_DEF]
                rep_sc.append([_norm(v, AXES_DEF[i][2], AXES_DEF[i][3])
                                for i, v in enumerate(raw)])
            if rep_sc:
                result[proto] = list(np.nanmean(rep_sc, axis=0))
        return result

    scores2, scores3 = _scores(r2_runs), _scores(r3_runs)
    axis_labels = [a[1] for a in AXES_DEF]
    N = len(axis_labels)
    angles = np.linspace(0, 2 * np.pi, N, endpoint=False).tolist() + [0]

    fig, (ax2, ax3) = plt.subplots(1, 2, figsize=(16, 8), subplot_kw={"polar": True})

    for ax, scores, title in [(ax2, scores2, "Round 2"), (ax3, scores3, "Round 3")]:
        for proto, svals in scores.items():
            vals = svals + svals[:1]
            ax.plot(angles, vals, linewidth=1.8,
                    label=PROTO_LABELS.get(proto, proto), color=PROTO_COLORS[proto])
            ax.fill(angles, vals, alpha=0.10, color=PROTO_COLORS[proto])
        ax.set_xticks(angles[:-1])
        ax.set_xticklabels(axis_labels, fontsize=9)
        ax.set_rlim(0, 1)
        ax.set_yticks([0.25, 0.5, 0.75, 1.0])
        ax.set_yticklabels(["0.25", "0.5", "0.75", "1.0"], fontsize=7)
        ax.set_title(title, pad=20, fontsize=12, fontweight="bold")
        ax.legend(loc="upper right", bbox_to_anchor=(1.35, 1.1), fontsize=8)

    fig.suptitle("Policy Radar — Round 2 (left) vs Round 3 (right)",
                 fontsize=13, fontweight="bold")
    fig.text(0.5, 0.01,
             "Normalisation: metric ÷ reference max, clipped to [0, 1].  "
             "1/Latency = 1−lat/100k ms;  1/Depletions = 1−n/100.  Outward = better.",
             ha="center", fontsize=7, color="dimgray")
    plt.tight_layout()
    _save(fig, "cmp_04_policy_radar_merged")


# ===========================================================================
# 16.  Energy recovered — grouped boxplot
# ===========================================================================
def cmp_02_energy_recovered_merged(r2_runs, r3_runs):
    r2d, r3d = _energy_per_proto(r2_runs), _energy_per_proto(r3_runs)
    protos = [p for p in PROTOCOLS if p in r2d or p in r3d]
    n = len(protos)
    w = 0.35
    pos_r2 = [i * 2 + 0.80 for i in range(n)]
    pos_r3 = [i * 2 + 1.45 for i in range(n)]

    fig, ax = plt.subplots(figsize=(max(10, n * 2), 6))
    bp2 = ax.boxplot([r2d.get(p, [0]) for p in protos],
                     positions=pos_r2, widths=w, patch_artist=True,
                     manage_ticks=False,
                     medianprops=dict(color="black", linewidth=1.5))
    bp3 = ax.boxplot([r3d.get(p, [0]) for p in protos],
                     positions=pos_r3, widths=w, patch_artist=True,
                     manage_ticks=False,
                     medianprops=dict(color="black", linewidth=1.5))
    for patch, proto in zip(bp2["boxes"], protos):
        patch.set_facecolor(PROTO_COLORS[proto]); patch.set_alpha(0.85)
    for patch, proto in zip(bp3["boxes"], protos):
        patch.set_facecolor(PROTO_COLORS[proto])
        patch.set_alpha(0.45); patch.set_hatch("///")

    tick_pos = [(a + b) / 2 for a, b in zip(pos_r2, pos_r3)]
    ax.set_xticks(tick_pos)
    ax.set_xticklabels([PROTO_LABELS.get(p, p) for p in protos],
                       rotation=20, ha="right")
    ax.set_ylabel("Energy recovered per session (Wh)")
    ax.set_title(
        "Energy Recovered per Charge Session — Round 2 (solid) vs Round 3 (hatched)",
        fontsize=12, fontweight="bold")
    _proto_legend(ax, protos)
    ax.grid(axis="y", alpha=0.3)
    fig.tight_layout()
    _save(fig, "cmp_02_energy_recovered_merged")


# ===========================================================================
# main
# ===========================================================================
def main():
    OUT_ROOT.mkdir(parents=True, exist_ok=True)
    print("Discovering runs…")
    r2_runs = _discover_r2()
    r3_runs = _discover_r3()
    print(f"  R2: {len(r2_runs)} runs,  R3: {len(r3_runs)} runs")

    tasks = [
        ("cmp_01_mean_pdr_merged",                    cmp_01_mean_pdr_merged),
        ("cmp_01_network_pdr_over_time_merged",        cmp_01_network_pdr_over_time_merged),
        ("cmp_02_charge_success_rate_merged",          cmp_02_charge_success_rate_merged),
        ("cmp_03_charge_outcome_breakdown_merged",     cmp_03_charge_outcome_breakdown_merged),
        ("cmp_CL_G_routing_drop_timeseries",           cmp_CL_G_routing_drop_timeseries),
        ("cmp_03_dead_uav_cumulative_merged",          cmp_03_dead_uav_cumulative_merged),
        ("cmp_CL_F_pdr_vs_depletions_timeseries",      cmp_CL_F_pdr_vs_depletions_timeseries),
        ("cmp_CL_B_charging_vs_pdr_scatter",           cmp_CL_B_charging_vs_pdr_scatter),
        ("cmp_CL_E_correlation_bar_chart",             cmp_CL_E_correlation_bar_chart),
        ("cmp_CL_D_mechanism_chain",                   cmp_CL_D_mechanism_chain),
        ("cmp_06_mean_e2e_delay_merged",               cmp_06_mean_e2e_delay_merged),
        ("cmp_06_pdr_vs_e2e_scatter",                  cmp_06_pdr_vs_e2e_scatter),
        ("cmp_CL_C_e2e_delay_conditioning_analysis",   cmp_CL_C_e2e_delay_conditioning_analysis),
        ("cmp_CL_K_delay_robustness",                  cmp_CL_K_delay_robustness),
        ("cmp_04_policy_radar_merged",                 cmp_04_policy_radar_merged),
        ("cmp_02_energy_recovered_merged",             cmp_02_energy_recovered_merged),
    ]

    print(f"\nGenerating {len(tasks)} comparison figures → {OUT_ROOT}/\n")
    ok, failed = 0, []
    for name, fn in tasks:
        try:
            fn(r2_runs, r3_runs)
            ok += 1
        except Exception as exc:
            print(f"  [WARN] {name}: {exc}")
            failed.append(name)

    print(f"\nDone: {ok}/{len(tasks)} figures saved to {OUT_ROOT}/")
    if failed:
        print("Failed:", ", ".join(failed))


if __name__ == "__main__":
    main()
