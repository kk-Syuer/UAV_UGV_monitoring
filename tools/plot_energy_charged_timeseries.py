#!/usr/bin/env python3
"""Plot cumulative charged energy over mission time by policy.

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
    p = argparse.ArgumentParser(description="Plot cumulative charged energy over mission time")
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


def _prepare_charge_events(policy_dir: Path, role: str) -> pd.DataFrame:
    c_path = policy_dir / "charge_events.csv"
    c = load_csv_with_hints(c_path, dtype_hints={"run_id": "string"})
    require_columns(c, ["energy_recovered"], c_path)

    # Prefer charge_end_time for energy completion timestamp; fallback to terminal_time.
    if "charge_end_time" in c.columns:
        c = c.rename(columns={"charge_end_time": "time"})
    elif "terminal_time" in c.columns:
        c = c.rename(columns={"terminal_time": "time"})
    else:
        raise ValueError("charge_events.csv requires charge_end_time or terminal_time to build timeseries")

    c = safe_numeric(c, ["time", "energy_recovered"])
    c = align_time_seconds(c, "time")
    c, dropped = drop_time_resets(c, "time")
    if dropped:
        print(f"WARNING [{policy_dir.name}]: dropped {dropped} rows with decreasing time", file=sys.stderr)

    c = c.dropna(subset=["time", "energy_recovered"]).copy()
    c = c[c["energy_recovered"] > 0].copy()
    # Ensure curves always start from (t=0, y=0); move charging instants infinitesimally to the right.
    c["time"] = c["time"].clip(lower=0) + EPS

    if role != "ALL":
        require_columns(c, ["role"], c_path)
        c["role_label"] = normalize_role(c["role"])
        target = "CH" if role == "CH" else "member"
        c = c[c["role_label"] == target].copy()

    if "charge_completed" in c.columns:
        # Keep completed sessions only when this signal exists.
        completed = c["charge_completed"].astype(str).str.lower().isin(["true", "1", "yes"])
        c = c[completed].copy()

    return c


def main() -> int:
    args = parse_args()
    plt.rcParams.update({"font.size": FONT_SIZE, "axes.labelsize": FONT_SIZE, "legend.fontsize": FONT_SIZE - 1})

    fig_name = f"energy_charged_timeseries_{args.role.lower()}"
    out = args.out_dir / fig_name
    out.mkdir(parents=True, exist_ok=True)

    try:
        policies = discover_policy_dirs(args.data_root)
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    global_end = np.nan
    if args.align_mission_time == "common_window":
        ends = []
        for p in policies:
            try:
                end_t = _policy_mission_end(p)
                if pd.notna(end_t):
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
            c = _prepare_charge_events(policy_dir, args.role)
        except Exception as exc:
            print(f"ERROR [{policy_dir.name}]: {exc}", file=sys.stderr)
            return 1

        if args.align_mission_time == "common_window":
            c = c[c["time"] <= global_end].copy()

        if c.empty:
            table_rows.append({"policy": policy_dir.name, "runs": 0, "mean_final_energy_recovered_pctpt": 0.0, "std_final_energy_recovered_pctpt": 0.0})
            ax.plot([0.0], [0.0], label=policy_dir.name)
            continue

        if "run_id" not in c.columns:
            c["run_id"] = policy_dir.name

        c = c.sort_values(["run_id", "time"]).copy()
        c["cum_energy"] = c.groupby("run_id", dropna=False)["energy_recovered"].cumsum()

        all_runs = sorted(c["run_id"].dropna().astype(str).unique().tolist())
        plot_end = global_end if args.align_mission_time == "common_window" else float(c["time"].max())

        run_curves = []
        for run in all_runs:
            part = c[c["run_id"].astype(str) == run][["time", "cum_energy"]].sort_values("time")
            times = np.concatenate(([0.0], part["time"].to_numpy(dtype=float), [plot_end]))
            vals = np.concatenate(([0.0], part["cum_energy"].to_numpy(dtype=float), [part["cum_energy"].iloc[-1]]))
            run_curves.append(pd.DataFrame({"run_id": run, "time": times, "cum_energy": vals}))

        interp = pd.concat(run_curves, ignore_index=True)
        mean_curve = interp.groupby("time", as_index=False)["cum_energy"].mean().sort_values("time")

        ci_rows = []
        for t, part in interp.groupby("time"):
            lo, hi = bootstrap_ci(part["cum_energy"].to_numpy(dtype=float))
            ci_rows.append((t, lo, hi))
        ci = pd.DataFrame(ci_rows, columns=["time", "lo", "hi"]).sort_values("time")

        ax.plot(mean_curve["time"], mean_curve["cum_energy"], label=policy_dir.name)
        ax.fill_between(ci["time"], ci["lo"], ci["hi"], alpha=0.2)

        finals = interp.groupby("run_id", as_index=False)["cum_energy"].max()["cum_energy"].to_numpy(dtype=float)
        table_rows.append(
            {
                "policy": policy_dir.name,
                "runs": int(len(finals)),
                "mean_final_energy_recovered_pctpt": float(np.mean(finals)) if len(finals) else 0.0,
                "std_final_energy_recovered_pctpt": float(np.std(finals, ddof=0)) if len(finals) else 0.0,
            }
        )

    role_title = {"ALL": "all UAV", "CH": "CH only", "MEMBER": "member only"}[args.role]
    ax.set_xlabel("Mission time (s)")
    ax.set_ylabel("Cumulative charged energy (battery percentage-points, %pt)")
    ax.set_title(f"Cumulative charged energy over mission time ({role_title})")
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

    summary = pd.DataFrame(table_rows).sort_values("mean_final_energy_recovered_pctpt", ascending=False)
    summary.to_csv(out / "energy_charged_summary.csv", index=False)

    print(f"Saved: {png}")
    print(f"Saved: {pdf}")
    print(f"Saved: {svg}")
    print(f"Saved: {out / 'energy_charged_summary.csv'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
