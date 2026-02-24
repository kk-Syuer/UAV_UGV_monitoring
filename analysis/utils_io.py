#!/usr/bin/env python3
"""I/O helpers for round1 analysis scripts.

# requirements.txt snippet:
# pandas>=1.5
# numpy>=1.23
"""

from __future__ import annotations

import sys
from pathlib import Path
from typing import Iterable

import pandas as pd


DEFAULT_NA_VALUES = [-1, "null", ""]


def discover_policy_dirs(data_root: Path) -> list[Path]:
    if not data_root.exists():
        raise FileNotFoundError(f"Data root does not exist: {data_root}")
    policies = sorted([p for p in data_root.iterdir() if p.is_dir()], key=lambda p: p.name)
    if not policies:
        raise FileNotFoundError(f"No policy directories found under: {data_root}")
    return policies


def load_csv_with_hints(
    csv_path: Path,
    dtype_hints: dict[str, str] | None = None,
    zero_missing_columns: Iterable[str] | None = None,
) -> pd.DataFrame:
    if not csv_path.exists():
        raise FileNotFoundError(f"Missing required CSV: {csv_path}")

    df = pd.read_csv(
        csv_path,
        dtype=dtype_hints,
        na_values=DEFAULT_NA_VALUES,
        keep_default_na=True,
    )

    if zero_missing_columns:
        for col in zero_missing_columns:
            if col in df.columns:
                df.loc[pd.to_numeric(df[col], errors="coerce").eq(0), col] = pd.NA

    return df


def require_columns(df: pd.DataFrame, required: Iterable[str], source: Path) -> None:
    missing = [col for col in required if col not in df.columns]
    if missing:
        msg = f"ERROR: Missing required columns in {source}: {', '.join(missing)}"
        print(msg, file=sys.stderr)
        raise ValueError(msg)
