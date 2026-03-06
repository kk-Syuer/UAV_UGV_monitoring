#!/usr/bin/env python3
"""Event-overlay confirmation plots for cross-layer analysis.

For each protocol's merged window table, plots:
  1. DPR over time with timeout + death count markers and optional DPR-dip shading
  2. E2E delay over time with the same markers

Usage
-----
    python scripts/plot_cross_layer_overlays.py \
        --input  analysis/test_round2/cross_layer \
        --output analysis/test_round2/cross_layer \
        [--dpr-threshold 0.2]

Output
------
    <output>/figures/overlay_dpr_<protocol>.png
    <output>/figures/overlay_delay_<protocol>.png
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

warnings.filterwarnings("ignore", category=FutureWarning)
warnings.filterwarnings("ignore", category=RuntimeWarning)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _load_merged(tables_dir: Path, protocol: str) -> pd.DataFrame | None:
    path = tables_dir / f"{protocol}_merged.csv"
    if not path.exists():
        print(f"  [WARN] merged table not found: {path}")
        return None
    df = pd.read_csv(path).sort_values("bin_start_s").reset_index(drop=True)
    return df


def _discover_protocols(tables_dir: Path) -> list[str]:
    return sorted({
        p.stem.replace("_merged", "")
        for p in tables_dir.glob("*_merged.csv")
    })


def _add_event_bars(ax_twin, t_min: np.ndarray,
                    timeout_counts: np.ndarray,
                    death_counts:   np.ndarray,
                    bin_width_min:  float):
    """Add a secondary y-axis with stacked bars for timeout and death counts."""
    width = bin_width_min * 0.8
    bottom = np.zeros(len(t_min))

    if np.nansum(timeout_counts) > 0:
        ax_twin.bar(t_min, timeout_counts, width=width, bottom=bottom,
                    color="purple", alpha=0.35, label="Timeouts (count)",
                    align="edge")
        bottom = bottom + timeout_counts

    if np.nansum(death_counts) > 0:
        ax_twin.bar(t_min, death_counts, width=width, bottom=bottom,
                    color="crimson", alpha=0.40, label="Deaths (count)",
                    align="edge")

    ax_twin.set_ylabel("Event count (per bin)", color="gray", fontsize=8)
    ax_twin.tick_params(axis="y", labelcolor="gray", labelsize=7)
    ax_twin.set_ylim(bottom=0)


def _shade_dip_regions(ax, t_min: np.ndarray, values: np.ndarray, threshold: float):
    """Shade contiguous regions where values < threshold."""
    in_dip = False
    dip_start = None
    for i, v in enumerate(values):
        below = (not np.isnan(v)) and (v < threshold)
        if below and not in_dip:
            dip_start = t_min[i]
            in_dip = True
        elif not below and in_dip:
            ax.axvspan(dip_start, t_min[i], color="orange", alpha=0.15, label="_nolegend_")
            in_dip = False
    if in_dip and dip_start is not None:
        ax.axvspan(dip_start, t_min[-1], color="orange", alpha=0.15, label="_nolegend_")


# ---------------------------------------------------------------------------
# Single-protocol overlay plots
# ---------------------------------------------------------------------------

def plot_protocol_overlays(proto: str, df: pd.DataFrame,
                           fig_dir: Path, dpr_threshold: float):
    t_min = df["bin_start_min"].values
    bin_width_min = float(t_min[1] - t_min[0]) if len(t_min) > 1 else 1.0

    timeout_counts = df["timeout_count"].fillna(0).values  if "timeout_count" in df.columns else np.zeros(len(df))
    death_counts   = df["death_count"].fillna(0).values    if "death_count"   in df.columns else np.zeros(len(df))

    # ------------------------------------------------------------------ #
    # Plot 1: DPR overlay
    # ------------------------------------------------------------------ #
    if "dpr_mean" in df.columns:
        fig, ax = plt.subplots(figsize=(12, 5))
        ax_twin = ax.twinx()

        dpr = df["dpr_mean"].values
        ax.plot(t_min, dpr, color="steelblue", linewidth=1.8, label="DPR (mean)", zorder=3)
        ax.axhline(0.95, color="red",    linestyle="--", linewidth=1.0, label="PDR target 0.95")
        ax.axhline(dpr_threshold, color="orange", linestyle=":",
                   linewidth=1.0, label=f"DPR threshold {dpr_threshold}")

        _shade_dip_regions(ax, t_min, dpr, dpr_threshold)
        _add_event_bars(ax_twin, t_min, timeout_counts, death_counts, bin_width_min)

        ax.set_xlabel("Experiment time (min)")
        ax.set_ylabel("DPR (mean per bin)")
        ax.set_ylim(-0.05, 1.08)
        ax.set_title(f"{proto}  —  DPR over time with event markers")

        # Merge legends from both axes
        handles1, labels1 = ax.get_legend_handles_labels()
        handles2, labels2 = ax_twin.get_legend_handles_labels()
        ax.legend(handles1 + handles2, labels1 + labels2,
                  loc="lower left", fontsize=8, framealpha=0.7)

        fig.tight_layout()
        fig.savefig(fig_dir / f"overlay_dpr_{proto}.png", dpi=130, bbox_inches="tight")
        plt.close(fig)
        print(f"  [OK] overlay_dpr_{proto}.png")
    else:
        print(f"  [SKIP] {proto}: no dpr_mean column")

    # ------------------------------------------------------------------ #
    # Plot 2: E2E Delay overlay
    # ------------------------------------------------------------------ #
    if "e2e_delay_mean_ms" in df.columns:
        fig, ax = plt.subplots(figsize=(12, 5))
        ax_twin = ax.twinx()

        delay = df["e2e_delay_mean_ms"].values
        ax.plot(t_min, delay, color="darkorange", linewidth=1.8,
                label="E2E Delay mean (ms)", zorder=3)
        ax.axhline(200, color="red", linestyle="--", linewidth=1.0,
                   label="Delay target 200 ms")

        _add_event_bars(ax_twin, t_min, timeout_counts, death_counts, bin_width_min)

        ax.set_xlabel("Experiment time (min)")
        ax.set_ylabel("E2E Delay (ms)")
        ax.set_title(f"{proto}  —  E2E Delay over time with event markers")

        handles1, labels1 = ax.get_legend_handles_labels()
        handles2, labels2 = ax_twin.get_legend_handles_labels()
        ax.legend(handles1 + handles2, labels1 + labels2,
                  loc="upper left", fontsize=8, framealpha=0.7)

        fig.tight_layout()
        fig.savefig(fig_dir / f"overlay_delay_{proto}.png", dpi=130, bbox_inches="tight")
        plt.close(fig)
        print(f"  [OK] overlay_delay_{proto}.png")
    else:
        print(f"  [SKIP] {proto}: no e2e_delay_mean_ms column")


# ---------------------------------------------------------------------------
# Combined multi-protocol summary (DPR only)
# ---------------------------------------------------------------------------

def plot_combined_dpr(protos_data: dict[str, pd.DataFrame], fig_dir: Path):
    """All protocols on a single DPR axes (no event bars, for quick comparison)."""
    cmap = plt.get_cmap("tab10")
    fig, ax = plt.subplots(figsize=(13, 5))
    any_data = False
    for i, (proto, df) in enumerate(protos_data.items()):
        if "dpr_mean" not in df.columns:
            continue
        t = df["bin_start_min"].values
        d = df["dpr_mean"].values
        ax.plot(t, d, color=cmap(i % 10), linewidth=1.6, label=proto, alpha=0.85)
        any_data = True

    if not any_data:
        plt.close(fig)
        return

    ax.axhline(0.95, color="red", linestyle="--", linewidth=1.0, label="PDR target 0.95")
    ax.set_xlabel("Experiment time (min)")
    ax.set_ylabel("DPR (mean per bin)")
    ax.set_ylim(-0.05, 1.08)
    ax.set_title("DPR Comparison — All Protocols (merged replicates)")
    ax.legend(fontsize=8, loc="lower left", framealpha=0.75)
    fig.tight_layout()
    fig.savefig(fig_dir / "overlay_dpr_all_protocols.png", dpi=130, bbox_inches="tight")
    plt.close(fig)
    print("  [OK] overlay_dpr_all_protocols.png")


def plot_combined_delay(protos_data: dict[str, pd.DataFrame], fig_dir: Path):
    cmap = plt.get_cmap("tab10")
    fig, ax = plt.subplots(figsize=(13, 5))
    any_data = False
    for i, (proto, df) in enumerate(protos_data.items()):
        if "e2e_delay_mean_ms" not in df.columns:
            continue
        t = df["bin_start_min"].values
        d = df["e2e_delay_mean_ms"].values
        ax.plot(t, d, color=cmap(i % 10), linewidth=1.6, label=proto, alpha=0.85)
        any_data = True

    if not any_data:
        plt.close(fig)
        return

    ax.axhline(200, color="red", linestyle="--", linewidth=1.0, label="Delay target 200 ms")
    ax.set_xlabel("Experiment time (min)")
    ax.set_ylabel("E2E Delay (ms)")
    ax.set_title("E2E Delay Comparison — All Protocols (merged replicates)")
    ax.legend(fontsize=8, loc="upper left", framealpha=0.75)
    fig.tight_layout()
    fig.savefig(fig_dir / "overlay_delay_all_protocols.png", dpi=130, bbox_inches="tight")
    plt.close(fig)
    print("  [OK] overlay_delay_all_protocols.png")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description="Plot cross-layer event overlay figures.")
    ap.add_argument("--input",          default="analysis/test_round2/cross_layer",
                    help="Directory containing tables/ from build_cross_layer_windows.py")
    ap.add_argument("--output",         default="analysis/test_round2/cross_layer",
                    help="Output directory (figures/ subfolder created here)")
    ap.add_argument("--dpr-threshold",  type=float, default=0.2,
                    help="DPR level below which regions are shaded (default: 0.2)")
    args = ap.parse_args()

    tables_dir = Path(args.input) / "tables"
    fig_dir    = Path(args.output) / "figures"
    fig_dir.mkdir(parents=True, exist_ok=True)

    protocols = _discover_protocols(tables_dir)
    if not protocols:
        print(f"[ERROR] No merged tables found under {tables_dir}")
        sys.exit(1)

    print(f"Protocols: {protocols}\n")

    protos_data: dict[str, pd.DataFrame] = {}
    for proto in protocols:
        print(f"[plot] {proto}")
        df = _load_merged(tables_dir, proto)
        if df is None or df.empty:
            continue
        protos_data[proto] = df
        plot_protocol_overlays(proto, df, fig_dir, args.dpr_threshold)

    # Combined comparison plots
    print("\n[combined plots]")
    plot_combined_dpr(protos_data, fig_dir)
    plot_combined_delay(protos_data, fig_dir)

    print(f"\nDone. Figures written to: {fig_dir}")


if __name__ == "__main__":
    main()
