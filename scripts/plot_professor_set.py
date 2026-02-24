#!/usr/bin/env python3
from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns


@dataclass
class Table:
    path: Path
    df: pd.DataFrame


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Create professor figure set from tidy extracted CSV metrics")
    p.add_argument("--input", type=Path, default=Path("artifacts/raw_metrics"), help="Input directory with tidy CSV files")
    p.add_argument("--out", type=Path, default=Path("artifacts/figures"), help="Output directory for figures")
    return p.parse_args()


def _first_present(cols: Iterable[str], options: Iterable[str]) -> str | None:
    cols_set = set(cols)
    for c in options:
        if c in cols_set:
            return c
    return None


def _numeric(series: pd.Series) -> pd.Series:
    return pd.to_numeric(series, errors="coerce")


def _infer_policy_run(path: Path, input_root: Path, df: pd.DataFrame) -> pd.DataFrame:
    out = df.copy()
    if "policy" not in out.columns:
        rel = path.relative_to(input_root)
        out["policy"] = rel.parts[0] if rel.parts else "unknown"
    if "run" not in out.columns:
        run_col = _first_present(out.columns, ["run_id", "seed", "repeat", "trial"])
        if run_col is not None:
            out["run"] = out[run_col].astype(str)
        else:
            rel = path.relative_to(input_root)
            out["run"] = rel.parts[1] if len(rel.parts) > 2 else "run0"
    out["policy"] = out["policy"].astype(str)
    out["run"] = out["run"].astype(str)
    return out


def load_tables(input_root: Path) -> list[Table]:
    csvs = sorted(input_root.rglob("*.csv"))
    if not csvs:
        raise FileNotFoundError(f"No CSV files found under {input_root}")

    preferred_raw_names = {
        "pdr_raw.csv",
        "delay_raw.csv",
        "queue_raw.csv",
        "charging_sessions_raw.csv",
    }

    non_summary_csvs = [p for p in csvs if "summary" not in p.stem.lower() and "summary" not in p.name.lower()]
    candidate_csvs = [p for p in non_summary_csvs if p.name in preferred_raw_names]
    if not candidate_csvs:
        candidate_csvs = non_summary_csvs

    tables: list[Table] = []
    for p in candidate_csvs:
        try:
            raw = pd.read_csv(p)
        except Exception:
            continue
        if raw.empty:
            continue
        raw = _infer_policy_run(p, input_root, raw)
        tables.append(Table(path=p, df=raw))

    if not tables:
        listed = "\n".join(f"  - {path.name}" for path in csvs)
        raise RuntimeError(
            "No usable non-summary CSV tables were found. "
            f"Directory scanned: {input_root}\n"
            f"Files seen ({len(csvs)}):\n{listed}"
        )
    return tables


def setup_style() -> None:
    sns.set_theme(style="whitegrid", context="talk")
    plt.rcParams.update(
        {
            "font.family": "DejaVu Sans",
            "axes.titlesize": 14,
            "axes.labelsize": 12,
            "legend.fontsize": 10,
            "xtick.labelsize": 10,
            "ytick.labelsize": 10,
            "figure.dpi": 120,
        }
    )


def save_dual(fig: plt.Figure, out_dir: Path, stem: str) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    fig.tight_layout()
    fig.savefig(out_dir / f"{stem}.png", dpi=300)
    fig.savefig(out_dir / f"{stem}.pdf")
    plt.close(fig)


def build_pdr_frame(tables: list[Table]) -> pd.DataFrame:
    pieces: list[pd.DataFrame] = []
    for t in tables:
        df = t.df
        pdr_col = _first_present(df.columns, ["pdr", "delivery_ratio", "packet_delivery_ratio"])
        num_col = _first_present(df.columns, ["delivered", "delivered_packets", "rx_packets", "received"])
        den_col = _first_present(df.columns, ["sent", "tx_packets", "total_packets", "generated_packets", "attempted"])

        part = df[["policy", "run"]].copy()
        if pdr_col is not None:
            part["pdr"] = _numeric(df[pdr_col])
        elif num_col is not None and den_col is not None:
            num = _numeric(df[num_col])
            den = _numeric(df[den_col])
            den = den.mask(den <= 0)  # drop zero denominator bins before reliability plotting.
            part["pdr"] = num / den
        else:
            continue

        part = part.replace([np.inf, -np.inf], np.nan).dropna(subset=["pdr"])
        if not part.empty:
            pieces.append(part)

    if not pieces:
        raise RuntimeError("Unable to build PDR frame: no PDR or delivered/sent columns found.")
    return pd.concat(pieces, ignore_index=True)


