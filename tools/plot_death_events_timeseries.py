#!/usr/bin/env python3
"""Plot cumulative death events over mission time by policy.

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

from tools.utils_clean import align_time_seconds, drop_time_resets, normalize_role, safe_numeric
from tools.utils_io import discover_policy_dirs, load_csv_with_hints, require_columns

FONT_SIZE = 12
N_BOOT = 500
RNG = np.random.default_rng(42)
EPS = 1e-6



def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Plot cumulative death events over mission time")
    p.add_argument("--data_root", required=True, type=Path)
    p.add_argument("--out_dir", type=Path, default=Path("analysis/test_round1"))
    p.add_argument("--role", default="ALL", choices=["ALL", "CH", "MEMBER"])
    p.add_argument(
        "--align_mission_time",
        choices=["none", "common_window"],
        default="common_window",
        help="Truncate all policies to a shared mission-time window for fair visual comparison.",
    )
    return p.parse_args()


def bootstrap_ci(values: np.ndarray, n_boot: int = N_BOOT) -> tuple[float, float]:
    if len(values) <= 1:
        return np.nan, np.nan
    stats = np.empty(n_boot, dtype=float)
    for i in range(n_boot):
        sample = RNG.choice(values, size=len(values), replace=True)
        stats[i] = np.nanmean(sample)
    return float(np.nanpercentile(stats, 2.5)), float(np.nanpercentile(stats, 97.5))


def _policy_mission_end(policy_dir: Path) -> float:
    status_csv = policy_dir / "status_timeseries.csv"
    if not status_csv.exists():
        return np.nan
    s = load_csv_with_hints(status_csv)
    require_columns(s, ["time"], status_csv)
    s = safe_numeric(s, ["time"])
    s = align_time_seconds(s, "time")
    s, _ = drop_time_resets(s, "time")
    t = s["time"].dropna()
    return float(t.max()) if not t.empty else np.nan


def _mission_start_by_run(policy_dir: Path) -> dict[str, float]:
    status_csv = policy_dir / "status_timeseries.csv"
    if not status_csv.exists():
        return {}
    s = load_csv_with_hints(status_csv, dtype_hints={"run_id": "string"})
    require_columns(s, ["time"], status_csv)
    s = safe_numeric(s, ["time"])
    s = s.dropna(subset=["time"]).copy()
    if s.empty:
        return {}
    if "run_id" in s.columns:
        starts = s.groupby("run_id", dropna=False)["time"].min()
        return {str(k): float(v) for k, v in starts.items() if pd.notna(v)}
    return {"__single__": float(s["time"].min())}


def _prepare_events(policy_dir: Path, role: str) -> pd.DataFrame:
    d_path = policy_dir / "death_events.csv"
    d = load_csv_with_hints(d_path, dtype_hints={"run_id": "string"})
    require_columns(d, ["time"], d_path)
    d = safe_numeric(d, ["time"])

    starts = _mission_start_by_run(policy_dir)
    if not starts:
        raise ValueError("status_timeseries.csv missing/empty; cannot map death times to mission time")

    if "run_id" in d.columns:
        d["_run_key"] = d["run_id"].astype(str)
        d["_start_time"] = d["_run_key"].map(starts)
    else:
        fallback = starts.get("__single__", min(starts.values()))
        d["_start_time"] = fallback
        d["run_id"] = policy_dir.name

    d["time"] = d["time"] - d["_start_time"]
    d = d.dropna(subset=["time"]).copy()
    d = d[d["time"] >= 0].copy()

    d, dropped = drop_time_resets(d, "time")
    if dropped:
        print(f"WARNING [{policy_dir.name}]: dropped {dropped} rows with decreasing time", file=sys.stderr)

    if role != "ALL":
        require_columns(d, ["role"], d_path)
        d["role_label"] = normalize_role(d["role"])
        target = "CH" if role == "CH" else "member"
        d = d[d["role_label"] == target].copy()

    # Ensure curves always start from (t=0, y=0); move death instants infinitesimally to the right.
    d["time"] = d["time"] + EPS

    return d


def main() -> int:
    args = parse_args()
    plt.rcParams.update({"font.size": FONT_SIZE, "axes.labelsize": FONT_SIZE, "legend.fontsize": FONT_SIZE - 1})

    fig_name = f"death_events_timeseries_{args.role.lower()}"
    out = args.out_dir / fig_name
    out.mkdir(parents=True, exist_ok=True)

    try:
        policies = discover_policy_dirs(args.data_root)
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    global_end = np.nan
    policy_window_end: dict[str, float] = {}
    if args.align_mission_time == "common_window":
        ends = []
        for p in policies:
            try:
                end_t = _policy_mission_end(p)
                if pd.notna(end_t):
                    policy_window_end[p.name] = float(end_t)
                    ends.append(float(end_t))
            except Exception as exc:
                print(f"WARNING [{p.name}]: failed to estimate mission end time: {exc}", file=sys.stderr)
        if not ends:
            print("ERROR: could not infer mission end times for alignment", file=sys.stderr)
            return 1
        global_end = float(min(ends))
        print(f"INFO: using common mission-time end = {global_end:.2f}s", file=sys.stderr)

    fig, ax = plt.subplots(figsize=(10, 5))
    table_rows: list[dict] = []

    for policy_dir in policies:
        try:
            d = _prepare_events(policy_dir, args.role)
        except Exception as exc:
            print(f"ERROR [{policy_dir.name}]: {exc}", file=sys.stderr)
            return 1

        if args.align_mission_time == "common_window":
            d = d[d["time"] <= global_end].copy()
            end_report = policy_window_end.get(policy_dir.name, np.nan)
            if pd.notna(end_report):
                print(
                    f"INFO [{policy_dir.name}]: truncation window policy_end={end_report:.2f}s, effective_end={global_end:.2f}s",
                    file=sys.stderr,
                )

        if d.empty:
            table_rows.append(
                {
                    "policy": policy_dir.name,
                    "runs": 0,
                    "mean_final_death_events": 0.0,
                    "std_final_death_events": 0.0,
                    "truncation_applied": args.align_mission_time == "common_window",
                    "truncation_cutoff_s": global_end if args.align_mission_time == "common_window" else np.nan,
                }
            )
            ax.plot([0.0], [0.0], label=policy_dir.name)
            continue

        if "run_id" not in d.columns:
            d["run_id"] = policy_dir.name

        d = d.sort_values(["run_id", "time"]).copy()
        d["event"] = 1
        d["cum_deaths"] = d.groupby("run_id", dropna=False)["event"].cumsum()

        all_runs = sorted(d["run_id"].dropna().astype(str).unique().tolist())
        plot_end = global_end if args.align_mission_time == "common_window" else float(d["time"].max())

        run_curves = []
        for run in all_runs:
            part = d[d["run_id"].astype(str) == run][["time", "cum_deaths"]].sort_values("time")
            times = np.concatenate(([0.0], part["time"].to_numpy(dtype=float), [plot_end]))
            vals = np.concatenate(([0.0], part["cum_deaths"].to_numpy(dtype=float), [part["cum_deaths"].iloc[-1]]))
            run_curves.append(pd.DataFrame({"run_id": run, "time": times, "cum_deaths": vals}))

        interp = pd.concat(run_curves, ignore_index=True)
        mean_curve = interp.groupby("time", as_index=False)["cum_deaths"].mean().sort_values("time")

        ci_rows = []
        for t, part in interp.groupby("time"):
            lo, hi = bootstrap_ci(part["cum_deaths"].to_numpy(dtype=float))
            ci_rows.append((t, lo, hi))
        ci = pd.DataFrame(ci_rows, columns=["time", "lo", "hi"]).sort_values("time")

        ax.plot(mean_curve["time"], mean_curve["cum_deaths"], label=policy_dir.name)
        ax.fill_between(ci["time"], ci["lo"], ci["hi"], alpha=0.2)

        finals = interp.groupby("run_id", as_index=False)["cum_deaths"].max()["cum_deaths"].to_numpy(dtype=float)
        table_rows.append(
            {
                "policy": policy_dir.name,
                "runs": int(len(finals)),
                "mean_final_death_events": float(np.mean(finals)) if len(finals) else 0.0,
                "std_final_death_events": float(np.std(finals, ddof=0)) if len(finals) else 0.0,
                "truncation_applied": args.align_mission_time == "common_window",
                "truncation_cutoff_s": global_end if args.align_mission_time == "common_window" else np.nan,
            }
        )

    role_title = {"ALL": "all UAV", "CH": "CH only", "MEMBER": "member only"}[args.role]
    ax.set_xlabel("Mission time (s)")
    ax.set_ylabel("Cumulative death events (count)")

    title_suffix = " (common window)" if args.align_mission_time == "common_window" else ""
    ax.set_title(f"Cumulative death events over mission time ({role_title}){title_suffix}")
    ax.grid(alpha=0.3)
    ax.legend(loc="best")
    fig.tight_layout()

    png = out / f"{fig_name}.png"
    pdf = out / f"{fig_name}.pdf"
    svg = out / f"{fig_name}.svg"
    fig.savefig(png, dpi=200)
    fig.savefig(pdf)
    fig.savefig(svg)
    plt.close(fig)

    summary = pd.DataFrame(table_rows).sort_values("mean_final_death_events")
    summary.to_csv(out / "death_events_summary.csv", index=False)

    print(f"Saved: {png}")
    print(f"Saved: {pdf}")
    print(f"Saved: {svg}")
    print(f"Saved: {out / 'death_events_summary.csv'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
