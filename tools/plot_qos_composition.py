#!/usr/bin/env python3
"""Plot QoS composition from qos_metrics.csv (heatmap or stacked outcomes).

# requirements.txt snippet:
# pandas>=1.5
# numpy>=1.23
# matplotlib>=3.6
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

from tools.utils_clean import safe_numeric
from tools.utils_io import discover_policy_dirs, load_csv_with_hints, require_columns

FONT_SIZE = 12


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Plot QoS composition by policy/category")
    p.add_argument("--data_root", required=True, type=Path)
    p.add_argument("--out_dir", type=Path, default=Path("analysis/test_round1"))
    p.add_argument("--view", required=True, choices=["heatmap_pdr", "stacked_outcomes"])
    p.add_argument("--category", default=None, help="Required when --view stacked_outcomes")
    return p.parse_args()


def load_qos_rows(policy_dir: Path) -> pd.DataFrame:
    path = policy_dir / "qos_metrics.csv"
    df = load_csv_with_hints(path, dtype_hints={"run_id": "string", "control_type": "string"})
    require_columns(df, ["control_type", "generated", "delivered", "dropped"], path)
    df = safe_numeric(df, ["generated", "delivered", "dropped"])
    df["control_type"] = df["control_type"].astype(str).str.strip()
    return df


def aggregate_policy_qos(df: pd.DataFrame) -> pd.DataFrame:
    out = (
        df.groupby("control_type", as_index=False)[["generated", "delivered", "dropped"]]
        .sum(min_count=1)
    )
    out = out[out["generated"].notna() & out["generated"].gt(0)].copy()
    out["pdr"] = out["delivered"] / out["generated"]
    out["drop_ratio"] = out["dropped"] / out["generated"]
    return out


def build_combined(data_root: Path) -> tuple[list[str], list[str], pd.DataFrame]:
    policies = discover_policy_dirs(data_root)
    policy_names: list[str] = []
    rows: list[pd.DataFrame] = []

    for policy_dir in policies:
        policy_names.append(policy_dir.name)
        pol = aggregate_policy_qos(load_qos_rows(policy_dir))
        pol["policy"] = policy_dir.name
        rows.append(pol)

    merged = pd.concat(rows, ignore_index=True) if rows else pd.DataFrame(columns=["policy", "control_type", "pdr", "drop_ratio"])
    categories = sorted(merged["control_type"].dropna().unique().tolist())
    return policy_names, categories, merged


def render_heatmap_pdr(policy_names: list[str], categories: list[str], merged: pd.DataFrame, out_dir: Path) -> None:
    matrix = np.full((len(categories), len(policy_names)), np.nan, dtype=float)
    for i, cat in enumerate(categories):
        for j, policy in enumerate(policy_names):
            sel = merged[(merged["control_type"] == cat) & (merged["policy"] == policy)]
            if not sel.empty:
                matrix[i, j] = float(sel.iloc[0]["pdr"])

    plt.rcParams.update({"font.size": FONT_SIZE, "axes.labelsize": FONT_SIZE})
    fig, ax = plt.subplots(figsize=(max(9, len(policy_names) * 1.3), max(6, len(categories) * 0.35)))

    masked = np.ma.masked_invalid(matrix)
    im = ax.imshow(masked, aspect="auto", vmin=0.0, vmax=1.0, cmap="viridis")

    ax.set_xticks(np.arange(len(policy_names)))
    ax.set_xticklabels(policy_names, rotation=30, ha="right")
    ax.set_yticks(np.arange(len(categories)))
    ax.set_yticklabels(categories)
    ax.set_xlabel("Policy")
    ax.set_ylabel("Message category (control_type)")
    ax.set_title("QoS composition heatmap: PDR (delivered/generated)")

    cbar = fig.colorbar(im, ax=ax)
    cbar.set_label("PDR (ratio)")

    fig.tight_layout()
    fig_name = "qos_heatmap_pdr"
    target_dir = out_dir / fig_name
    target_dir.mkdir(parents=True, exist_ok=True)
    png = target_dir / f"{fig_name}.png"
    pdf = target_dir / f"{fig_name}.pdf"
    fig.savefig(png, dpi=200)
    fig.savefig(pdf)
    plt.close(fig)
    print(f"Saved: {png}")
    print(f"Saved: {pdf}")


def render_stacked_outcomes(policy_names: list[str], merged: pd.DataFrame, category: str, out_dir: Path) -> None:
    sel = merged[merged["control_type"] == category].copy()

    delivered = []
    dropped = []
    for policy in policy_names:
        row = sel[sel["policy"] == policy]
        if row.empty:
            delivered.append(0.0)
            dropped.append(0.0)
        else:
            delivered.append(float(row.iloc[0]["pdr"]))
            dropped.append(float(row.iloc[0]["drop_ratio"]))

    x = np.arange(len(policy_names))

    plt.rcParams.update({"font.size": FONT_SIZE, "axes.labelsize": FONT_SIZE})
    fig, ax = plt.subplots(figsize=(max(9, len(policy_names) * 1.2), 5))
    ax.bar(x, delivered, label="delivered/generated", color="#1f77b4")
    ax.bar(x, dropped, bottom=delivered, label="dropped/generated", color="#ff7f0e")

    ax.set_xticks(x)
    ax.set_xticklabels(policy_names, rotation=30, ha="right")
    ax.set_xlabel("Policy")
    ax.set_ylabel("Outcome proportion (ratio)")
    ax.set_title(f"QoS outcomes for category: {category}")
    ax.set_ylim(0, max(1.0, np.nanmax(np.array(delivered) + np.array(dropped)) * 1.05 if policy_names else 1.0))
    ax.grid(axis="y", alpha=0.3)
    ax.legend(loc="best")

    fig.tight_layout()
    safe_category = "".join(ch if ch.isalnum() or ch in {"-", "_"} else "_" for ch in category)
    fig_name = f"qos_stacked_{safe_category}"
    target_dir = out_dir / fig_name
    target_dir.mkdir(parents=True, exist_ok=True)
    png = target_dir / f"{fig_name}.png"
    pdf = target_dir / f"{fig_name}.pdf"
    fig.savefig(png, dpi=200)
    fig.savefig(pdf)
    plt.close(fig)
    print(f"Saved: {png}")
    print(f"Saved: {pdf}")


def main() -> int:
    args = parse_args()

    if args.view == "stacked_outcomes" and not args.category:
        print("ERROR: --category is required when --view stacked_outcomes", file=sys.stderr)
        return 1

    try:
        policy_names, categories, merged = build_combined(args.data_root)
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    if args.view == "heatmap_pdr":
        if not categories:
            print("ERROR: no categories found in qos_metrics.csv files", file=sys.stderr)
            return 1
        render_heatmap_pdr(policy_names, categories, merged, args.out_dir)
    else:
        assert args.category is not None
        if args.category not in set(categories):
            print(
                f"ERROR: category '{args.category}' not found. Available: {', '.join(categories)}",
                file=sys.stderr,
            )
            return 1
        render_stacked_outcomes(policy_names, merged, args.category, args.out_dir)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
