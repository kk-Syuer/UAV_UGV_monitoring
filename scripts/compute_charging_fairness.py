#!/usr/bin/env python3
"""Compute Jain's fairness index for charging outcomes across UAVs.

Two fairness metrics:
  1. energy_fairness  — Jain index over energy_recovered per UAV (per run)
  2. charge_fairness  — Jain index over successful charge count per UAV

Outputs per-replicate and per-protocol (merged) fairness summaries plus
a comparative bar/box plot.

Usage
-----
    python scripts/compute_charging_fairness.py \
        --input  experiment_data_collection/test_round2 \
        --output analysis/test_round2/cross_layer

Output
------
    <output>/fairness_summary.csv
    <output>/figures/fairness_energy_barplot.png
    <output>/figures/fairness_charges_barplot.png
    <output>/figures/fairness_boxplot.png
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

sys.path.insert(0, str(Path(__file__).parent))
from plotting_utils import discover_runs, load_csv_if_exists, ensure_t_rel  # noqa: E402

# ---------------------------------------------------------------------------
# Jain's fairness index
# ---------------------------------------------------------------------------

def jains_index(values: np.ndarray) -> float:
    """Return Jain's fairness index in [1/n, 1.0].  Returns NaN if n < 2."""
    v = values[~np.isnan(values)]
    v = v[v >= 0]
    if len(v) < 2:
        return float("nan")
    s  = float(np.sum(v))
    s2 = float(np.sum(v ** 2))
    if s2 == 0:
        return float("nan")
    return (s ** 2) / (len(v) * s2)


# ---------------------------------------------------------------------------
# Per-run fairness computation
# ---------------------------------------------------------------------------

def compute_run_fairness(run: dict) -> dict | None:
    folder   = run["folder"]
    protocol = run["protocol"]
    replicate = run["replicate"]

    path = folder / "charge_events.csv"
    df = load_csv_if_exists(path)
    if df is None:
        print(f"  [WARN] {protocol} r{replicate}: charge_events.csv not found")
        return None

    if "uav_id" not in df.columns:
        print(f"  [WARN] {protocol} r{replicate}: charge_events missing uav_id column")
        return None

    result = {
        "protocol":  protocol,
        "replicate": replicate,
        "n_uavs":    df["uav_id"].nunique(),
        "n_events":  len(df),
    }

    # --- Energy fairness ---
    if "energy_recovered" in df.columns and "outcome" in df.columns:
        started = df[df["outcome"] == "STARTED"].copy()
        if not started.empty:
            per_uav_energy = started.groupby("uav_id")["energy_recovered"].sum()
            result["energy_fairness"] = jains_index(per_uav_energy.values)
            result["energy_recovered_mean"] = float(per_uav_energy.mean())
            result["energy_recovered_std"]  = float(per_uav_energy.std())
        else:
            result["energy_fairness"] = float("nan")
    else:
        if "energy_recovered" not in df.columns:
            print(f"  [WARN] {protocol} r{replicate}: no energy_recovered column")
        result["energy_fairness"] = float("nan")

    # --- Charge-count fairness ---
    if "outcome" in df.columns:
        started = df[df["outcome"] == "STARTED"]
        charge_counts = started.groupby("uav_id").size()
        # Include UAVs that appear in the dataset but had 0 successful charges
        all_uavs = df["uav_id"].unique()
        charge_counts = charge_counts.reindex(all_uavs, fill_value=0)
        result["charge_fairness"] = jains_index(charge_counts.values.astype(float))
        result["charges_per_uav_mean"] = float(charge_counts.mean())
        result["charges_per_uav_std"]  = float(charge_counts.std())
        result["total_started"]        = int((df["outcome"] == "STARTED").sum())
        result["total_timeout"]        = int((df["outcome"] == "TIMEOUT").sum())
        result["total_requests"]       = len(df)
    else:
        print(f"  [WARN] {protocol} r{replicate}: no outcome column")
        result["charge_fairness"] = float("nan")

    return result


