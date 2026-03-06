#!/usr/bin/env python3
"""Cross-layer lagged correlation analysis.

Reads merged window tables produced by build_cross_layer_windows.py and
computes Pearson + Spearman correlations (at lag 0) and lagged correlation
curves between network metrics (DPR, E2E delay) and scheduling metrics
(timeout_rate, success_rate, dock_util, queue_len, mean_wait,
decision_latency, cum_deaths).

Usage
-----
    python scripts/cross_layer_lagged_correlation.py \
        --input  analysis/test_round2/cross_layer \
        --output analysis/test_round2/cross_layer \
        [--max-lag-min 10]

Output
------
    <output>/correlation/lag_corr_<protocol>_<net>_vs_<sched>.csv
    <output>/correlation/lag_corr_<protocol>_<net>_vs_<sched>.png
    <output>/correlation/summary_best_lag.csv
    <output>/correlation/heatmap_pearson_lag0.png
    <output>/correlation/heatmap_spearman_lag0.png
"""

import argparse
import sys
import warnings
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

# Silence pandas future warnings
warnings.filterwarnings("ignore", category=FutureWarning)
warnings.filterwarnings("ignore", category=RuntimeWarning)


# ---------------------------------------------------------------------------
# Pure-numpy correlation helpers (no scipy required)
# ---------------------------------------------------------------------------

def _pearson_r(x: np.ndarray, y: np.ndarray) -> float:
    """Pearson correlation coefficient via numpy."""
    if len(x) < 3 or x.std() == 0 or y.std() == 0:
        return np.nan
    return float(np.corrcoef(x, y)[0, 1])


def _spearman_r(x: np.ndarray, y: np.ndarray) -> float:
    """Spearman rank correlation via numpy (rank ties broken by first occurrence)."""
    if len(x) < 3:
        return np.nan
    xr = x.argsort().argsort().astype(float)
    yr = y.argsort().argsort().astype(float)
    if xr.std() == 0 or yr.std() == 0:
        return np.nan
    return float(np.corrcoef(xr, yr)[0, 1])

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

NETWORK_METRICS = ["dpr_mean", "e2e_delay_mean_ms"]

SCHED_METRICS = [
    "timeout_rate",
    "success_rate",
    "dock_util_mean",
    "queue_len_mean",
    "mean_wait_ms",
    "decision_latency_mean_ms",
    "cum_deaths",
]

METRIC_LABELS = {
    "dpr_mean":                   "DPR",
    "e2e_delay_mean_ms":          "E2E Delay (ms)",
    "timeout_rate":               "Timeout Rate",
    "success_rate":               "Success Rate",
    "dock_util_mean":             "Dock Utilisation",
    "queue_len_mean":             "Queue Length",
    "mean_wait_ms":               "Mean Wait (ms)",
    "decision_latency_mean_ms":   "Decision Latency (ms)",
    "cum_deaths":                 "Cumulative Deaths",
}

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _safe_corr(x: np.ndarray, y: np.ndarray):
    """Return (pearson_r, nan, spearman_r, nan) — p-values omitted (no scipy)."""
    mask = ~(np.isnan(x) | np.isnan(y))
    if mask.sum() < 5:
        return np.nan, np.nan, np.nan, np.nan
    xm, ym = x[mask], y[mask]
    pr = _pearson_r(xm, ym)
    sr = _spearman_r(xm, ym)
    return pr, np.nan, sr, np.nan


def _lagged_corr(x: np.ndarray, y: np.ndarray, max_lag_bins: int):
    """Return (lags, pearson_r_arr, spearman_r_arr).

    Positive lag k means y is shifted forward by k bins (x leads y).
    """
    lags = np.arange(-max_lag_bins, max_lag_bins + 1)
    prs, srs = [], []
    n = len(x)
    for lag in lags:
        if lag >= 0:
            xi, yi = x[:n - lag] if lag > 0 else x, y[lag:] if lag > 0 else y
        else:
            k = -lag
            xi, yi = x[k:], y[:n - k]
        pr, _, sr, _ = _safe_corr(xi, yi)
        prs.append(pr)
        srs.append(sr)
    return lags, np.array(prs), np.array(srs)


