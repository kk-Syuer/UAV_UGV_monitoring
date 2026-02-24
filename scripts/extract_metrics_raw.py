#!/usr/bin/env python3
"""Extract raw per-run metrics from OUTPUT_ROOT policy/run folders."""

from __future__ import annotations

import argparse
import csv
import math
from collections import defaultdict
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple

MESSAGE_FILE = "messages.csv"
QUEUE_FILE = "charge_queue_timeseries.csv"
DEATH_FILE = "death_events.csv"
CHARGE_FILE = "charge_events.csv"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Extract raw run-level metrics and tidy CSVs")
    parser.add_argument("--input", required=True, help="Root directory containing policy/run folders")
    parser.add_argument("--out", required=True, help="Output directory for tidy CSVs and contract")
    return parser.parse_args()


def iter_policy_runs(input_root: Path) -> List[Tuple[str, str, Path]]:
    runs: List[Tuple[str, str, Path]] = []
    for policy_dir in sorted([p for p in input_root.iterdir() if p.is_dir()]):
        for run_dir in sorted([r for r in policy_dir.iterdir() if r.is_dir()]):
            runs.append((policy_dir.name, run_dir.name, run_dir))
    return runs


def read_csv(path: Path) -> List[Dict[str, str]]:
    with path.open("r", newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def str_is_true(value: Optional[str]) -> bool:
    if value is None:
        return False
    return value.strip().lower() in {"1", "true", "t", "yes", "y"}


def parse_float(value: Optional[str]) -> Optional[float]:
    if value is None:
        return None
    s = value.strip()
    if s == "":
        return None
    try:
        return float(s)
    except ValueError:
        return None


def parse_time(row: Dict[str, str], candidates: Sequence[str]) -> Optional[float]:
    for key in candidates:
        if key in row:
            parsed = parse_float(row.get(key))
            if parsed is not None:
                return parsed
    return None


def write_csv(path: Path, rows: Iterable[Dict[str, object]], fieldnames: Sequence[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow(row)


def corr(xs: List[float], ys: List[float]) -> Optional[float]:
    if len(xs) < 2 or len(ys) < 2 or len(xs) != len(ys):
        return None
    mx = sum(xs) / len(xs)
    my = sum(ys) / len(ys)
    cov = sum((x - mx) * (y - my) for x, y in zip(xs, ys))
    vx = sum((x - mx) ** 2 for x in xs)
    vy = sum((y - my) ** 2 for y in ys)
    if vx <= 0 or vy <= 0:
        return None
    return cov / math.sqrt(vx * vy)


def run_linearity_check(series: List[Tuple[float, float]]) -> Optional[float]:
    ordered = sorted(series, key=lambda x: x[0])
    xs = [t for t, _ in ordered]
    ys = [v for _, v in ordered]
    return corr(xs, ys)


def main() -> None:
    args = parse_args()
    input_root = Path(args.input)
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    runs = iter_policy_runs(input_root)

    contract_rows: Dict[str, Dict[str, object]] = {
        MESSAGE_FILE: {"present_runs": 0, "columns": set(), "semantics": "Per-message outcomes; delivered flag and optional type labels + delays."},
        QUEUE_FILE: {"present_runs": 0, "columns": set(), "semantics": "Queue-length timeseries for charge scheduling over simulation time."},
        DEATH_FILE: {"present_runs": 0, "columns": set(), "semantics": "Failure/death event log with UAV identity and event time."},
        CHARGE_FILE: {"present_runs": 0, "columns": set(), "semantics": "Charging sessions with request/dock/end timing and battery/energy fields."},
    }

    messages_tidy: List[Dict[str, object]] = []
    pdr_overall_rows: List[Dict[str, object]] = []
    delay_rows: List[Dict[str, object]] = []
    death_rows: List[Dict[str, object]] = []
    wait_rows: List[Dict[str, object]] = []
    throughput_rows: List[Dict[str, object]] = []
    queue_rows: List[Dict[str, object]] = []

    run_pdr_by_cat: Dict[Tuple[str, str], Dict[str, Tuple[int, int]]] = {}
    categories_seen: set[str] = set()

    pdr_zero_denominator_bins = 0
    non_integer_queue_columns: set[str] = set()
    linearity_failures: List[str] = []

    for policy, run, run_path in runs:
        files = {f.name: f for f in run_path.iterdir() if f.is_file() and f.suffix.lower() == ".csv"}

        messages_path = files.get(MESSAGE_FILE)
        if messages_path:
            rows = read_csv(messages_path)
            contract_rows[MESSAGE_FILE]["present_runs"] += 1
            contract_rows[MESSAGE_FILE]["columns"].update(rows[0].keys() if rows else [])

            delivered_count = 0
            type_counts: Dict[str, List[int]] = defaultdict(lambda: [0, 0])

            for i, row in enumerate(rows):
                delivered = str_is_true(row.get("delivered"))
                delivered_count += int(delivered)

                message_type = row.get("control_type") or row.get("msg_type") or "__all__"
                message_type = message_type.strip() if isinstance(message_type, str) else "__all__"
                if message_type:
                    categories_seen.add(message_type)
                    type_counts[message_type][0] += int(delivered)
                    type_counts[message_type][1] += 1

                delay = parse_float(row.get("e2e_delay_ms"))
                if delay is not None:
                    delay_rows.append(
                        {
                            "policy": policy,
                            "run": run,
                            "message_index": i,
                            "delay_ms": delay,
                        }
                    )

                messages_tidy.append(
                    {
                        "policy": policy,
                        "run": run,
                        "message_index": i,
                        "delivered": delivered,
                        "control_type": row.get("control_type", ""),
                        "msg_type": row.get("msg_type", ""),
                        "e2e_delay_ms": row.get("e2e_delay_ms", ""),
                    }
                )

            total_messages = len(rows)
            pdr_overall_rows.append(
                {
                    "policy": policy,
                    "run": run,
                    "messages_total": total_messages,
                    "messages_delivered": delivered_count,
                    "pdr_overall": (delivered_count / total_messages) if total_messages > 0 else float("nan"),
                }
            )
            run_pdr_by_cat[(policy, run)] = {k: (v[0], v[1]) for k, v in type_counts.items()}

        death_path = files.get(DEATH_FILE)
        if death_path:
            rows = read_csv(death_path)
            contract_rows[DEATH_FILE]["present_runs"] += 1
            contract_rows[DEATH_FILE]["columns"].update(rows[0].keys() if rows else [])

            cumulative = 0.0
            cumulative_series: List[Tuple[float, float]] = []
            for row in rows:
                event_time = parse_time(row, ["time", "event_time", "timestamp", "sim_time"])
                if event_time is None:
                    continue
                cumulative += 1.0
                cumulative_series.append((event_time, cumulative))
                death_rows.append(
                    {
                        "policy": policy,
                        "run": run,
                        "uav_id": row.get("uav_id", row.get("agent_id", "")),
                        "event_time": event_time,
                    }
                )
            death_corr = run_linearity_check(cumulative_series)
            if death_corr is not None and death_corr <= 0.99:
                linearity_failures.append(f"deaths:{policy}/{run} corr={death_corr:.4f}")

        charge_path = files.get(CHARGE_FILE)
        if charge_path:
            rows = read_csv(charge_path)
            contract_rows[CHARGE_FILE]["present_runs"] += 1
            contract_rows[CHARGE_FILE]["columns"].update(rows[0].keys() if rows else [])

            successful = 0
            charged_energy_total = 0.0
            completion_times: List[float] = []
            charged_energy_cumulative: List[Tuple[float, float]] = []
            cumulative_energy = 0.0
            for row in rows:
                success = str_is_true(row.get("success")) or str_is_true(row.get("completed"))
                if not success:
                    continue
                request_time = parse_time(row, ["request_time"])
                dock_start = parse_time(row, ["dock_start_time", "start_time"])
                end_time = parse_time(row, ["end_time", "charge_end_time", "undock_time"])
                if request_time is not None and dock_start is not None:
                    wait_rows.append(
                        {
                            "policy": policy,
                            "run": run,
                            "role": row.get("role", row.get("uav_role", "")),
                            "wait_time": dock_start - request_time,
                            "request_time": request_time,
                            "dock_start_time": dock_start,
                        }
                    )

                request_battery = parse_float(row.get("request_battery"))
                end_battery = parse_float(row.get("end_battery"))
                energy = None
                if request_battery is not None and end_battery is not None:
                    energy = max(0.0, end_battery - request_battery)
                if energy is None:
                    fallback = parse_float(row.get("energy_recovered"))
                    energy = fallback if fallback is not None else 0.0

                charged_energy_total += energy
                successful += 1
                if end_time is not None:
                    completion_times.append(end_time)
                    cumulative_energy += energy
                    charged_energy_cumulative.append((end_time, cumulative_energy))

            if completion_times:
                sim_duration_hours = (max(completion_times) - min(completion_times)) / 3600.0
                throughput = successful / sim_duration_hours if sim_duration_hours > 0 else float("nan")
            else:
                throughput = float("nan")

            throughput_rows.append(
                {
                    "policy": policy,
                    "run": run,
                    "successful_charges": successful,
                    "successful_charges_per_hour": throughput,
                    "total_charged_energy": charged_energy_total,
                }
            )

            completion_series = sorted((t, i + 1.0) for i, t in enumerate(sorted(completion_times)))
            comp_corr = run_linearity_check(completion_series)
            if comp_corr is not None and comp_corr <= 0.99:
                linearity_failures.append(f"charge-completions:{policy}/{run} corr={comp_corr:.4f}")
            energy_corr = run_linearity_check(charged_energy_cumulative)
            if energy_corr is not None and energy_corr <= 0.99:
                linearity_failures.append(f"charged-energy:{policy}/{run} corr={energy_corr:.4f}")

        queue_path = files.get(QUEUE_FILE)
        if queue_path:
            rows = read_csv(queue_path)
            contract_rows[QUEUE_FILE]["present_runs"] += 1
            contract_rows[QUEUE_FILE]["columns"].update(rows[0].keys() if rows else [])

            for row in rows:
                time_value = parse_time(row, ["time", "timestamp", "sim_time"])
                if time_value is None:
                    continue
                for col, raw in row.items():
                    if col in {"time", "timestamp", "sim_time"}:
                        continue
                    value = parse_float(raw)
                    if value is None:
                        continue
                    is_int = math.isclose(value, round(value), rel_tol=0, abs_tol=1e-9)
                    queue_label = "queue length"
                    if not is_int:
                        non_integer_queue_columns.add(col)
                        queue_label = "mean queue length"
                    queue_rows.append(
                        {
                            "policy": policy,
                            "run": run,
                            "time": time_value,
                            "queue_column": col,
                            "queue_value": value,
                            "queue_label": queue_label,
                        }
                    )

    pdr_by_type_rows: List[Dict[str, object]] = []
    for policy, run, _ in runs:
        counts = run_pdr_by_cat.get((policy, run), {})
        for cat in sorted(categories_seen):
            delivered, denom = counts.get(cat, (0, 0))
            if denom == 0:
                pdr_zero_denominator_bins += 1
            pdr_by_type_rows.append(
                {
                    "policy": policy,
                    "run": run,
                    "category": cat,
                    "messages_total": denom,
                    "messages_delivered": delivered,
                    "pdr": (delivered / denom) if denom > 0 else float("nan"),
                }
            )

    write_csv(
        out_dir / "messages_tidy.csv",
        messages_tidy,
        ["policy", "run", "message_index", "delivered", "control_type", "msg_type", "e2e_delay_ms"],
    )
    write_csv(
        out_dir / "pdr_overall_per_run.csv",
        pdr_overall_rows,
        ["policy", "run", "messages_total", "messages_delivered", "pdr_overall"],
    )
    write_csv(
        out_dir / "pdr_by_type_per_run.csv",
        pdr_by_type_rows,
        ["policy", "run", "category", "messages_total", "messages_delivered", "pdr"],
    )
    write_csv(out_dir / "delay_samples.csv", delay_rows, ["policy", "run", "message_index", "delay_ms"])
    write_csv(out_dir / "death_events_tidy.csv", death_rows, ["policy", "run", "uav_id", "event_time"])
    write_csv(
        out_dir / "charge_wait_samples.csv",
        wait_rows,
        ["policy", "run", "role", "wait_time", "request_time", "dock_start_time"],
    )
    write_csv(
        out_dir / "charge_throughput_per_run.csv",
        throughput_rows,
        ["policy", "run", "successful_charges", "successful_charges_per_hour", "total_charged_energy"],
    )
    write_csv(
        out_dir / "queue_timeseries_tidy.csv",
        queue_rows,
        ["policy", "run", "time", "queue_column", "queue_value", "queue_label"],
    )

    contract_path = out_dir / "data_contract.md"
    with contract_path.open("w", encoding="utf-8") as f:
        f.write("# Data Contract\n\n")
        f.write("Generated from discovered policy/run folders under input root.\n\n")
        f.write("| File | Present in runs | Required columns found | Inferred semantics |\n")
        f.write("|---|---:|---|---|\n")
        for fname in [MESSAGE_FILE, QUEUE_FILE, DEATH_FILE, CHARGE_FILE]:
            entry = contract_rows[fname]
            cols = sorted(entry["columns"])
            f.write(
                f"| `{fname}` | {entry['present_runs']} | {', '.join(cols) if cols else '(none found)'} | {entry['semantics']} |\n"
            )

    print(f"WARN: pdr_bins_with_zero_denominator={pdr_zero_denominator_bins} (set to NaN)")
    if non_integer_queue_columns:
        cols = ", ".join(sorted(non_integer_queue_columns))
        print(f"WARN: queue_columns_with_non_integer_values={len(non_integer_queue_columns)} [{cols}] (labeled as mean queue length)")
    else:
        print("WARN: queue_columns_with_non_integer_values=0")

    if linearity_failures:
        print("WARN: cumulative_series_linearity_failures (corr<=0.99):")
        for item in linearity_failures:
            print(f"  - {item}")
    else:
        print("WARN: cumulative_series_linearity_failures=0")


if __name__ == "__main__":
    main()