def plot_01_overall_pdr_by_policy(tables: list[Table], out_dir: Path) -> None:
    pdr = build_pdr_frame(tables)
    run_mean = pdr.groupby(["policy", "run"], as_index=False)["pdr"].mean()
    stats = run_mean.groupby("policy")["pdr"].agg(["mean", "count", "std"]).reset_index()
    stats["sem"] = stats["std"] / np.sqrt(stats["count"].clip(lower=1))
    stats.loc[stats["count"] <= 1, "sem"] = 0.0

    fig, ax = plt.subplots(figsize=(9, 5))
    palette = sns.color_palette("Set2", n_colors=len(stats))
    ax.bar(stats["policy"], stats["mean"], yerr=stats["sem"], capsize=4, color=palette, edgecolor="black", linewidth=0.6)
    ax.set_ylim(0, 1.0)
    ax.set_ylabel("Packet Delivery Ratio (PDR)")
    ax.set_title("Overall PDR by policy")
    ax.yaxis.set_major_formatter(lambda x, pos: f"{x:.2f}")

    best = stats.sort_values("mean", ascending=False).iloc[0]
    ax.text(0.02, 0.98, f"Best policy: {best['policy']} ({best['mean']:.3f})", transform=ax.transAxes, va="top", ha="left", fontsize=10)
    save_dual(fig, out_dir, "01_overall_pdr_by_policy")


def build_delay_frame(tables: list[Table]) -> pd.DataFrame:
    candidates = ["e2e_delay_ms", "delay_ms", "latency_ms", "end_to_end_delay_ms"]
    out = []
    for t in tables:
        c = _first_present(t.df.columns, candidates)
        if c is None:
            continue
        part = t.df[["policy"]].copy()
        part["delay_ms"] = _numeric(t.df[c])
        part = part[(part["delay_ms"] >= 0) & part["delay_ms"].notna()]
        if not part.empty:
            out.append(part)
    if not out:
        raise RuntimeError("No raw delay samples found.")
    return pd.concat(out, ignore_index=True)


def plot_02_e2e_delay_distribution_by_policy(tables: list[Table], out_dir: Path) -> None:
    delays = build_delay_frame(tables)
    fig, ax = plt.subplots(figsize=(10, 5))
    sns.violinplot(data=delays, x="policy", y="delay_ms", inner="quartile", cut=0, ax=ax, palette="Set2")
    ax.set_ylabel("End-to-end delay (ms)")
    ax.set_title("End-to-end delay distribution by policy")

    med_iqr = delays.groupby("policy")["delay_ms"].agg(med="median", q1=lambda s: s.quantile(0.25), q3=lambda s: s.quantile(0.75))
    summary = "\n".join([f"{k}: med={v.med:.1f}, IQR={v.q1:.1f}-{v.q3:.1f}" for k, v in med_iqr.iterrows()])
    ax.text(1.01, 0.98, summary, transform=ax.transAxes, va="top", fontsize=9)
    save_dual(fig, out_dir, "02_e2e_delay_distribution_by_policy")


