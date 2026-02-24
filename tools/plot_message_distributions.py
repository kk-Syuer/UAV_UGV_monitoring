#!/usr/bin/env python3
"""Plot message-level metric distributions by policy (box plot).

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
FLOW_CHOICES = ["HEARTBEAT", "CHARGE_REQUEST", "CHARGE_DECISION", "CHARGE_STATUS", "ALL"]


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Plot message metric distributions by policy")
    p.add_argument("--data_root", required=True, type=Path)
    p.add_argument("--out_dir", type=Path, default=Path("analysis/test_round1"))
    p.add_argument("--metric", required=True, choices=["e2e_delay", "hop_count"])
    p.add_argument("--flow_type", default="ALL", choices=FLOW_CHOICES)
    return p.parse_args()


def _parse_bool(series: pd.Series) -> pd.Series:
    return (
        series.astype(str)
        .str.strip()
        .str.lower()
        .isin({"1", "true", "yes", "y", "t"})
    )


def _control_type_filter(df: pd.DataFrame, flow_type: str) -> pd.Series:
    if flow_type == "ALL":
        return pd.Series(True, index=df.index)

    ct = df.get("control_type", pd.Series("", index=df.index)).astype(str).str.upper()
    ft = pd.to_numeric(df.get("flow_type", pd.Series(np.nan, index=df.index)), errors="coerce")

    if flow_type == "HEARTBEAT":
        return ct.eq("HEARTBEAT")
    if flow_type == "CHARGE_REQUEST":
        return ct.eq("CHARGE_REQUEST")
    if flow_type == "CHARGE_DECISION":
        return ct.eq("CHARGE_DECISION")
    # CHARGE_STATUS in these logs is usually control-plane status/ack style traffic.
    if flow_type == "CHARGE_STATUS":
        return ct.isin({"CHARGE_STATUS", "STATUS_CH", "STATUS_MEMBER", "ACK", "CHARGE_ACK"}) | ft.eq(1)

    return pd.Series(True, index=df.index)


def dedupe_messages(df: pd.DataFrame) -> pd.DataFrame:
    # Keep earliest delivered if any delivered exists per (run_id, msg_id), else keep last record.
    rows: list[pd.DataFrame] = []
    sort_cols = [c for c in ["run_id", "msg_id", "delivered_time_s", "creation_time_s"] if c in df.columns]
    ordered = df.sort_values(sort_cols, na_position="last")

    for _, g in ordered.groupby(["run_id", "msg_id"], dropna=False, sort=False):
        delivered_group = g[g["_delivered"]]
        if not delivered_group.empty:
            rows.append(delivered_group.iloc[[0]])
        else:
            rows.append(g.iloc[[-1]])

    if not rows:
        return ordered.iloc[0:0].copy()
    return pd.concat(rows, ignore_index=True)


def load_metric_for_policy(policy_dir: Path, metric: str, flow_type: str) -> tuple[pd.Series, int]:
    csv_path = policy_dir / "messages.csv"
    df = load_csv_with_hints(csv_path, dtype_hints={"run_id": "string", "msg_id": "string"})
    require_columns(df, ["run_id", "msg_id", "delivered"], csv_path)

    if metric == "e2e_delay":
        require_columns(df, ["e2e_delay_ms"], csv_path)
    else:
        require_columns(df, ["hop_count"], csv_path)

    df = safe_numeric(df, ["e2e_delay_ms", "hop_count", "creation_time_s", "delivered_time_s"])
    df["_delivered"] = _parse_bool(df["delivered"])

    df = df.loc[_control_type_filter(df, flow_type)].copy()
    deduped = dedupe_messages(df)

    removed_negative_delay = 0
    if metric == "e2e_delay":
        neg = deduped["e2e_delay_ms"].lt(0).fillna(False)
        removed_negative_delay = int(neg.sum())
        deduped = deduped.loc[~neg]
        values = deduped["e2e_delay_ms"].dropna()
    else:
        values = deduped["hop_count"].dropna()

    return values, removed_negative_delay


def render_plot(data: list[pd.Series], labels: list[str], metric: str, out_dir: Path, kind: str) -> None:
    plt.rcParams.update({"font.size": FONT_SIZE, "axes.labelsize": FONT_SIZE, "legend.fontsize": FONT_SIZE - 1})
    fig, ax = plt.subplots(figsize=(10, 5))

    if kind == "box":
        ax.boxplot(data, labels=labels, showfliers=True)
        fig_name = f"message_{metric}_distribution_box"
    else:
        v = ax.violinplot(data, showmeans=True, showmedians=False, showextrema=True)
        for body in v["bodies"]:
            body.set_alpha(0.4)
            body.set_facecolor("#1f77b4")
        ax.set_xticks(np.arange(1, len(labels) + 1))
        ax.set_xticklabels(labels)
        fig_name = f"message_{metric}_distribution_violin"

    ylab = "End-to-end delay (ms)" if metric == "e2e_delay" else "Hop count (hops)"
    ax.set_xlabel("Policy")
    ax.set_ylabel(ylab)
    ax.set_title(f"Message {metric} distribution by policy")
    ax.grid(axis="y", alpha=0.3)
    fig.tight_layout()

    target_dir = out_dir / fig_name
    target_dir.mkdir(parents=True, exist_ok=True)
    png = target_dir / f"{fig_name}.png"
    pdf = target_dir / f"{fig_name}.pdf"
    fig.savefig(png, dpi=200)
    fig.savefig(pdf)
    plt.close(fig)
    print(f"Saved: {png}")
    print(f"Saved: {pdf}")


def run(kind: str = "box") -> int:
    args = parse_args()
    try:
        policies = discover_policy_dirs(args.data_root)
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    labels: list[str] = []
    data: list[pd.Series] = []
    total_negative_delays = 0

    for policy_dir in policies:
        try:
            values, removed_neg = load_metric_for_policy(policy_dir, args.metric, args.flow_type)
        except Exception as exc:
            print(f"ERROR [{policy_dir.name}]: {exc}", file=sys.stderr)
            return 1

        labels.append(policy_dir.name)
        data.append(values)
        total_negative_delays += removed_neg
        if args.metric == "e2e_delay" and removed_neg:
            print(f"WARNING [{policy_dir.name}]: removed {removed_neg} negative delay rows", file=sys.stderr)

    if args.metric == "e2e_delay":
        print(f"INFO: total negative delay rows removed={total_negative_delays}", file=sys.stderr)

    render_plot(data, labels, args.metric, args.out_dir, kind=kind)
    return 0


def main() -> int:
    return run(kind="box")


if __name__ == "__main__":
    raise SystemExit(main())