# ---------------------------------------------------------------------------
# Plotting
# ---------------------------------------------------------------------------

def _bar_plot(summary_df: pd.DataFrame, metric: str, ylabel: str,
              title: str, out_path: Path):
    """Bar chart: mean ± std per protocol, sorted descending."""
    grp = summary_df.groupby("protocol")[metric].agg(["mean", "std"]).reset_index()
    grp = grp.sort_values("mean", ascending=False)

    cmap = plt.get_cmap("tab10")
    fig, ax = plt.subplots(figsize=(max(6, len(grp) * 1.2), 5))
    xs = np.arange(len(grp))
    colors = [cmap(i % 10) for i in range(len(grp))]
    bars = ax.bar(xs, grp["mean"], yerr=grp["std"].fillna(0),
                  color=colors, capsize=4, width=0.6, error_kw={"linewidth": 1.2})

    for bar, val in zip(bars, grp["mean"]):
        if not np.isnan(val):
            ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.01,
                    f"{val:.3f}", ha="center", va="bottom", fontsize=8)

    ax.set_xticks(xs)
    ax.set_xticklabels(grp["protocol"], rotation=30, ha="right", fontsize=9)
    ax.set_ylabel(ylabel)
    ax.set_ylim(0, 1.15)
    ax.axhline(1.0, color="green", linestyle="--", linewidth=0.8, label="Perfect fairness (1.0)")
    ax.legend(fontsize=8)
    ax.set_title(title)
    fig.tight_layout()
    fig.savefig(out_path, dpi=130, bbox_inches="tight")
    plt.close(fig)


