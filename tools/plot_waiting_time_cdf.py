#!/usr/bin/env python3
"""Plot empirical CDFs of charging waiting time by role/policy.

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
from tools.utils_io import discover_policy_dirs, load_csv_with_hints, require_columns

FONT_SIZE = 12


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Plot waiting-time CDF from charge_events.csv")
    p.add_argument("--data_root", required=True, type=Path)
    p.add_argument("--out_dir", type=Path, default=Path("analysis/test_round1"))
    p.add_argument("--role", required=True, choices=["CH", "MEMBER", "ALL"])
    p.add_argument("--use_effective_wait", action="store_true", help="Use effective_wait_ms instead of waiting_time_ms")
    return p.parse_args()


def _extract_wait_series(policy_dir: Path, role: str, use_effective_wait: bool) -> tuple[pd.Series, int]:
    path = policy_dir / "charge_events.csv"
    df = load_csv_with_hints(path)

    wait_col = "effective_wait_ms" if use_effective_wait else "waiting_time_ms"
    require_columns(df, [wait_col], path)
    if role != "ALL":
        require_columns(df, ["role"], path)

    df = safe_numeric(df, [wait_col])

    # Column-aware missing handling: for wait columns, treat 0 as missing sentinel.
    zero_missing = df[wait_col].eq(0).fillna(False)
    if zero_missing.any():
        df.loc[zero_missing, wait_col] = np.nan

    if role != "ALL":
        df["role_label"] = normalize_role(df["role"])
        target = "CH" if role == "CH" else "member"
        df = df[df["role_label"].eq(target)]

    neg_mask = df[wait_col].lt(0).fillna(False)
    neg_count = int(neg_mask.sum())
    if neg_count:
        df = df.loc[~neg_mask]

    return df[wait_col].dropna().sort_values(), neg_count


def _cdf_xy(values: pd.Series) -> tuple[np.ndarray, np.ndarray]:
    x = values.to_numpy(dtype=float)
    n = len(x)
    if n == 0:
        return np.array([]), np.array([])
    y = np.arange(1, n + 1, dtype=float) / n
    return x, y


def _render(role: str, args: argparse.Namespace) -> int:
    try:
        policies = discover_policy_dirs(args.data_root)
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    plt.rcParams.update({"font.size": FONT_SIZE, "axes.labelsize": FONT_SIZE, "legend.fontsize": FONT_SIZE - 1})
    fig, ax = plt.subplots(figsize=(10, 5))

    wait_label = "Effective wait" if args.use_effective_wait else "Waiting time"
    total_neg = 0

    for policy_dir in policies:
        try:
            vals, neg = _extract_wait_series(policy_dir, role, args.use_effective_wait)
        except Exception as exc:
            print(f"ERROR [{policy_dir.name}]: {exc}", file=sys.stderr)
            return 1
        total_neg += neg
        if neg:
            print(f"WARNING [{policy_dir.name}]: removed {neg} negative wait rows", file=sys.stderr)

        x, y = _cdf_xy(vals)
        if len(x) == 0:
            print(f"WARNING [{policy_dir.name}]: no valid wait values for role={role}", file=sys.stderr)
            continue
        ax.plot(x, y, label=policy_dir.name)

    print(f"INFO: total negative waits removed ({role})={total_neg}", file=sys.stderr)

    ax.set_xlabel(f"{wait_label} (ms)")
    ax.set_ylabel("Empirical CDF (ratio)")
    ax.set_title(f"Charging wait-time CDF ({role})")
    ax.grid(alpha=0.3)
    ax.legend(loc="best")
    fig.tight_layout()

    fig_name = f"wait_cdf_{role.lower()}"
    target_dir = args.out_dir / fig_name
    target_dir.mkdir(parents=True, exist_ok=True)
    png = target_dir / f"{fig_name}.png"
    pdf = target_dir / f"{fig_name}.pdf"
    fig.savefig(png, dpi=200)
    fig.savefig(pdf)
    plt.close(fig)
    print(f"Saved: {png}")
    print(f"Saved: {pdf}")
    return 0


def main() -> int:
    args = parse_args()
    if args.role == "ALL":
        rc_member = _render("MEMBER", args)
        rc_ch = _render("CH", args)
        return 0 if rc_member == 0 and rc_ch == 0 else 1
    return _render(args.role, args)


if __name__ == "__main__":
    raise SystemExit(main())
