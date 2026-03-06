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
import sys
import warnings
from pathlib import Path

import numpy as np
import pandas as pd

# Allow imports from the same scripts/ directory when run from repo root.
sys.path.insert(0, str(Path(__file__).parent))
from plotting_utils import discover_runs, load_csv_if_exists, ensure_t_rel  # noqa: E402

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

_FILE_MAP = {
    "network_timeseries":      "network_timeseries.csv",
    "charge_events":           "charge_events.csv",
    "death_events":            "death_events.csv",
    "charge_queue_timeseries": "charge_queue_timeseries.csv",
    "status_timeseries":       "status_timeseries.csv",
}


def _load(folder: Path, key: str) -> pd.DataFrame | None:
    path = folder / _FILE_MAP[key]
    df = load_csv_if_exists(path)
    if df is not None:
        ensure_t_rel(df)
    return df


def _load_slim(folder: Path, key: str, wanted: list[str]) -> pd.DataFrame | None:
    """Load only *wanted* columns from a CSV, applying time rename and t_rel_s.

    Falls back to full load if the requested columns cannot be detected.
    This avoids reading every column from large timeseries files.
    """
    path = folder / _FILE_MAP[key]
    if not path.exists() or path.stat().st_size == 0:
        return None
    try:
        header = pd.read_csv(path, nrows=0)
        available = set(header.columns)
        # Translate 'time_s' request → 'time' when old schema
        actual = []
        for c in wanted:
            if c in available:
                actual.append(c)
            elif c == "time_s" and "time" in available:
                actual.append("time")
        if not actual:
            return None
        df = pd.read_csv(path, usecols=actual)
        if "time" in df.columns and "time_s" not in df.columns:
            df.rename(columns={"time": "time_s"}, inplace=True)
        ensure_t_rel(df)
        return df if not df.empty else None
    except Exception as exc:
        warnings.warn(f"_load_slim failed for {path}: {exc}")
        return None


