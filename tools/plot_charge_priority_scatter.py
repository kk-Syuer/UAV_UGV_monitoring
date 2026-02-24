#!/usr/bin/env python3
"""Plot priority-score vs waiting-time bubble scatter by policy.

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

from tools.utils_clean import normalize_role, safe_numeric
from tools.utils_io import discover_policy_dirs, load_csv_with_hints

FONT_SIZE = 12


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Plot priority-vs-wait bubble scatter from charge_events.csv")
    p.add_argument("--data_root", required=True, type=Path)
    p.add_argument("--out_dir", type=Path, default=Path("analysis/test_round1"))
    p.add_argument("--policy_filter", default=None, help="Comma-separated policy names to include")
    return p.parse_args()


def _pick_first(df: pd.DataFrame, candidates: list[str]) -> str | None:
    for col in candidates:
        if col in df.columns:
            return col
    return None


def _prepare_policy_df(policy_dir: Path) -> tuple[pd.DataFrame, str, str, str]:
    path = policy_dir / "charge_events.csv"
    df = load_csv_with_hints(path)

    priority_col = _pick_first(df, ["priority_score", "decision_score", "decision_priority"])
    wait_col = _pick_first(df, ["waiting_time_ms", "effective_wait_ms"])
    queue_col = _pick_first(df, ["queue_size_at_decision", "decision_queue_size"])

    if not priority_col or not wait_col or not queue_col:
        missing = []
        if not priority_col:
            missing.append("priority_score/decision_score/decision_priority")
        if not wait_col:
            missing.append("waiting_time_ms/effective_wait_ms")
        if not queue_col:
            missing.append("queue_size_at_decision/decision_queue_size")
        raise ValueError(f"missing required columns: {', '.join(missing)}")

    df = safe_numeric(df, [priority_col, wait_col, queue_col])

    # Column-aware missing handling for waiting times only.
    zero_wait = df[wait_col].eq(0).fillna(False)
    if zero_wait.any():
        df.loc[zero_wait, wait_col] = np.nan

    neg_wait = df[wait_col].lt(0).fillna(False)
    neg_count = int(neg_wait.sum())
    if neg_count:
        print(f"WARNING [{policy_dir.name}]: removed {neg_count} negative wait rows", file=sys.stderr)
        df = df.loc[~neg_wait]

    df = df.dropna(subset=[priority_col, wait_col, queue_col]).copy()
    df = df[df[queue_col] >= 0]

    if "role" in df.columns:
        df["role_label"] = normalize_role(df["role"])
    else:
        df["role_label"] = "all"

    return df, priority_col, wait_col, queue_col


def _plot_one(policy: str, df: pd.DataFrame, priority_col: str, wait_col: str, queue_col: str, out_dir: Path) -> None:
    plt.rcParams.update({"font.size": FONT_SIZE, "axes.labelsize": FONT_SIZE, "legend.fontsize": FONT_SIZE - 1})
    fig, ax = plt.subplots(figsize=(8, 5))

    roles = sorted(df["role_label"].astype(str).unique().tolist())
    color_map = {"CH": "#d62728", "member": "#1f77b4", "all": "#2ca02c", "unknown": "#9467bd"}

    for role in roles:
        part = df[df["role_label"] == role]
        if part.empty:
            continue
        sizes = 30 + 40 * part[queue_col].to_numpy(dtype=float)
        ax.scatter(
            part[priority_col],
            part[wait_col],
            s=sizes,
            alpha=0.45,
            c=color_map.get(role, "#7f7f7f"),
            label=role,
            edgecolors="none",
        )

    ax.set_xlabel("Priority score (arb. units)")
    ax.set_ylabel("Waiting time (ms)")
    ax.set_title(f"Priority vs wait bubble scatter: {policy}")
    ax.grid(alpha=0.3)
    ax.legend(loc="best")
    fig.tight_layout()

    fig_name = f"priority_vs_wait_bubble_{policy}"
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
    try:
        policies = discover_policy_dirs(args.data_root)
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    wanted = None
    if args.policy_filter:
        wanted = {x.strip() for x in args.policy_filter.split(",") if x.strip()}

    plotted = 0
    for policy_dir in policies:
        if wanted is not None and policy_dir.name not in wanted:
            continue
        try:
            df, priority_col, wait_col, queue_col = _prepare_policy_df(policy_dir)
        except Exception as exc:
            print(f"WARNING [{policy_dir.name}]: skipping policy ({exc})", file=sys.stderr)
            continue

        if df.empty:
            print(f"WARNING [{policy_dir.name}]: no valid rows after cleaning, skipping", file=sys.stderr)
            continue

        _plot_one(policy_dir.name, df, priority_col, wait_col, queue_col, args.out_dir)
        plotted += 1

    if plotted == 0:
        print("ERROR: no policies plotted (check --policy_filter and required columns)", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
