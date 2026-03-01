"""plotting_utils.py
-------------------
Shared helpers for the UAV/UGV experiment plotting pipeline.

Schema compatibility notes
--------------------------
* Old schema (test_round1): time-columns named ``time``, ``ack_time``,
  ``creation_time``, ``request_time``, ``decision_time``; no ``t_rel_s``;
  no ``protocol_name`` / ``replicate_id`` / ``run_instance_id`` columns.
* New schema (test_round2+): columns named ``time_s``, ``t_rel_s``, etc.
  with run-metadata prefix in every row.

All helpers in this module normalise old → new column names transparently.
"""

import re
import warnings
from pathlib import Path
from typing import Optional, Tuple

import numpy as np
import pandas as pd
import matplotlib
import matplotlib.pyplot as plt

matplotlib.rcParams.update(
    {
        "figure.dpi": 150,
        "axes.grid": True,
        "grid.alpha": 0.3,
        "font.size": 10,
        "axes.labelsize": 10,
        "xtick.labelsize": 9,
        "ytick.labelsize": 9,
        "legend.fontsize": 9,
        "axes.spines.top": False,
        "axes.spines.right": False,
    }
)

# ---------------------------------------------------------------------------
# Run discovery
# ---------------------------------------------------------------------------

def discover_runs(root) -> list:
    """
    Walk *root* and return a list of run-descriptor dicts.

    A valid run folder must contain at least one ``*.csv`` file or a
    ``summary.json`` / ``summary_snapshots.jsonl`` file.

    Folder name format: ``<protocol_name>_<replicate_id>``
    (e.g. ``ugv_dynamic_1``).  If no numeric replicate suffix is found,
    *replicate_id* defaults to ``1``.

    Returns
    -------
    list of dict, each with keys:
        ``folder``    – Path to run directory
        ``protocol``  – Protocol name string (e.g. ``"ugv_dynamic"``)
        ``replicate`` – Replicate integer (e.g. ``1``)
        ``label``     – Human-readable label (e.g. ``"ugv_dynamic r1"``)
    """
    root = Path(root)
    runs: list = []
    if not root.exists():
        warnings.warn(f"Input root does not exist: {root}")
        return runs

    for item in sorted(root.iterdir()):
        if not item.is_dir():
            continue
        has_data = (
            any(item.glob("*.csv"))
            or (item / "summary.json").exists()
            or (item / "summary_snapshots.jsonl").exists()
        )
        if not has_data:
            continue
        protocol, replicate = parse_protocol_and_replicate(item.name)
        runs.append(
            {
                "folder": item,
                "protocol": protocol,
                "replicate": replicate,
                "label": f"{protocol} r{replicate}",
            }
        )
    return runs


def parse_protocol_and_replicate(folder_name: str) -> Tuple[str, int]:
    """
    Split ``ugv_dynamic_1`` → ``("ugv_dynamic", 1)``.

    If the folder name has no trailing ``_<digits>`` suffix the replicate
    defaults to ``1``.
    """
    m = re.match(r"^(.+?)_(\d+)$", folder_name)
    if m:
        return m.group(1), int(m.group(2))
    return folder_name, 1


# ---------------------------------------------------------------------------
# Data loading helpers
# ---------------------------------------------------------------------------

# Map of short keys used by load_run() to file names.
EXPECTED_FILES: dict = {
    "messages":                "messages.csv",
    "charge_events":           "charge_events.csv",
    "status_timeseries":       "status_timeseries.csv",
    "qos_metrics":             "qos_metrics.csv",
    "network_timeseries":      "network_timeseries.csv",
    "charge_queue_timeseries": "charge_queue_timeseries.csv",
    "weather_timeseries":      "weather_timeseries.csv",
    "recovery_events":         "recovery_events.csv",
    "death_events":            "death_events.csv",
    "preemption_events":       "preemption_events.csv",
    # New-schema event tables (test_round2+)
    "packet_generated_events": "packet_generated_events.csv",
    "packet_delivered_events": "packet_delivered_events.csv",
    "charge_request_events":   "charge_request_events.csv",
    "charge_decision_events":  "charge_decision_events.csv",
    "charge_session_events":   "charge_session_events.csv",
    "summary_snapshots":       "summary_snapshots.jsonl",
}

# Old-schema column renames applied transparently by load_csv_if_exists().
_COMPAT_RENAMES: list = [
    ("time",           "time_s"),
    ("ack_time",       "ack_time_s"),
    ("creation_time",  "creation_time_s"),
    ("request_time",   "request_time_s"),
    ("decision_time",  "decision_time_s"),
    ("dock_start_time","dock_start_time_s"),
    ("charge_end_time","charge_end_time_s"),
    ("terminal_time",  "terminal_time_s"),
]


def load_csv_if_exists(
    path,
    rename_time: bool = True,
    missing_report: Optional[list] = None,
) -> Optional[pd.DataFrame]:
    """
    Load a CSV from *path*, returning ``None`` if the file is absent or
    empty.  A warning is appended to *missing_report* (or emitted via
    ``warnings.warn`` if *missing_report* is ``None``) when the file is not
    found.

    Schema compatibility shim
    -------------------------
    When ``rename_time=True`` (default), old-schema column names such as
    ``time`` are silently renamed to their new-schema equivalents (``time_s``)
    so that downstream code can always use the canonical names.
    """
    path = Path(path)
    if not path.exists() or path.stat().st_size == 0:
        msg = f"MISSING file: {path}"
        if missing_report is not None:
            missing_report.append(msg)
        else:
            warnings.warn(msg)
        return None

    try:
        df = pd.read_csv(path)
    except Exception as exc:
        warnings.warn(f"Failed to read {path}: {exc}")
        return None

    if df.empty:
        return None

    if rename_time:
        for old, new in _COMPAT_RENAMES:
            _compat_rename(df, old, new)

    return df