def _bin_idx(t_rel_s: pd.Series, bin_sec: float) -> pd.Series:
    """Return zero-based bin index for each timestamp."""
    return (t_rel_s // bin_sec).astype(int)


def _build_bins(t_max_s: float, bin_sec: float) -> np.ndarray:
    return np.arange(0, t_max_s + bin_sec, bin_sec)


# ---------------------------------------------------------------------------
# Per-run window builder
# ---------------------------------------------------------------------------

def build_run_windows(run: dict, bin_sec: float) -> pd.DataFrame | None:
    folder = run["folder"]
    protocol = run["protocol"]
    replicate = run["replicate"]
    warnings_issued = []

    # ------------------------------------------------------------------ #
    # 1. network_timeseries  → dpr_mean, e2e_delay_mean_ms, e2e_delay_p95
    # ------------------------------------------------------------------ #
    _net_cols = ["time_s", "window_pdr", "window_delay_mean_ms", "window_delay_p95_ms"]
    net = _load_slim(folder, "network_timeseries", _net_cols)
    net_bins: dict[int, dict] = {}
    t_max = 0.0

    if net is not None and "t_rel_s" in net.columns:
        if "window_pdr" not in net.columns:
            warnings_issued.append("  [WARN] network_timeseries: missing window_pdr")
        net = net[net["t_rel_s"] >= 0].copy()
        t_max = max(t_max, net["t_rel_s"].max() if not net.empty else 0.0)
        net["_bin"] = _bin_idx(net["t_rel_s"], bin_sec)
        for b, grp in net.groupby("_bin"):
            row: dict = {}
            if "window_pdr" in grp.columns:
                row["dpr_mean"] = grp["window_pdr"].mean()
            if "window_delay_mean_ms" in grp.columns:
                row["e2e_delay_mean_ms"] = grp["window_delay_mean_ms"].mean()
            if "window_delay_p95_ms" in grp.columns:
                row["e2e_delay_p95_ms"] = grp["window_delay_p95_ms"].mean()
            net_bins[b] = row
    else:
        warnings_issued.append(f"  [WARN] {protocol} r{replicate}: network_timeseries missing or no t_rel_s")
    del net

    # ------------------------------------------------------------------ #
    # 2. charge_events  → outcome counts, rates, wait / latency
    # ------------------------------------------------------------------ #
    _ce_cols = ["request_time_s", "request_time", "outcome",
                "waiting_time_ms", "effective_wait_ms", "decision_latency_ms"]
    ce = _load_slim(folder, "charge_events", _ce_cols)
    ce_bins: dict[int, dict] = {}

    if ce is not None:
        # Determine the event time column (request_time_s preferred)
        time_col = None
        for cand in ("request_time_s", "request_time", "t_rel_s"):
            if cand in ce.columns:
                time_col = cand
                break
        if time_col is None:
            warnings_issued.append(f"  [WARN] {protocol} r{replicate}: charge_events no usable time column")
        else:
            if time_col != "t_rel_s":
                ce = ce.copy()
                ce["_t_rel_s"] = ce[time_col] - ce[time_col].min()
            else:
                ce["_t_rel_s"] = ce["t_rel_s"]

            t_max = max(t_max, ce["_t_rel_s"].max() if not ce.empty else 0.0)
            ce["_bin"] = _bin_idx(ce["_t_rel_s"], bin_sec)

            for b, grp in ce.groupby("_bin"):
                row: dict = {}
                if "outcome" in grp.columns:
                    vc = grp["outcome"].value_counts()
                    row["started_count"]  = int(vc.get("STARTED", 0))
                    row["timeout_count"]  = int(vc.get("TIMEOUT", 0))
                    row["success_count"]  = int(vc.get("STARTED", 0))  # alias
                    row["rejected_count"] = int(vc.get("REJECTED", 0))
                    row["dropped_count"]  = int(vc.get("DROPPED", 0))
                    total = len(grp)
                    row["timeout_rate"]  = row["timeout_count"]  / total if total else np.nan
                    row["success_rate"]  = row["success_count"]  / total if total else np.nan
                if "waiting_time_ms" in grp.columns:
                    row["mean_wait_ms"]   = grp["waiting_time_ms"].mean()
                    row["median_wait_ms"] = grp["waiting_time_ms"].median()
                if "effective_wait_ms" in grp.columns:
                    row["mean_effective_wait_ms"] = grp["effective_wait_ms"].mean()
                if "decision_latency_ms" in grp.columns:
                    row["decision_latency_mean_ms"]   = grp["decision_latency_ms"].mean()
                    row["decision_latency_median_ms"] = grp["decision_latency_ms"].median()
                ce_bins[b] = row
    else:
        warnings_issued.append(f"  [WARN] {protocol} r{replicate}: charge_events.csv not found")
    del ce

    # ------------------------------------------------------------------ #
    # 3. death_events  → death_count, cum_deaths
    # ------------------------------------------------------------------ #
    de = _load(folder, "death_events")  # small file, full load is fine
    de_bins: dict[int, int] = {}

    if de is not None and "t_rel_s" in de.columns and not de.empty:
        t_max = max(t_max, de["t_rel_s"].max())
        de["_bin"] = _bin_idx(de["t_rel_s"], bin_sec)
        for b, grp in de.groupby("_bin"):
            de_bins[b] = len(grp)
    else:
        warnings_issued.append(f"  [WARN] {protocol} r{replicate}: death_events missing or empty")
    del de

    # ------------------------------------------------------------------ #
    # 4. charge_queue_timeseries  → dock_util_mean, queue_len_mean
    # ------------------------------------------------------------------ #
    _cq_cols = ["time_s", "ugv_dock_utilization",
                "queue_length_ugv", "queue_length_ch", "queue_length_member"]
    cq = _load_slim(folder, "charge_queue_timeseries", _cq_cols)
    cq_bins: dict[int, dict] = {}

    if cq is not None and "t_rel_s" in cq.columns and not cq.empty:
        t_max = max(t_max, cq["t_rel_s"].max())
        cq["_bin"] = _bin_idx(cq["t_rel_s"], bin_sec)
        # queue length: sum of all role-specific columns if present
        q_cols = [c for c in ("queue_length_ugv", "queue_length_ch", "queue_length_member")
                  if c in cq.columns]
        for b, grp in cq.groupby("_bin"):
            row: dict = {}
            if "ugv_dock_utilization" in grp.columns:
                row["dock_util_mean"] = grp["ugv_dock_utilization"].mean()
            if q_cols:
                row["queue_len_mean"] = grp[q_cols].sum(axis=1).mean()
            cq_bins[b] = row
    else:
        warnings_issued.append(f"  [WARN] {protocol} r{replicate}: charge_queue_timeseries missing")
    del cq

    # ------------------------------------------------------------------ #
    # 5. status_timeseries  → mean_battery, frac_low_battery
    # Slim load: only the 3 columns we use (largest file in the dataset).
    # ------------------------------------------------------------------ #
    _st_cols = ["time_s", "uav_id", "battery_level"]
    st = _load_slim(folder, "status_timeseries", _st_cols)
    st_bins: dict[int, dict] = {}

    if st is not None and "t_rel_s" in st.columns and not st.empty:
        if "uav_id" in st.columns:
            st = st[st["uav_id"] != "sink_gateway"].copy()
        t_max = max(t_max, st["t_rel_s"].max())
        if "battery_level" in st.columns:
            st["_bin"] = _bin_idx(st["t_rel_s"], bin_sec)
            for b, grp in st.groupby("_bin"):
                batt = grp["battery_level"].dropna()
                st_bins[b] = {
                    "mean_battery":      float(batt.mean()) if len(batt) > 0 else np.nan,
                    "frac_low_battery":  float((batt < 20).mean()) if len(batt) > 0 else np.nan,
                }
        else:
            warnings_issued.append(f"  [WARN] {protocol} r{replicate}: status_timeseries no battery_level")
    else:
        warnings_issued.append(f"  [WARN] {protocol} r{replicate}: status_timeseries missing")
    del st
    gc.collect()

    # ------------------------------------------------------------------ #
    # 6. Assemble final table
    # ------------------------------------------------------------------ #
    if t_max == 0.0:
        for w in warnings_issued:
            print(w)
        print(f"  [SKIP] {protocol} r{replicate}: no usable time data")
        return None

    bins = _build_bins(t_max, bin_sec)
    n_bins = len(bins)
    records = []

    cum_deaths = 0
    for i, t_start in enumerate(bins):
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

    for w in warnings_issued:
        print(w)

    df_out = pd.DataFrame(records)
    # Ensure consistent column order
    front = ["bin_start_s", "bin_start_min", "protocol", "replicate",
             "dpr_mean", "e2e_delay_mean_ms", "e2e_delay_p95_ms",
             "timeout_count", "success_count", "started_count",
             "rejected_count", "dropped_count",
             "timeout_rate", "success_rate",
             "mean_wait_ms", "median_wait_ms", "mean_effective_wait_ms",
             "decision_latency_mean_ms", "decision_latency_median_ms",
             "dock_util_mean", "queue_len_mean",
             "death_count", "cum_deaths",
             "mean_battery", "frac_low_battery"]
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
    # For count columns we want the sum across replicates (not mean)
    for cnt_col in ("timeout_count", "success_count", "started_count",
                    "rejected_count", "dropped_count", "death_count"):
        if cnt_col in agg_num:
            agg_num[cnt_col] = "sum"
    # cum_deaths: take max across replicates (already cumulative per replicate)
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
        file_presence = []
        sample_run = next(r for r in runs if r["protocol"] == proto)
        for key, fname in _FILE_MAP.items():
            exists = (sample_run["folder"] / fname).exists()
            file_presence.append(f"{'✓' if exists else '✗'} {fname}")
        print(f"\n  Protocol: {proto}  (replicates: {sorted(reps)})")
        for fp in file_presence:
            print(f"    {fp}")
    print()

    # ------------------------------------------------------------------ #
    # Build per-run tables
    # ------------------------------------------------------------------ #
    groups: dict[str, list[pd.DataFrame]] = {}
    for run in sorted(runs, key=lambda r: (r["protocol"], r["replicate"])):
        tag = f"{run['protocol']}_r{run['replicate']}"
        print(f"[build] {tag}")
        df = build_run_windows(run, bin_sec)
        if df is None or df.empty:
            print(f"  [SKIP] {tag}: empty result")
            continue
        out_path = tables_dir / f"{run['protocol']}_{run['replicate']}.csv"
        df.to_csv(out_path, index=False)
        print(f"  → {out_path.relative_to(output_root.parent.parent)}")
        groups.setdefault(run["protocol"], []).append(df)

    # ------------------------------------------------------------------ #
    # Build per-protocol merged tables
    # ------------------------------------------------------------------ #
    print()
    for proto, frames in sorted(groups.items()):
        merged = merge_protocol(frames)
        if merged.empty:
            print(f"[SKIP] merged {proto}: empty")
            continue
        out_path = tables_dir / f"{proto}_merged.csv"
        merged.to_csv(out_path, index=False)
        print(f"[merged] {proto}  ({len(frames)} replicates)  → {out_path.relative_to(output_root.parent.parent)}")

    print(f"\nDone. Tables written to: {tables_dir}")


if __name__ == "__main__":
    main()
