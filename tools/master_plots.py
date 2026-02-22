#!/usr/bin/env python3
"""Generate master comparison plots/statistics for UAV-UGV charging experiments.

Designed for network_monitor outputs under each run directory, e.g.:
  <log_root>/<run_id>/charge_events.csv
  <log_root>/<run_id>/death_events.csv
  <log_root>/<run_id>/charge_queue_timeseries.csv
  <log_root>/<run_id>/network_timeseries.csv
  <log_root>/<run_id>/weather_timeseries.csv
  <log_root>/<run_id>/status_timeseries.csv
"""

from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

try:
    from scipy import stats  # type: ignore
except Exception:  # pragma: no cover
    stats = None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Master visualization + stats pipeline")
    parser.add_argument("--log-root", type=Path, default=Path("log"))
    parser.add_argument("--output-dir", type=Path, default=Path("analysis/master_plots"))
    parser.add_argument("--protocol-map", type=Path, default=None,
                        help="Optional CSV with columns run_id,protocol")
    parser.add_argument("--run-ids", nargs="*", default=None)
    parser.add_argument("--time-bin-sec", type=int, default=5)
    return parser.parse_args()


def clean_output_dir(output_dir: Path) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    for p in output_dir.glob("*.png"):
        p.unlink(missing_ok=True)
    for p in output_dir.glob("*.csv"):
        p.unlink(missing_ok=True)
    for p in output_dir.glob("*.json"):
        p.unlink(missing_ok=True)


