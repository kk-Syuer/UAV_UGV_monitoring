#!/usr/bin/env python3
"""Build time-aligned cross-layer window tables.

For each run, bins all relevant metrics onto a uniform time grid and writes
per-run and per-protocol-merged CSVs.

Usage
-----
    python scripts/build_cross_layer_windows.py \
        --input  experiment_data_collection/test_round2 \
        --output analysis/test_round2/cross_layer \
        [--bin-sec 60]

Output
------
    <output>/tables/<protocol>_<replicate>.csv   — per-run window table
    <output>/tables/<protocol>_merged.csv        — replicates averaged per bin
"""

import argparse
import gc
import re
import sys
import warnings
from pathlib import Path

import numpy as np
import pandas as pd

# ---------------------------------------------------------------------------
# Minimal helpers (inlined to avoid importing plotting_utils which pulls in
# matplotlib and inflates subprocess RSS by 100–300 MB)
# ---------------------------------------------------------------------------

def _parse_protocol_and_replicate(folder_name: str) -> tuple[str, int]:
    m = re.match(r"^(.+?)_(\d+)$", folder_name)
    if m:
        return m.group(1), int(m.group(2))
    return folder_name, 1


def discover_runs(root) -> list:
    root = Path(root)
    runs: list = []
    if not root.exists():
        warnings.warn(f"Input root does not exist: {root}")
        return runs
    for item in sorted(root.iterdir()):
        if not item.is_dir():
            continue
        if not (any(item.glob("*.csv")) or (item / "summary.json").exists()):
            continue
        protocol, replicate = _parse_protocol_and_replicate(item.name)
        runs.append({"folder": item, "protocol": protocol,
                     "replicate": replicate, "label": f"{protocol} r{replicate}"})
    return runs


_COMPAT_RENAMES = [
    ("time", "time_s"), ("request_time", "request_time_s"),
    ("decision_time", "decision_time_s"), ("terminal_time", "terminal_time_s"),
]


def load_csv_if_exists(path) -> pd.DataFrame | None:
    path = Path(path)
    if not path.exists() or path.stat().st_size == 0:
        return None
    try:
        df = pd.read_csv(path)
    except Exception as exc:
        warnings.warn(f"Failed to read {path}: {exc}")
        return None
    if df.empty:
        return None
    for old, new in _COMPAT_RENAMES:
        if old in df.columns and new not in df.columns:
            df.rename(columns={old: new}, inplace=True)
    return df


def ensure_t_rel(df: pd.DataFrame, time_col: str = "time_s") -> pd.DataFrame:
    if "t_rel_s" not in df.columns and time_col in df.columns:
        df["t_rel_s"] = df[time_col] - df[time_col].min()
    return df

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

_FILE_MAP = {
    "network_timeseries":      "network_timeseries.csv",
    "charge_events":           "charge_events.csv",
    "death_events":            "death_events.csv",
    "charge_queue_timeseries": "charge_queue_timeseries.csv",
    "status_timeseries":       "status_timeseries.csv",
}

_CHUNK = 50_000   # rows per chunk for streaming reads


# ---------------------------------------------------------------------------
# Streaming aggregation helper
# ---------------------------------------------------------------------------