def _kaplan_meier(times: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    t = np.sort(times[~np.isnan(times)])
    if t.size == 0:
        return np.array([0.0]), np.array([1.0])
    n = t.size
    unique, counts = np.unique(t, return_counts=True)
    surv = [1.0]
    x = [0.0]
    at_risk = n
    s = 1.0
    for ti, di in zip(unique, counts):
        s *= (1 - di / at_risk)
        x.extend([ti, ti])
        surv.extend([surv[-1], s])
        at_risk -= di
    return np.array(x[1:]), np.array(surv[1:])


def plot_03_survival_or_deaths_by_policy(tables: list[Table], out_dir: Path) -> None:
    death_time_cols = ["death_time", "death_time_s", "time_to_death_s", "lifetime_s", "time"]
    death_frames: list[pd.DataFrame] = []
    for t in tables:
        if "death" not in t.path.name.lower() and not any("death" in c.lower() for c in t.df.columns):
            continue
        c = _first_present(t.df.columns, death_time_cols)
        if c is None:
            continue
        part = t.df[["policy"]].copy()
        part["death_time"] = _numeric(t.df[c])
        part = part[(part["death_time"] >= 0) & part["death_time"].notna()]
        if not part.empty:
            death_frames.append(part)

    if death_frames:
        deaths = pd.concat(death_frames, ignore_index=True)
        fig, ax = plt.subplots(figsize=(9, 5))
        for policy, grp in deaths.groupby("policy"):
            x, s = _kaplan_meier(grp["death_time"].to_numpy(dtype=float))
            ax.step(x, s, where="post", label=policy)
        ax.set_ylim(0, 1.02)
        ax.set_xlabel("Time (s)")
        ax.set_ylabel("Survival probability")
        ax.set_title("Kaplan–Meier survival by policy")
        ax.legend(loc="best")
        final_vals = deaths.groupby("policy")["death_time"].median().sort_values()
        ax.text(0.02, 0.02, "Median death time (s): " + ", ".join([f"{k}={v:.0f}" for k, v in final_vals.items()]), transform=ax.transAxes, fontsize=9)
    else:
        counts = []
        for t in tables:
            if "death" in t.path.name.lower() or "dead" in " ".join(t.df.columns).lower():
                c = _first_present(t.df.columns, ["is_dead", "dead", "death_event"])
                if c is not None:
                    val = _numeric(t.df[c]).fillna(0)
                    g = t.df[["policy"]].copy()
                    g["count"] = (val > 0).astype(int)
                    counts.append(g.groupby("policy", as_index=False)["count"].sum())
        if counts:
            cnt = pd.concat(counts).groupby("policy", as_index=False)["count"].sum()
        else:
            # Fall back: count rows in death event-like files.
            fallback = []
            for t in tables:
                if "death" in t.path.name.lower():
                    fallback.append(t.df.groupby("policy", as_index=False).size().rename(columns={"size": "count"}))
            if not fallback:
                raise RuntimeError("No death information found for figure 03.")
            cnt = pd.concat(fallback).groupby("policy", as_index=False)["count"].sum()

        fig, ax = plt.subplots(figsize=(8, 5))
        sns.barplot(data=cnt, x="policy", y="count", ax=ax, palette="Set2")
        ax.set_ylabel("Total deaths")
        ax.set_title("Total deaths by policy")

    save_dual(fig, out_dir, "03_survival_or_deaths_by_policy")


def _decode_role(values: pd.Series) -> pd.Series:
    s = values.astype(str).str.strip().str.upper()
    mapped = pd.Series(index=s.index, dtype=object)
    mapped[s.isin(["CH", "CLUSTER_HEAD", "CLUSTERHEAD", "HEAD", "1", "TRUE"])] = "CH"
    mapped[s.isin(["MEMBER", "MBR", "0", "FALSE"])] = "MEMBER"
    unmapped = mapped.isna()
    mapped.loc[unmapped & s.str.contains("CH")] = "CH"
    mapped.loc[unmapped & (~s.str.contains("CH"))] = "MEMBER"
    return mapped


def build_wait_frame(tables: list[Table]) -> pd.DataFrame:
    wait_cols = ["waiting_time_ms", "wait_time_ms", "queue_wait_ms", "waiting_ms", "wait_time"]
    role_cols = ["role", "uav_role", "node_role", "is_cluster_head", "role_id"]
    out = []
    for t in tables:
        wc = _first_present(t.df.columns, wait_cols)
        rc = _first_present(t.df.columns, role_cols)
        if wc is None or rc is None:
            continue
        part = t.df[["policy"]].copy()
        part["waiting_ms"] = _numeric(t.df[wc])
        part["role"] = _decode_role(t.df[rc])
        part = part[(part["waiting_ms"] >= 0) & part["waiting_ms"].notna()]
        if not part.empty:
            out.append(part)
    if not out:
        raise RuntimeError("No waiting-time samples with role information found.")
    return pd.concat(out, ignore_index=True)


def plot_04_waiting_time_cdf_by_policy(tables: list[Table], out_dir: Path) -> None:
    waits = build_wait_frame(tables)
    fig, ax = plt.subplots(figsize=(10, 6))
    for (policy, role), grp in waits.groupby(["policy", "role"]):
        x = np.sort(grp["waiting_ms"].to_numpy(dtype=float))
        if x.size == 0:
            continue
        y = np.arange(1, x.size + 1) / x.size
        ax.step(x, y, where="post", label=f"{policy} - {role}")

    ax.set_xlabel("Waiting time (ms)")
    ax.set_ylabel("Empirical CDF")
    ax.set_title("Waiting-time ECDF by policy and role (CH vs MEMBER)")
    ax.legend(loc="lower right", ncol=2)

    med = waits.groupby(["policy", "role"])["waiting_ms"].median().sort_values()
    ax.text(0.02, 0.02, "Median wait (ms): " + "; ".join([f"{p}-{r}={v:.1f}" for (p, r), v in med.items()]), transform=ax.transAxes, fontsize=8)
    save_dual(fig, out_dir, "04_waiting_time_cdf_by_policy")


def build_queue_frame(tables: list[Table]) -> pd.DataFrame:
    qcols = ["queue_length", "queue_length_ugv", "queue_len", "queue_length_ch", "queue_value"]
    tcols = ["time", "timestamp", "t", "sim_time"]
    out = []
    for t in tables:
        qc = _first_present(t.df.columns, qcols)
        tc = _first_present(t.df.columns, tcols)
        if qc is None or tc is None:
            continue
        part = t.df[["policy", "run"]].copy()
        part["time"] = _numeric(t.df[tc])
        part["queue"] = _numeric(t.df[qc])
        part = part.dropna(subset=["time", "queue"]).sort_values("time")
        if not part.empty:
            out.append(part)
    if not out:
        raise RuntimeError("No queue length over time data found.")
    return pd.concat(out, ignore_index=True)


def plot_05_queue_length_over_time(tables: list[Table], out_dir: Path) -> None:
    queue = build_queue_frame(tables)
    fractional = np.any(np.abs(queue["queue"] - np.round(queue["queue"])) > 1e-9)

    fig, ax = plt.subplots(figsize=(10, 5))
    for (policy, run), grp in queue.groupby(["policy", "run"]):
        label = policy if run == queue[queue["policy"] == policy]["run"].iloc[0] else None
        ax.step(grp["time"], grp["queue"], where="post", alpha=0.8 if label else 0.3, label=label)

    ylabel = "Mean queue length" if fractional else "Queue length"
    title = "Queue length over time (step plot, raw timestamps)"
    if fractional:
        title = "Mean queue length over time (step plot, raw timestamps)"
    ax.set_xlabel("Time (s)")
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.legend(loc="best")

    by_policy_peak = queue.groupby("policy")["queue"].max().sort_values(ascending=False)
    ax.text(0.02, 0.98, "Peak: " + ", ".join([f"{p}={v:.2f}" for p, v in by_policy_peak.items()]), transform=ax.transAxes, va="top", fontsize=9)
    save_dual(fig, out_dir, "05_queue_length_over_time")


def main() -> None:
    args = parse_args()
    setup_style()
    tables = load_tables(args.input)

    plot_01_overall_pdr_by_policy(tables, args.out)
    plot_02_e2e_delay_distribution_by_policy(tables, args.out)
    plot_03_survival_or_deaths_by_policy(tables, args.out)
    plot_04_waiting_time_cdf_by_policy(tables, args.out)
    plot_05_queue_length_over_time(tables, args.out)


if __name__ == "__main__":
    main()