def load_protocol_map(path: Optional[Path]) -> Dict[str, str]:
    if path is None or not path.exists():
        return {}
    result: Dict[str, str] = {}
    with path.open(newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            run_id = (row.get("run_id") or "").strip()
            protocol = (row.get("protocol") or "").strip()
            if run_id and protocol:
                result[run_id] = protocol
    return result


def find_runs(log_root: Path, run_ids: Optional[List[str]]) -> Iterable[Path]:
    if run_ids:
        for run_id in run_ids:
            yield log_root / run_id
        return
    if not log_root.exists():
        return
    for p in sorted(log_root.iterdir()):
        if p.is_dir():
            yield p


def read_metadata_protocol(run_path: Path) -> Optional[str]:
    for name in ("metadata.json", "metadata.csv", "metadata.yaml", "metadata.yml", "metadata.txt"):
        p = run_path / name
        if not p.exists():
            continue
        if p.suffix == ".json":
            try:
                data = json.loads(p.read_text())
            except Exception:
                continue
            for k in ("protocol", "decision_policy", "charging_policy"):
                if data.get(k):
                    return str(data[k]).strip()
        elif p.suffix == ".csv":
            with p.open(newline="") as f:
                reader = csv.DictReader(f)
                for row in reader:
                    for k in ("protocol", "decision_policy", "charging_policy"):
                        v = (row.get(k) or "").strip()
                        if v:
                            return v
                    break
        else:
            for line in p.read_text(errors="ignore").splitlines():
                if ":" not in line:
                    continue
                k, v = [x.strip() for x in line.split(":", 1)]
                if k in {"protocol", "decision_policy", "charging_policy"} and v:
                    return v
    return None


def infer_protocol(run_id: str, run_path: Path, protocol_map: Dict[str, str]) -> str:
    if run_id in protocol_map:
        return protocol_map[run_id]
    md = read_metadata_protocol(run_path)
    if md:
        return md
    name = run_id.lower()
    if "role" in name and "priority" in name:
        return "Role-Priority"
    if "edf" in name:
        return "EDF"
    if "fcfs" in name:
        return "FCFS"
    return run_id


def read_optional_csv(path: Path) -> pd.DataFrame:
    if not path.exists():
        return pd.DataFrame()
    frame = pd.read_csv(path)
    return frame


def load_frames(log_root: Path, run_ids: Optional[List[str]], protocol_map: Dict[str, str]) -> Dict[str, pd.DataFrame]:
    bundle: Dict[str, List[pd.DataFrame]] = {
        "charge_events": [],
        "death_events": [],
        "queue": [],
        "network": [],
        "weather": [],
        "status": [],
    }

    for run_path in find_runs(log_root, run_ids):
        charge = read_optional_csv(run_path / "charge_events.csv")
        if charge.empty:
            continue
        run_id = str(charge.get("run_id", pd.Series([run_path.name])).iloc[0])
        protocol = infer_protocol(run_id, run_path, protocol_map)

        files = {
            "charge_events": charge,
            "death_events": read_optional_csv(run_path / "death_events.csv"),
            "queue": read_optional_csv(run_path / "charge_queue_timeseries.csv"),
            "network": read_optional_csv(run_path / "network_timeseries.csv"),
            "weather": read_optional_csv(run_path / "weather_timeseries.csv"),
            "status": read_optional_csv(run_path / "status_timeseries.csv"),
        }
        for _, frame in files.items():
            if frame.empty:
                continue
            if "run_id" not in frame.columns:
                frame["run_id"] = run_id
            frame["protocol"] = protocol

        for k, frame in files.items():
            if not frame.empty:
                bundle[k].append(frame)

    return {k: (pd.concat(v, ignore_index=True) if v else pd.DataFrame()) for k, v in bundle.items()}


def to_num(s: pd.Series) -> pd.Series:
    return pd.to_numeric(s, errors="coerce")


def plot_km_survival(death_events: pd.DataFrame, status: pd.DataFrame, output_dir: Path) -> None:
    if death_events.empty:
        return

    # Build per-run event times and censor times
    if "fleet_size" not in death_events.columns:
        return
    death_events = death_events.copy()
    death_events["time"] = to_num(death_events["time"])

    records = []
    for (protocol, run_id), grp in death_events.groupby(["protocol", "run_id"]):
        fleet = int(pd.to_numeric(grp["fleet_size"], errors="coerce").dropna().max() or 0)
        if fleet <= 0:
            continue
        death_times = sorted(grp["time"].dropna().tolist())

        if not status.empty and {"run_id", "time"}.issubset(status.columns):
            ctime = to_num(status[status["run_id"] == run_id]["time"]).max()
            if np.isnan(ctime):
                ctime = max(death_times) if death_times else 0.0
        else:
            ctime = max(death_times) if death_times else 0.0

        for dt in death_times:
            records.append((protocol, run_id, float(dt), 1))
        for _ in range(max(0, fleet - len(death_times))):
            records.append((protocol, run_id, float(ctime), 0))

    if not records:
        return

    km = pd.DataFrame(records, columns=["protocol", "run_id", "time", "event"])

    plt.figure(figsize=(10, 6))
    for protocol, g in km.groupby("protocol"):
        g = g.sort_values("time")
        n = len(g)
        if n == 0:
            continue
        at_risk = n
        surv = 1.0
        xs, ys = [0.0], [1.0]
        for t, tg in g.groupby("time"):
            d = int((tg["event"] == 1).sum())
            c = int((tg["event"] == 0).sum())
            if at_risk > 0 and d > 0:
                surv *= (1.0 - d / at_risk)
                xs.append(float(t))
                ys.append(surv)
            at_risk -= (d + c)
        plt.step(xs, ys, where="post", label=protocol)

    plt.xlabel("Simulation Time (s)")
    plt.ylabel("Survival Probability")
    plt.title("Swarm Survivability (Kaplan-Meier)")
    plt.ylim(0, 1.02)
    plt.legend()
    plt.tight_layout()
    plt.savefig(output_dir / "01_km_survivability.png", dpi=180)
    plt.close()


def plot_fairness_boxplot(charge_events: pd.DataFrame, output_dir: Path) -> None:
    if charge_events.empty or "effective_wait_ms" not in charge_events.columns:
        return
    frame = charge_events.copy()
    frame["wait_ms"] = to_num(frame["effective_wait_ms"])  # use terminal-aware wait
    frame = frame[frame["wait_ms"] >= 0]
    if frame.empty:
        return

    role = to_num(frame.get("role", pd.Series([-1] * len(frame))))
    frame["role_name"] = np.where(role == 1, "CH", "Member")

    protocols = list(frame["protocol"].dropna().unique())
    fig, ax = plt.subplots(figsize=(12, 6))
    width = 0.35

    ch_data, mb_data, ticks = [], [], []
    for i, p in enumerate(protocols):
        gp = frame[frame["protocol"] == p]
        ch_data.append(gp[gp["role_name"] == "CH"]["wait_ms"].dropna().values)
        mb_data.append(gp[gp["role_name"] == "Member"]["wait_ms"].dropna().values)
        ticks.append(i)

    bp1 = ax.boxplot(ch_data, positions=np.array(ticks) - width / 2, widths=0.3, patch_artist=True, showfliers=False)
    bp2 = ax.boxplot(mb_data, positions=np.array(ticks) + width / 2, widths=0.3, patch_artist=True, showfliers=False)
    for b in bp1["boxes"]:
        b.set_facecolor("#4C78A8")
    for b in bp2["boxes"]:
        b.set_facecolor("#F58518")

    ax.set_xticks(ticks)
    ax.set_xticklabels(protocols, rotation=25)
    ax.set_ylabel("Waiting Time (ms)")
    ax.set_xlabel("Scheduling Strategy")
    ax.set_title("Fairness & Starvation: CH vs Member Waiting Time")
    ax.legend([bp1["boxes"][0], bp2["boxes"][0]], ["CH", "Member"], loc="upper right")
    plt.tight_layout()
    plt.savefig(output_dir / "02_fairness_boxplot.png", dpi=180)
    plt.close()


def plot_queue_dynamics(queue: pd.DataFrame, output_dir: Path) -> None:
    if queue.empty:
        return
    needed = {"time", "active_charging", "queue_length", "active_mission_count"}
    if not needed.issubset(queue.columns):
        return

    queue = queue.copy()
    for col in ("time", "active_charging", "queue_length", "active_mission_count"):
        queue[col] = to_num(queue[col])

    for protocol, g in queue.groupby("protocol"):
        g = g.sort_values("time")
        plt.figure(figsize=(11, 6))
        plt.stackplot(
            g["time"],
            g["active_charging"].clip(lower=0),
            g["queue_length"].clip(lower=0),
            g["active_mission_count"].clip(lower=0),
            labels=["Charging", "Queued", "On Mission"],
            alpha=0.85,
        )
        plt.xlabel("Simulation Time (s)")
        plt.ylabel("UAV Count")
        plt.title(f"Queue Dynamics - {protocol}")
        plt.legend(loc="upper left")
        plt.tight_layout()
        safe = str(protocol).replace("/", "_").replace(" ", "_")
        plt.savefig(output_dir / f"03_queue_dynamics_{safe}.png", dpi=180)
        plt.close()


def plot_weather_sensitivity(network: pd.DataFrame, weather: pd.DataFrame, output_dir: Path) -> None:
    if network.empty or weather.empty:
        return
    if not {"run_id", "time", "window_pdr"}.issubset(network.columns):
        return
    if not {"run_id", "time", "wind_speed"}.issubset(weather.columns):
        return

    n = network[["run_id", "protocol", "time", "window_pdr"]].copy()
    w = weather[["run_id", "time", "wind_speed"]].copy()
    n["time"] = to_num(n["time"])
    n["window_pdr"] = to_num(n["window_pdr"])
    w["time"] = to_num(w["time"])
    w["wind_speed"] = to_num(w["wind_speed"])

    merged = pd.merge_asof(
        n.sort_values("time"),
        w.sort_values("time"),
        on="time",
        by="run_id",
        direction="nearest",
        tolerance=5.0,
    ).dropna(subset=["wind_speed", "window_pdr"])
    if merged.empty:
        return

    merged["wind_bin"] = pd.cut(merged["wind_speed"], bins=10)
    heat = merged.pivot_table(index="protocol", columns="wind_bin", values="window_pdr", aggfunc="mean")

    plt.figure(figsize=(12, 4))
    plt.imshow(heat.values, aspect="auto", cmap="viridis", vmin=0, vmax=1)
    plt.colorbar(label="Mean PDR")
    plt.yticks(range(len(heat.index)), heat.index)
    plt.xticks(range(len(heat.columns)), [str(c) for c in heat.columns], rotation=30, ha="right")
    plt.xlabel("Wind Speed Bin")
    plt.ylabel("Strategy")
    plt.title("Weather Sensitivity Heatmap: Wind vs PDR")
    plt.tight_layout()
    plt.savefig(output_dir / "04_weather_heatmap.png", dpi=180)
    plt.close()


def plot_control_loop_latency(network: pd.DataFrame, charge_events: pd.DataFrame, output_dir: Path) -> None:
    if network.empty or charge_events.empty:
        return
    if not {"run_id", "time", "window_delay_mean_ms"}.issubset(network.columns):
        return
    if not {"run_id", "decision_time", "charge_completed"}.issubset(charge_events.columns):
        return

    n = network[["run_id", "protocol", "time", "window_delay_mean_ms"]].copy()
    c = charge_events[["run_id", "decision_time", "charge_completed"]].copy()
    n["time"] = to_num(n["time"])
    n["window_delay_mean_ms"] = to_num(n["window_delay_mean_ms"])
    c["decision_time"] = to_num(c["decision_time"])
    c["success"] = c["charge_completed"].astype(str).str.lower().isin(["true", "1", "yes"])

    c["time_bin"] = (c["decision_time"] // 5) * 5
    c_agg = c.groupby(["run_id", "time_bin"]).agg(decision_success_rate=("success", "mean")).reset_index()

    n = n.rename(columns={"time": "time_bin"})
    merged = pd.merge(n, c_agg, on=["run_id", "time_bin"], how="inner")
    if merged.empty:
        return

    fig, ax1 = plt.subplots(figsize=(11, 6))
    ax2 = ax1.twinx()

    g = merged.groupby("time_bin").agg(
        delay_ms=("window_delay_mean_ms", "mean"),
        success=("decision_success_rate", "mean"),
    ).reset_index().sort_values("time_bin")

    ax1.plot(g["time_bin"], g["delay_ms"], color="#E45756", label="Network Delay")
    ax2.plot(g["time_bin"], g["success"], color="#4C78A8", label="Decision Success Rate")

    ax1.set_xlabel("Simulation Time (s)")
    ax1.set_ylabel("Delay (ms)", color="#E45756")
    ax2.set_ylabel("Decision Success Rate", color="#4C78A8")
    ax2.set_ylim(0, 1.02)
    ax1.set_title("Control-Loop Latency vs Charging Decision Success")

    lines = ax1.get_lines() + ax2.get_lines()
    ax1.legend(lines, [l.get_label() for l in lines], loc="upper right")
    plt.tight_layout()
    plt.savefig(output_dir / "05_control_loop_latency.png", dpi=180)
    plt.close()


def compute_statistics(charge_events: pd.DataFrame, queue: pd.DataFrame, status: pd.DataFrame, output_dir: Path) -> None:
    stats_rows = []

    ce = charge_events.copy()
    ce["effective_wait_ms"] = to_num(ce.get("effective_wait_ms", pd.Series(dtype=float)))
    ce = ce[ce["effective_wait_ms"] >= 0]

    # Kruskal-Wallis
    kw_result = {"available": bool(stats is not None), "p_value": None, "H": None}
    if stats is not None and not ce.empty:
        groups = [g["effective_wait_ms"].dropna().values for _, g in ce.groupby("protocol")]
        groups = [g for g in groups if len(g) > 0]
        if len(groups) >= 2:
            h, p = stats.kruskal(*groups)
            kw_result.update({"p_value": float(p), "H": float(h)})

    # UGV utilization
    util = pd.DataFrame()
    if not queue.empty and {"protocol", "ugv_dock_utilization"}.issubset(queue.columns):
        q = queue.copy()
        q["ugv_dock_utilization"] = to_num(q["ugv_dock_utilization"])
        util = q.groupby("protocol").agg(
            avg_ugv_utilization=("ugv_dock_utilization", "mean"),
            idle_time_ratio=("ugv_dock_utilization", lambda s: float((s < 1e-6).mean())),
        ).reset_index()

    # Energy efficiency ratio = monitoring duration / total energy consumed
    eff = pd.DataFrame(columns=["protocol", "energy_efficiency_ratio"])
    if not status.empty and {"protocol", "time", "energy_consumption_rate", "uav_id", "run_id"}.issubset(status.columns):
        s = status.copy()
        s["time"] = to_num(s["time"])
        s["energy_consumption_rate"] = to_num(s["energy_consumption_rate"]).clip(lower=0)
        s = s.sort_values(["run_id", "uav_id", "time"])
        s["dt"] = s.groupby(["run_id", "uav_id"]) ["time"].diff().fillna(0).clip(lower=0)
        s["energy_used"] = s["energy_consumption_rate"] * s["dt"]

        duration = s.groupby(["protocol", "run_id"]).agg(
            monitoring_duration=("time", lambda x: float(np.nanmax(x) - np.nanmin(x)))
        ).reset_index()
        energy = s.groupby(["protocol", "run_id"]).agg(total_energy=("energy_used", "sum")).reset_index()
        e = pd.merge(duration, energy, on=["protocol", "run_id"], how="inner")
        e["energy_efficiency_ratio"] = e["monitoring_duration"] / e["total_energy"].replace(0, np.nan)
        eff = e.groupby("protocol").agg(energy_efficiency_ratio=("energy_efficiency_ratio", "mean")).reset_index()

    protocols = sorted(set(charge_events.get("protocol", pd.Series(dtype=str)).dropna().tolist()))
    for p in protocols:
        row = {"protocol": p}
        if not util.empty and (util["protocol"] == p).any():
            ur = util[util["protocol"] == p].iloc[0]
            row["avg_ugv_utilization"] = ur["avg_ugv_utilization"]
            row["idle_time_ratio"] = ur["idle_time_ratio"]
        if not eff.empty and (eff["protocol"] == p).any():
            er = eff[eff["protocol"] == p].iloc[0]
            row["energy_efficiency_ratio"] = er["energy_efficiency_ratio"]
        stats_rows.append(row)

    pd.DataFrame(stats_rows).to_csv(output_dir / "advanced_statistics_summary.csv", index=False)
    (output_dir / "kruskal_wallis_waiting_time.json").write_text(json.dumps(kw_result, indent=2))


def write_data_quality_warnings(charge_events: pd.DataFrame, output_dir: Path) -> None:
    warnings = []

    if "waiting_time_ms" in charge_events.columns:
        w = to_num(charge_events["waiting_time_ms"])
        neg = int((w < 0).sum())
        warnings.append({
            "check": "negative_waiting_time_ms",
            "count": neg,
            "message": "If count>0, align request timestamp to msg.header.stamp (simulation send time).",
        })

    if "failure_reason" in charge_events.columns:
        fr = charge_events["failure_reason"].astype(str)
        timeout_n = int(fr.str.contains("TIMEOUT", case=False, na=False).sum())
        routing_drop_n = int(fr.str.contains("ROUTING_DROP", case=False, na=False).sum())
        warnings.append({
            "check": "failure_attribution",
            "TIMEOUT_count": timeout_n,
            "ROUTING_DROP_count": routing_drop_n,
            "message": "Keep TIMEOUT vs ROUTING_DROP separated in all analyses.",
        })

    pd.DataFrame(warnings).to_csv(output_dir / "critical_warnings_checks.csv", index=False)


def main() -> None:
    args = parse_args()
    clean_output_dir(args.output_dir)

    protocol_map = load_protocol_map(args.protocol_map)
    data = load_frames(args.log_root, args.run_ids, protocol_map)

    charge = data["charge_events"]
    if charge.empty:
        raise SystemExit("No run with charge_events.csv found.")

    plot_km_survival(data["death_events"], data["status"], args.output_dir)
    plot_fairness_boxplot(charge, args.output_dir)
    plot_queue_dynamics(data["queue"], args.output_dir)
    plot_weather_sensitivity(data["network"], data["weather"], args.output_dir)
    plot_control_loop_latency(data["network"], charge, args.output_dir)

    compute_statistics(charge, data["queue"], data["status"], args.output_dir)
    write_data_quality_warnings(charge, args.output_dir)

    print(f"Generated outputs in: {args.output_dir}")


if __name__ == "__main__":
    main()