def _compat_rename(df: pd.DataFrame, old: str, new: str) -> None:
    """Rename *old* → *new* only when *old* exists and *new* does not."""
    if old in df.columns and new not in df.columns:
        df.rename(columns={old: new}, inplace=True)


def ensure_t_rel(df: pd.DataFrame, time_col: str = "time_s") -> pd.DataFrame:
    """
    Add ``t_rel_s`` = ``time_col`` − min(``time_col``) if the column is
    absent.  The dataframe is modified in-place and also returned for
    chaining.
    """
    if "t_rel_s" not in df.columns and time_col in df.columns:
        df["t_rel_s"] = df[time_col] - df[time_col].min()
    return df


# ---------------------------------------------------------------------------
# Figure saving
# ---------------------------------------------------------------------------

def savefig(fig: plt.Figure, output_dir, name: str) -> Path:
    """
    Save *fig* as ``<output_dir>/<name>.png``, creating parent dirs as
    needed.  The figure is closed after saving.
    """
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    out_path = output_dir / (name if name.endswith(".png") else name + ".png")
    fig.savefig(out_path, bbox_inches="tight")
    plt.close(fig)
    return out_path


# ---------------------------------------------------------------------------
# Statistical helpers
# ---------------------------------------------------------------------------

def ecdf(data) -> Tuple[np.ndarray, np.ndarray]:
    """
    Return ``(sorted_values, cumulative_probabilities)`` for an empirical
    CDF.  NaN values are dropped.
    """
    arr = np.asarray(data, dtype=float)
    arr = arr[~np.isnan(arr)]
    if len(arr) == 0:
        return np.array([]), np.array([])
    xs = np.sort(arr)
    ys = np.arange(1, len(xs) + 1) / len(xs)
    return xs, ys


def merge_asof_weather(
    df: pd.DataFrame,
    weather_df: pd.DataFrame,
    time_col: str = "time_s",
    weather_time_col: str = "time_s",
) -> pd.DataFrame:
    """
    Left-join *df* with *weather_df* using ``pd.merge_asof`` (nearest
    backward match) on their respective time columns.

    Both dataframes are sorted internally; the original index is reset.
    """
    df_sorted = df.sort_values(time_col).reset_index(drop=True)
    w_sorted = weather_df.sort_values(weather_time_col).reset_index(drop=True)

    # Keep only weather columns that do not already exist in df
    extra_w_cols = [c for c in w_sorted.columns
                    if c not in df_sorted.columns or c == weather_time_col]
    w_sorted = w_sorted[[weather_time_col] + [c for c in extra_w_cols
                                               if c != weather_time_col]]

    merged = pd.merge_asof(
        df_sorted,
        w_sorted,
        left_on=time_col,
        right_on=weather_time_col,
        direction="nearest",
    )
    return merged


# ---------------------------------------------------------------------------
# Color / style helpers
# ---------------------------------------------------------------------------

def protocol_color_map(identifiers: list) -> dict:
    """
    Assign a consistent colour (from matplotlib's ``tab10`` palette) to each
    unique value in *identifiers*.  Identifiers are sorted before assignment
    so that the mapping is reproducible regardless of discovery order.
    """
    cmap = plt.get_cmap("tab10")
    unique = sorted(set(identifiers))
    return {ident: cmap(i % 10) for i, ident in enumerate(unique)}


def label_bars(ax, bars, fmt: str = "{:.2f}", fontsize: int = 8) -> None:
    """Annotate each bar in *bars* with its height value."""
    for bar in bars:
        h = bar.get_height()
        if not np.isnan(h):
            ax.text(
                bar.get_x() + bar.get_width() / 2.0,
                h,
                fmt.format(h),
                ha="center",
                va="bottom",
                fontsize=fontsize,
            )


def deduplicate_legend(ax) -> None:
    """Remove duplicate labels from the axes legend."""
    handles, labels = ax.get_legend_handles_labels()
    seen: dict = {}
    for h, lbl in zip(handles, labels):
        if lbl not in seen:
            seen[lbl] = h
    if seen:
        ax.legend(list(seen.values()), list(seen.keys()))


def boxplot_multi(
    data_by_label: dict,
    ylabel: str,
    title: str,
    fig_dir,
    name: str,
) -> None:
    """
    Draw a side-by-side box plot for each key in *data_by_label* and save
    to *fig_dir*/*name*.png.
    """
    labels = list(data_by_label.keys())
    data = [data_by_label[lbl] for lbl in labels]
    fig, ax = plt.subplots(figsize=(max(8, len(labels) * 1.5), 5))
    bp = ax.boxplot(data, tick_labels=labels, patch_artist=True, showfliers=False)
    cmap = plt.get_cmap("tab10")
    for i, patch in enumerate(bp["boxes"]):
        patch.set_facecolor(cmap(i % 10))
        patch.set_alpha(0.65)
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.set_xticklabels(labels, rotation=20, ha="right")
    savefig(fig, fig_dir, name)