def _stream_agg(
    path: Path,
    mean_cols:    dict[str, str]             | None = None,
    derived_cols: dict[str, list[str]]       | None = None,
    frac_cols:    dict[str, tuple[str, float]]| None = None,
    bin_sec: float = 60.0,
    filter_col:  str | None = None,
    filter_excl: str | None = None,
) -> tuple[dict[int, dict[str, float]], float]:
    """Read *path* in chunks; accumulate per-bin statistics.  Never holds
    more than ``_CHUNK`` rows in memory at once.

    Parameters
    ----------
    mean_cols    : {csv_col: output_key}  — compute mean per bin
    derived_cols : {output_key: [csv_col, …]} — row-wise sum, then mean per bin
    frac_cols    : {csv_col: (output_key, threshold)} — fraction of rows < threshold
    filter_col   : column name used for row filtering
    filter_excl  : value to exclude (rows where filter_col == filter_excl are dropped)

    Returns
    -------
    bins : {bin_idx: {output_key: value}}
    t_max : max relative time in seconds
    """
    if not path.exists() or path.stat().st_size == 0:
        return {}, 0.0

    mean_cols    = mean_cols    or {}
    derived_cols = derived_cols or {}
    frac_cols    = frac_cols    or {}

    # ── Detect time column (old schema: 'time'; new schema: 'time_s') ──────
    try:
        header_cols = pd.read_csv(path, nrows=0).columns.tolist()
    except Exception as exc:
        warnings.warn(f"Cannot read header of {path}: {exc}")
        return {}, 0.0

    t_raw = "time_s" if "time_s" in header_cols else ("time" if "time" in header_cols else None)
    if t_raw is None:
        warnings.warn(f"No time column found in {path}")
        return {}, 0.0

    # Only request columns that actually exist in the file
    needed = set([t_raw])
    for c in mean_cols:
        if c in header_cols:
            needed.add(c)
    for cols_list in derived_cols.values():
        needed.update(c for c in cols_list if c in header_cols)
    for c in frac_cols:
        if c in header_cols:
            needed.add(c)
    if filter_col and filter_col in header_cols:
        needed.add(filter_col)
    read_cols = list(needed)

    # ── First pass: find global min_time using only the time column ─────────
    try:
        t_series = pd.read_csv(path, usecols=[t_raw])[t_raw].dropna()
        if t_series.empty:
            return {}, 0.0
        min_time = float(t_series.min())
        t_max    = float(t_series.max()) - min_time
        del t_series
        gc.collect()
    except Exception as exc:
        warnings.warn(f"Cannot read time column from {path}: {exc}")
        return {}, 0.0

    # ── Accumulators ────────────────────────────────────────────────────────
    b_sum: dict[int, dict[str, float]] = {}  # {bin: {key: running_sum}}
    b_cnt: dict[int, dict[str, int]]   = {}  # {bin: {key: running_count}}
    b_blw: dict[int, dict[str, int]]   = {}  # {bin: {key: running_count_below}}

    # ── Second pass: stream chunks ──────────────────────────────────────────
    try:
        for chunk in pd.read_csv(path, usecols=read_cols, chunksize=_CHUNK,
                                 low_memory=True):
            if filter_col and filter_col in chunk.columns and filter_excl is not None:
                chunk = chunk[chunk[filter_col] != filter_excl]
            if chunk.empty:
                continue

            t_rel = chunk[t_raw].values.astype(np.float64) - min_time
            valid = t_rel >= 0
            if not valid.any():
                continue
            chunk  = chunk[valid]
            t_rel  = t_rel[valid]
            bins_v = (t_rel // bin_sec).astype(np.int32)

            chunk = chunk.copy()
            chunk["_bin"] = bins_v

            for bi_val, grp in chunk.groupby("_bin", sort=False):
                bi = int(bi_val)
                if bi not in b_sum:
                    b_sum[bi] = {}
                    b_cnt[bi] = {}
                    b_blw[bi] = {}

                # mean_cols
                for csv_col, out_key in mean_cols.items():
                    if csv_col not in grp.columns:
                        continue
                    vals = grp[csv_col].dropna().values
                    if len(vals) == 0:
                        continue
                    b_sum[bi][out_key] = b_sum[bi].get(out_key, 0.0) + float(vals.sum())
                    b_cnt[bi][out_key] = b_cnt[bi].get(out_key, 0)   + len(vals)

                # derived_cols (row-wise sum → mean)
                for out_key, col_list in derived_cols.items():
                    avail = [c for c in col_list if c in grp.columns]
                    if not avail:
                        continue
                    row_totals = grp[avail].sum(axis=1).dropna().values
                    if len(row_totals) == 0:
                        continue
                    b_sum[bi][out_key] = b_sum[bi].get(out_key, 0.0) + float(row_totals.sum())
                    b_cnt[bi][out_key] = b_cnt[bi].get(out_key, 0)   + len(row_totals)

                # frac_cols
                for csv_col, (out_key, threshold) in frac_cols.items():
                    if csv_col not in grp.columns:
                        continue
                    vals = grp[csv_col].dropna().values
                    if len(vals) == 0:
                        continue
                    # Reuse count from mean_cols if same column was also in mean_cols
                    mean_key = mean_cols.get(csv_col, "_frac_" + csv_col)
                    b_cnt[bi][out_key + "_n"] = b_cnt[bi].get(out_key + "_n", 0) + len(vals)
                    b_blw[bi][out_key]        = b_blw[bi].get(out_key, 0) + int((vals < threshold).sum())

    except Exception as exc:
        warnings.warn(f"Error streaming {path}: {exc}")

    # ── Compute final per-bin values ────────────────────────────────────────
    result: dict[int, dict[str, float]] = {}
    for bi in set(b_sum) | set(b_blw):
        result[bi] = {}
        for out_key in b_sum.get(bi, {}):
            n = b_cnt.get(bi, {}).get(out_key, 0)
            if n > 0:
                result[bi][out_key] = b_sum[bi][out_key] / n
        for out_key, below in b_blw.get(bi, {}).items():
            n = b_cnt.get(bi, {}).get(out_key + "_n", 0)
            if n > 0:
                result[bi][out_key] = below / n

    return result, t_max


# ---------------------------------------------------------------------------
# Small-file loader (charge_events, death_events)
# ---------------------------------------------------------------------------

def _load_small(folder: Path, key: str) -> pd.DataFrame | None:
    """Full load for small event tables (charge_events, death_events)."""
    path = folder / _FILE_MAP[key]
    df = load_csv_if_exists(path)
    if df is not None:
        ensure_t_rel(df)
    return df


def _build_bins(t_max_s: float, bin_sec: float) -> np.ndarray:
    return np.arange(0, t_max_s + bin_sec, bin_sec)


# ---------------------------------------------------------------------------
# Per-run window builder
# ---------------------------------------------------------------------------

def build_run_windows(run: dict, bin_sec: float) -> pd.DataFrame | None:
    folder    = run["folder"]
    protocol  = run["protocol"]
    replicate = run["replicate"]
    tag       = f"{protocol} r{replicate}"
    warn: list[str] = []

    # ------------------------------------------------------------------ #
    # 1. network_timeseries — streaming
    # ------------------------------------------------------------------ #
    print(f"  [{tag}] network_timeseries …", flush=True)
    net_bins, t_max = _stream_agg(
        folder / _FILE_MAP["network_timeseries"],
        mean_cols={
            "window_pdr":            "dpr_mean",
            "window_delay_mean_ms":  "e2e_delay_mean_ms",
            "window_delay_p95_ms":   "e2e_delay_p95_ms",
        },
        bin_sec=bin_sec,
    )
    if not net_bins:
        warn.append(f"  [WARN] {tag}: network_timeseries empty or missing")
    gc.collect()

    # ------------------------------------------------------------------ #
    # 2. charge_events — full load (small file)
    # ------------------------------------------------------------------ #
    print(f"  [{tag}] charge_events …", flush=True)
    ce = _load_small(folder, "charge_events")
    ce_bins: dict[int, dict] = {}
    if ce is not None:
        time_col = next((c for c in ("request_time_s", "request_time", "t_rel_s")
                         if c in ce.columns), None)
        if time_col is None:
            warn.append(f"  [WARN] {tag}: charge_events no usable time column")
        else:
            t0 = ce[time_col].min()
            ce["_t_rel_s"] = ce[time_col] - t0
            t_max = max(t_max, float(ce["_t_rel_s"].max()))
            ce["_bin"] = (ce["_t_rel_s"] // bin_sec).astype(int)
            for b, grp in ce.groupby("_bin"):
                row: dict = {}
                if "outcome" in grp.columns:
                    vc = grp["outcome"].value_counts()
                    row["started_count"]  = int(vc.get("STARTED",  0))
                    row["timeout_count"]  = int(vc.get("TIMEOUT",  0))
                    row["success_count"]  = int(vc.get("STARTED",  0))
                    row["rejected_count"] = int(vc.get("REJECTED", 0))
                    row["dropped_count"]  = int(vc.get("DROPPED",  0))
                    n = len(grp)
                    row["timeout_rate"] = row["timeout_count"] / n if n else np.nan
                    row["success_rate"] = row["success_count"] / n if n else np.nan
                if "waiting_time_ms"      in grp.columns:
                    row["mean_wait_ms"]           = float(grp["waiting_time_ms"].mean())
                    row["median_wait_ms"]         = float(grp["waiting_time_ms"].median())
                if "effective_wait_ms"    in grp.columns:
                    row["mean_effective_wait_ms"] = float(grp["effective_wait_ms"].mean())
                if "decision_latency_ms"  in grp.columns:
                    row["decision_latency_mean_ms"]   = float(grp["decision_latency_ms"].mean())
                    row["decision_latency_median_ms"] = float(grp["decision_latency_ms"].median())
                ce_bins[int(b)] = row
    else:
        warn.append(f"  [WARN] {tag}: charge_events.csv not found")
    del ce
    gc.collect()

    # ------------------------------------------------------------------ #
    # 3. death_events — full load (small file)
    # ------------------------------------------------------------------ #
    print(f"  [{tag}] death_events …", flush=True)
    de = _load_small(folder, "death_events")
    de_bins: dict[int, int] = {}
    if de is not None and "t_rel_s" in de.columns and not de.empty:
        t_max = max(t_max, float(de["t_rel_s"].max()))
        de["_bin"] = (de["t_rel_s"] // bin_sec).astype(int)
        for b, grp in de.groupby("_bin"):
            de_bins[int(b)] = len(grp)
    else:
        warn.append(f"  [WARN] {tag}: death_events missing or empty")
    del de
    gc.collect()

    # ------------------------------------------------------------------ #
    # 4. charge_queue_timeseries — streaming
    # ------------------------------------------------------------------ #
    print(f"  [{tag}] charge_queue_timeseries …", flush=True)
    cq_bins, cq_tmax = _stream_agg(
        folder / _FILE_MAP["charge_queue_timeseries"],
        mean_cols={
            "ugv_dock_utilization": "dock_util_mean",
        },
        derived_cols={
            "queue_len_mean": ["queue_length_ugv", "queue_length_ch", "queue_length_member"],
        },
        bin_sec=bin_sec,
    )
    t_max = max(t_max, cq_tmax)
    if not cq_bins:
        warn.append(f"  [WARN] {tag}: charge_queue_timeseries empty or missing")
    gc.collect()

    # ------------------------------------------------------------------ #
    # 5. status_timeseries — streaming (largest file; filter sink_gateway)
    # ------------------------------------------------------------------ #
    print(f"  [{tag}] status_timeseries …", flush=True)
    st_bins, st_tmax = _stream_agg(
        folder / _FILE_MAP["status_timeseries"],
        mean_cols={
            "battery_level": "mean_battery",
        },
        frac_cols={
            "battery_level": ("frac_low_battery", 20.0),
        },
        bin_sec=bin_sec,
        filter_col="uav_id",
        filter_excl="sink_gateway",
    )
    t_max = max(t_max, st_tmax)
    if not st_bins:
        warn.append(f"  [WARN] {tag}: status_timeseries empty or missing")
    gc.collect()

    # ------------------------------------------------------------------ #
    # 6. Assemble final table
    # ------------------------------------------------------------------ #
    for w in warn:
        print(w, flush=True)

    if t_max == 0.0:
        print(f"  [SKIP] {tag}: no usable time data", flush=True)
        return None

    bins = _build_bins(t_max, bin_sec)
    records = []
    cum_deaths = 0

    for t_start in bins:
        b = int(t_start // bin_sec)
        row: dict = {
            "bin_start_s":   float(t_start),
            "bin_start_min": round(float(t_start) / 60.0, 4),
            "protocol":      protocol,
            "replicate":     replicate,
        }
        row.update(net_bins.get(b, {}))
        row.update(ce_bins.get(b, {}))
        deaths = de_bins.get(b, 0)
        cum_deaths += deaths
        row["death_count"] = deaths
        row["cum_deaths"]  = cum_deaths
        row.update(cq_bins.get(b, {}))
        row.update(st_bins.get(b, {}))
        records.append(row)

    df_out = pd.DataFrame(records)
    front = [
        "bin_start_s", "bin_start_min", "protocol", "replicate",
        "dpr_mean", "e2e_delay_mean_ms", "e2e_delay_p95_ms",
        "timeout_count", "success_count", "started_count",
        "rejected_count", "dropped_count",
        "timeout_rate", "success_rate",
        "mean_wait_ms", "median_wait_ms", "mean_effective_wait_ms",
        "decision_latency_mean_ms", "decision_latency_median_ms",
        "dock_util_mean", "queue_len_mean",
        "death_count", "cum_deaths",
        "mean_battery", "frac_low_battery",
    ]
    cols = [c for c in front if c in df_out.columns] + \
           [c for c in df_out.columns if c not in front]
    return df_out[cols]


# ---------------------------------------------------------------------------
# Merge replicates per protocol
# ---------------------------------------------------------------------------

def merge_protocol(frames: list[pd.DataFrame]) -> pd.DataFrame:
    """Average numeric columns across replicates, aligned by bin_start_s."""
    if not frames:
        return pd.DataFrame()
    combined = pd.concat(frames, ignore_index=True)
    group_cols = ["bin_start_s", "bin_start_min", "protocol"]
    agg_num = {c: "mean" for c in combined.select_dtypes(include=np.number).columns
               if c not in ("bin_start_s", "bin_start_min", "replicate")}
    for cnt_col in ("timeout_count", "success_count", "started_count",
                    "rejected_count", "dropped_count", "death_count"):
        if cnt_col in agg_num:
            agg_num[cnt_col] = "sum"
    if "cum_deaths" in agg_num:
        agg_num["cum_deaths"] = "mean"
    agg_df = combined.groupby(group_cols, as_index=False).agg(agg_num)
    return agg_df.sort_values("bin_start_s").reset_index(drop=True)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description="Build cross-layer window tables.")
    ap.add_argument("--input",   default="experiment_data_collection/test_round2",
                    help="Root directory of experiment data (contains per-run folders)")
    ap.add_argument("--output",  default="analysis/test_round2/cross_layer",
                    help="Output root directory")
    ap.add_argument("--bin-sec", type=float, default=60.0,
                    help="Bin width in seconds (default: 60)")
    args = ap.parse_args()

    input_root  = Path(args.input)
    output_root = Path(args.output)
    bin_sec     = args.bin_sec
    tables_dir  = output_root / "tables"
    tables_dir.mkdir(parents=True, exist_ok=True)

    runs = discover_runs(input_root)
    if not runs:
        print(f"[ERROR] No runs found under {input_root}", flush=True)
        sys.exit(1)

    # ── Data coverage report ─────────────────────────────────────────────
    print("\n=== Data Coverage Report ===", flush=True)
    protocols_seen: dict[str, list] = {}
    for run in runs:
        protocols_seen.setdefault(run["protocol"], []).append(run["replicate"])
    for proto, reps in sorted(protocols_seen.items()):
        sample_run = next(r for r in runs if r["protocol"] == proto)
        file_presence = [
            f"{'✓' if (sample_run['folder'] / fname).exists() else '✗'} {fname}"
            for fname in _FILE_MAP.values()
        ]
        print(f"\n  Protocol: {proto}  (replicates: {sorted(reps)})")
        for fp in file_presence:
            print(f"    {fp}")
    print(flush=True)

    # ── Build per-run tables ─────────────────────────────────────────────
    groups: dict[str, list[pd.DataFrame]] = {}
    for run in sorted(runs, key=lambda r: (r["protocol"], r["replicate"])):
        tag = f"{run['protocol']}_r{run['replicate']}"
        print(f"[build] {tag}", flush=True)
        df = build_run_windows(run, bin_sec)
        if df is None or df.empty:
            print(f"  [SKIP] {tag}: empty result", flush=True)
            continue
        out_path = tables_dir / f"{run['protocol']}_{run['replicate']}.csv"
        df.to_csv(out_path, index=False)
        print(f"  [OK] {tag} → {out_path.name}", flush=True)
        groups.setdefault(run["protocol"], []).append(df)
        del df
        gc.collect()

    # ── Merge replicates per protocol ────────────────────────────────────
    print(flush=True)
    for proto, frames in sorted(groups.items()):
        merged = merge_protocol(frames)
        if merged.empty:
            print(f"[SKIP] merged {proto}: empty", flush=True)
            continue
        out_path = tables_dir / f"{proto}_merged.csv"
        merged.to_csv(out_path, index=False)
        print(f"[merged] {proto}  ({len(frames)} replicates)  → {out_path.name}", flush=True)

    print(f"\nDone. Tables written to: {tables_dir}", flush=True)


if __name__ == "__main__":
    main()
