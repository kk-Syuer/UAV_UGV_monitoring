#!/usr/bin/env python3
"""Data cleaning utilities for round1 analysis scripts.

# requirements.txt snippet:
# pandas>=1.5
# numpy>=1.23
"""

from __future__ import annotations

from typing import Iterable

import numpy as np
import pandas as pd


ROLE_MAP = {0: "member", 1: "CH"}


def normalize_role(series: pd.Series) -> pd.Series:
    """Normalize role values (0/1/member/ch) to labels member/CH."""
    if series.dtype.kind in {"i", "u", "f"}:
        return series.map(lambda x: ROLE_MAP.get(int(x), "unknown") if pd.notna(x) else np.nan)
    cleaned = series.astype(str).str.strip().str.lower()
    return cleaned.map({"0": "member", "1": "CH", "member": "member", "ch": "CH", "clusterhead": "CH"}).fillna("unknown")


def safe_numeric(df: pd.DataFrame, columns: Iterable[str]) -> pd.DataFrame:
    """Coerce selected columns to numeric with NaN for invalid values."""
    for col in columns:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors="coerce")
    return df


def align_time_seconds(df: pd.DataFrame, time_col: str = "time") -> pd.DataFrame:
    """Convert absolute time to relative mission seconds, per run_id when present."""
    if time_col not in df.columns:
        return df
    out = df.copy()
    out[time_col] = pd.to_numeric(out[time_col], errors="coerce")
    if "run_id" in out.columns:
        out[time_col] = out[time_col] - out.groupby("run_id")[time_col].transform("min")
    else:
        out[time_col] = out[time_col] - out[time_col].min()
    return out


def drop_time_resets(df: pd.DataFrame, time_col: str = "time") -> tuple[pd.DataFrame, int]:
    """Drop rows where time decreases compared to prior row in run (logging reset)."""
    if time_col not in df.columns:
        return df, 0
    out = df.copy()
    if "run_id" in out.columns:
        dec_mask = out.groupby("run_id")[time_col].diff().lt(0).fillna(False)
    else:
        dec_mask = out[time_col].diff().lt(0).fillna(False)
    dropped = int(dec_mask.sum())
    out = out.loc[~dec_mask].copy()
    return out, dropped


def winsorize_series(series: pd.Series, lower_q: float = 0.01, upper_q: float = 0.99) -> pd.Series:
    """Winsorize a numeric series by quantile clipping."""
    clean = pd.to_numeric(series, errors="coerce")
    lo = clean.quantile(lower_q)
    hi = clean.quantile(upper_q)
    return clean.clip(lower=lo, upper=hi)