def _load_merged(tables_dir: Path, protocol: str) -> pd.DataFrame | None:
    path = tables_dir / f"{protocol}_merged.csv"
    if not path.exists():
        print(f"  [WARN] merged table not found: {path}")
        return None
    df = pd.read_csv(path)
    df = df.sort_values("bin_start_s").reset_index(drop=True)
    return df


def _discover_protocols(tables_dir: Path) -> list[str]:
    return sorted({
        p.stem.replace("_merged", "")
        for p in tables_dir.glob("*_merged.csv")
    })


# ---------------------------------------------------------------------------
# Per-protocol analysis
# ---------------------------------------------------------------------------

def analyse_protocol(proto: str, df: pd.DataFrame,
                     max_lag_bins: int, corr_dir: Path,
                     fig_dir: Path) -> list[dict]:
    """Compute lag correlations for all network×sched pairs.  Return summary rows."""
    summary_rows = []

    net_cols   = [c for c in NETWORK_METRICS if c in df.columns]
    sched_cols = [c for c in SCHED_METRICS   if c in df.columns]

    if not net_cols or not sched_cols:
        print(f"  [WARN] {proto}: insufficient columns for correlation")
        return summary_rows

    for net_col in net_cols:
        for sched_col in sched_cols:
            x = df[net_col].values.astype(float)
            y = df[sched_col].values.astype(float)

            lags, prs, srs = _lagged_corr(x, y, max_lag_bins)

            # Save lag curve CSV
            lag_df = pd.DataFrame({
                "lag_bins": lags,
                "pearson_r":  prs,
                "spearman_r": srs,
            })
            csv_name = f"lag_corr_{proto}_{net_col}_vs_{sched_col}.csv"
            lag_df.to_csv(corr_dir / csv_name, index=False)

            # Plot lag curve
            fig, ax = plt.subplots(figsize=(8, 4))
            ax.plot(lags, prs, label="Pearson r",  color="steelblue", linewidth=1.8)
            ax.plot(lags, srs, label="Spearman ρ", color="darkorange", linewidth=1.8,
                    linestyle="--")
            ax.axvline(0,  color="black",  linewidth=0.8, linestyle=":")
            ax.axhline(0,  color="gray",   linewidth=0.6, linestyle=":")
            ax.axhline(0.5,  color="green", linewidth=0.6, linestyle=":")
            ax.axhline(-0.5, color="red",   linewidth=0.6, linestyle=":")
            ax.set_xlabel("Lag (bins)")
            ax.set_ylabel("Correlation coefficient")
            net_lbl   = METRIC_LABELS.get(net_col,   net_col)
            sched_lbl = METRIC_LABELS.get(sched_col, sched_col)
            ax.set_title(f"{proto}  |  {net_lbl} vs {sched_lbl}")
            ax.set_ylim(-1.05, 1.05)
            ax.legend(fontsize=9)
            fig.tight_layout()
            png_name = f"lag_corr_{proto}_{net_col}_vs_{sched_col}.png"
            fig.savefig(fig_dir / png_name, dpi=120, bbox_inches="tight")
            plt.close(fig)

            # Best-lag summary (by |Pearson r|)
            valid_mask = ~np.isnan(prs)
            if valid_mask.any():
                best_idx = int(np.nanargmax(np.abs(prs)))
                best_lag = int(lags[best_idx])
                best_pr  = float(prs[best_idx])
                best_sr  = float(srs[best_idx]) if not np.isnan(srs[best_idx]) else np.nan
            else:
                best_lag, best_pr, best_sr = 0, np.nan, np.nan

            # Lag-0 values
            lag0_idx = int(np.where(lags == 0)[0][0])
            pr0 = float(prs[lag0_idx]) if not np.isnan(prs[lag0_idx]) else np.nan
            sr0 = float(srs[lag0_idx]) if not np.isnan(srs[lag0_idx]) else np.nan

            summary_rows.append({
                "protocol":        proto,
                "network_metric":  net_col,
                "sched_metric":    sched_col,
                "pearson_r_lag0":  pr0,
                "spearman_r_lag0": sr0,
                "best_lag_bins":   best_lag,
                "best_pearson_r":  best_pr,
                "best_spearman_r": best_sr,
            })

    return summary_rows


# ---------------------------------------------------------------------------
# Heatmap helpers
# ---------------------------------------------------------------------------