def _box_plot(summary_df: pd.DataFrame, out_path: Path):
    """Side-by-side box plot of both fairness metrics per protocol."""
    protocols = sorted(summary_df["protocol"].unique())
    fig, axes = plt.subplots(1, 2, figsize=(max(10, len(protocols) * 1.5), 5), sharey=True)

    for ax, metric, title in zip(
        axes,
        ["energy_fairness", "charge_fairness"],
        ["Energy Fairness (Jain)", "Charge-Count Fairness (Jain)"],
    ):
        data_by_proto = [
            summary_df.loc[summary_df["protocol"] == p, metric].dropna().values
            for p in protocols
        ]
        data_by_proto = [d if len(d) > 0 else [np.nan] for d in data_by_proto]
        bp = ax.boxplot(data_by_proto, patch_artist=True, notch=False,
                        medianprops={"color": "black", "linewidth": 2})
        cmap = plt.get_cmap("tab10")
        for patch, color_idx in zip(bp["boxes"], range(len(protocols))):
            patch.set_facecolor(cmap(color_idx % 10))
            patch.set_alpha(0.7)
        ax.set_xticklabels(protocols, rotation=35, ha="right", fontsize=8)
        ax.set_ylabel("Jain's fairness index")
        ax.set_ylim(0, 1.15)
        ax.axhline(1.0, color="green", linestyle="--", linewidth=0.8)
        ax.set_title(title)

    fig.suptitle("Charging Fairness (Jain's Index) per Protocol", fontsize=11)
    fig.tight_layout()
    fig.savefig(out_path, dpi=130, bbox_inches="tight")
    plt.close(fig)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description="Compute Jain's charging fairness index.")
    ap.add_argument("--input",  default="experiment_data_collection/test_round2",
                    help="Root directory of experiment data")
    ap.add_argument("--output", default="analysis/test_round2/cross_layer",
                    help="Output directory")
    args = ap.parse_args()

    input_root  = Path(args.input)
    output_root = Path(args.output)
    fig_dir     = output_root / "figures"
    fig_dir.mkdir(parents=True, exist_ok=True)

    runs = discover_runs(input_root)
    if not runs:
        print(f"[ERROR] No runs found under {input_root}")
        sys.exit(1)

    # ------------------------------------------------------------------ #
    # Data coverage report
    # ------------------------------------------------------------------ #
    print("\n=== Data Coverage Report ===")
    protocols_seen: dict[str, list] = {}
    for run in runs:
        protocols_seen.setdefault(run["protocol"], []).append(run["replicate"])
    for proto, reps in sorted(protocols_seen.items()):
        ce_exists = (next(r for r in runs if r["protocol"] == proto)["folder"]
                     / "charge_events.csv").exists()
        print(f"  {proto}  (replicates: {sorted(reps)})  "
              f"charge_events: {'✓' if ce_exists else '✗'}")
    print()

    # ------------------------------------------------------------------ #
    # Compute fairness per run
    # ------------------------------------------------------------------ #
    rows = []
    for run in sorted(runs, key=lambda r: (r["protocol"], r["replicate"])):
        result = compute_run_fairness(run)
        if result is not None:
            rows.append(result)

    if not rows:
        print("[ERROR] No fairness data could be computed.")
        sys.exit(1)

    summary_df = pd.DataFrame(rows)

    # ------------------------------------------------------------------ #
    # Append per-protocol merged rows
    # ------------------------------------------------------------------ #
    merged_rows = []
    for proto, grp in summary_df.groupby("protocol"):
        for metric in ["energy_fairness", "charge_fairness"]:
            vals = grp[metric].dropna()
            if len(vals) == 0:
                continue
        merged_row = {"protocol": proto, "replicate": "merged"}
        for col in summary_df.select_dtypes(include=np.number).columns:
            if col == "replicate":
                continue
            merged_row[f"{col}_mean"] = float(grp[col].mean())
            merged_row[f"{col}_std"]  = float(grp[col].std())
        merged_rows.append(merged_row)

    # Save per-replicate summary
    out_csv = output_root / "fairness_summary.csv"
    summary_df.to_csv(out_csv, index=False)
    print(f"Per-replicate fairness  → {out_csv}")

    # Save merged summary
    if merged_rows:
        merged_df = pd.DataFrame(merged_rows)
        merged_csv = output_root / "fairness_summary_merged.csv"
        merged_df.to_csv(merged_csv, index=False)
        print(f"Merged fairness         → {merged_csv}")

    # ------------------------------------------------------------------ #
    # Plots
    # ------------------------------------------------------------------ #
    if "energy_fairness" in summary_df.columns:
        _bar_plot(
            summary_df,
            metric="energy_fairness",
            ylabel="Jain's fairness index (energy recovered)",
            title="Charging Energy Fairness per Protocol",
            out_path=fig_dir / "fairness_energy_barplot.png",
        )
        print(f"  [OK] fairness_energy_barplot.png")

    if "charge_fairness" in summary_df.columns:
        _bar_plot(
            summary_df,
            metric="charge_fairness",
            ylabel="Jain's fairness index (successful charges)",
            title="Charge-Count Fairness per Protocol",
            out_path=fig_dir / "fairness_charges_barplot.png",
        )
        print(f"  [OK] fairness_charges_barplot.png")

    if "energy_fairness" in summary_df.columns and "charge_fairness" in summary_df.columns:
        _box_plot(summary_df, fig_dir / "fairness_boxplot.png")
        print(f"  [OK] fairness_boxplot.png")

    # ------------------------------------------------------------------ #
    # Print summary table to stdout
    # ------------------------------------------------------------------ #
    print("\n=== Fairness Summary ===")
    display_cols = [c for c in ["protocol", "replicate", "n_uavs",
                                "energy_fairness", "charge_fairness",
                                "total_started", "total_requests"]
                    if c in summary_df.columns]
    with pd.option_context("display.float_format", "{:.4f}".format,
                           "display.max_columns", 20,
                           "display.width", 120):
        print(summary_df[display_cols].to_string(index=False))

    print(f"\nDone. Outputs written to: {output_root}")


if __name__ == "__main__":
    main()