def _heatmap(matrix: pd.DataFrame, title: str, out_path: Path):
    fig, ax = plt.subplots(figsize=(max(6, len(matrix.columns) * 1.1),
                                    max(4, len(matrix.index) * 0.6)))
    vmax = min(1.0, max(0.01, np.nanmax(np.abs(matrix.values))))
    im = ax.imshow(matrix.values.astype(float), aspect="auto",
                   cmap="RdBu_r", vmin=-vmax, vmax=vmax)
    plt.colorbar(im, ax=ax, fraction=0.03, pad=0.02)
    ax.set_xticks(range(len(matrix.columns)))
    ax.set_xticklabels([METRIC_LABELS.get(c, c) for c in matrix.columns],
                        rotation=40, ha="right", fontsize=8)
    ax.set_yticks(range(len(matrix.index)))
    ax.set_yticklabels([METRIC_LABELS.get(r, r) for r in matrix.index], fontsize=8)
    for i in range(len(matrix.index)):
        for j in range(len(matrix.columns)):
            v = matrix.values[i, j]
            if not np.isnan(v):
                ax.text(j, i, f"{v:.2f}", ha="center", va="center", fontsize=7,
                        color="white" if abs(v) > 0.6 else "black")
    ax.set_title(title, fontsize=10)
    fig.tight_layout()
    fig.savefig(out_path, dpi=130, bbox_inches="tight")
    plt.close(fig)


def build_heatmaps(summary_df: pd.DataFrame, corr_dir: Path, corr_type: str):
    """One heatmap per protocol: rows=net, cols=sched."""
    col_name = f"{corr_type}_r_lag0"
    if col_name not in summary_df.columns:
        return
    for proto, grp in summary_df.groupby("protocol"):
        pivot = grp.pivot(index="network_metric", columns="sched_metric",
                          values=col_name)
        out_path = corr_dir / f"heatmap_{corr_type}_{proto}.png"
        title = f"{proto}  —  {corr_type.capitalize()} correlation (lag 0)"
        _heatmap(pivot, title, out_path)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description="Cross-layer lagged correlation analysis.")
    ap.add_argument("--input",        default="analysis/test_round2/cross_layer",
                    help="Directory containing tables/ from build_cross_layer_windows.py")
    ap.add_argument("--output",       default="analysis/test_round2/cross_layer",
                    help="Output directory (correlation/ subfolder created here)")
    ap.add_argument("--max-lag-min",  type=float, default=10.0,
                    help="Max lag in minutes for lagged correlation (default: 10)")
    ap.add_argument("--bin-sec",      type=float, default=60.0,
                    help="Bin size used when building windows (needed to convert lag to bins)")
    args = ap.parse_args()

    tables_dir = Path(args.input) / "tables"
    corr_dir   = Path(args.output) / "correlation"
    fig_dir    = corr_dir / "figures"
    corr_dir.mkdir(parents=True, exist_ok=True)
    fig_dir.mkdir(parents=True, exist_ok=True)

    max_lag_bins = max(1, int(args.max_lag_min * 60 / args.bin_sec))

    protocols = _discover_protocols(tables_dir)
    if not protocols:
        print(f"[ERROR] No merged tables found under {tables_dir}")
        sys.exit(1)

    print(f"Protocols found: {protocols}")
    print(f"Max lag: {args.max_lag_min} min  →  {max_lag_bins} bins\n")

    all_summary = []
    for proto in protocols:
        print(f"[analyse] {proto}")
        df = _load_merged(tables_dir, proto)
        if df is None:
            continue
        rows = analyse_protocol(proto, df, max_lag_bins, corr_dir, fig_dir)
        all_summary.extend(rows)
        print(f"  computed {len(rows)} correlation pairs")

    if not all_summary:
        print("[WARN] No correlation results produced.")
        return

    summary_df = pd.DataFrame(all_summary)
    summary_path = corr_dir / "summary_best_lag.csv"
    summary_df.to_csv(summary_path, index=False)
    print(f"\nSummary  → {summary_path}")

    # Heatmaps
    build_heatmaps(summary_df, corr_dir, "pearson")
    build_heatmaps(summary_df, corr_dir, "spearman")
    print(f"Heatmaps → {corr_dir}")

    print("\nDone.")


if __name__ == "__main__":
    main()
