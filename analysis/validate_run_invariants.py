#!/usr/bin/env python3
"""Validate per-run invariants and print a compact integrity report."""

from __future__ import annotations

import argparse
import csv
import math
from collections import Counter
from pathlib import Path


def to_float(value: str) -> float | None:
    if value is None:
        return None
    text = str(value).strip()
    if text == "" or text.lower() in {"nan", "none", "null"}:
        return None
    try:
        return float(text)
    except ValueError:
        return None


def to_bool(value: str) -> bool | None:
    if value is None:
        return None
    text = str(value).strip().lower()
    if text in {"1", "true", "t", "yes", "y"}:
        return True
    if text in {"0", "false", "f", "no", "n"}:
        return False
    return None


def is_integer_like(value: float | None, tol: float = 1e-9) -> bool:
    if value is None:
        return False
    return abs(value - round(value)) <= tol


def safe_ratio(num: float, den: float) -> float | None:
    if den <= 0:
        return None
    return num / den


def read_rows(path: Path) -> list[dict[str, str]]:
    with path.open("r", newline="") as f:
        return list(csv.DictReader(f))


def pdr_from_messages(rows: list[dict[str, str]]) -> tuple[float | None, int, int]:
    generated = len(rows)
    delivered = sum(1 for r in rows if to_bool(r.get("delivered", "")) is True)
    return safe_ratio(delivered, generated), generated, delivered


def pdr_from_windows(rows: list[dict[str, str]]) -> tuple[float | None, float, float, int]:
    generated_sum = 0.0
    delivered_sum = 0.0
    invalid_bins = 0
    for r in rows:
        g = to_float(r.get("window_generated", ""))
        d = to_float(r.get("window_delivered", ""))
        pdr = to_float(r.get("window_pdr", ""))
        if g is not None:
            generated_sum += g
        if d is not None:
            delivered_sum += d
        if (g is None or g <= 0) or (pdr is not None and pdr < 0):
            invalid_bins += 1
    return safe_ratio(delivered_sum, generated_sum), generated_sum, delivered_sum, invalid_bins


def death_checks(death_rows: list[dict[str, str]], queue_rows: list[dict[str, str]]) -> tuple[Counter, dict[str, int | bool]]:
    cause_counts: Counter[str] = Counter()
    for r in death_rows:
        cause = (r.get("cause") or "UNKNOWN").strip() or "UNKNOWN"
        cause_counts[cause] += 1

    # Compare final queue counters to event count from death_events.csv.
    death_events = len(death_rows)
    latest = max(queue_rows, key=lambda r: to_float(r.get("time", "")) or float("-inf"), default=None)

    final_dead_event_count = None
    final_current_dead_count = None
    if latest is not None:
        final_dead_event_count = to_float(latest.get("dead_event_count", ""))
        final_current_dead_count = to_float(latest.get("current_dead_count", ""))

    # The cumulative source should track events, not currently dead UAVs.
    event_based = False
    if final_dead_event_count is not None:
        event_based = math.isclose(final_dead_event_count, float(death_events), abs_tol=1e-6)

    return cause_counts, {
        "death_events": death_events,
        "final_dead_event_count": int(final_dead_event_count) if final_dead_event_count is not None else -1,
        "final_current_dead_count": int(final_current_dead_count) if final_current_dead_count is not None else -1,
        "event_based": event_based,
    }


def charge_checks(rows: list[dict[str, str]]) -> dict[str, object]:
    failures = Counter()
    success_outcome = 0
    success_completed = 0
    decisions = 0
    started = 0
    req = len(rows)
    energy_recovered_sum = 0.0
    battery_delta_sum = 0.0

    for r in rows:
        decision_time = to_float(r.get("decision_time", ""))
        dock_start_time = to_float(r.get("dock_start_time", ""))
        outcome = (r.get("outcome") or "").strip() or "UNKNOWN"
        failure_reason = (r.get("failure_reason") or "").strip() or "UNKNOWN"
        completed = to_bool(r.get("charge_completed", "")) is True

        if decision_time is not None and decision_time > 0:
            decisions += 1
        if dock_start_time is not None and dock_start_time > 0:
            started += 1
        if completed:
            success_completed += 1
        if outcome.upper() == "SUCCESS":
            success_outcome += 1
        if outcome.upper() != "SUCCESS":
            failures[f"{outcome}|{failure_reason}"] += 1

        if completed:
            energy_recovered_sum += to_float(r.get("energy_recovered", "")) or 0.0
            end_b = to_float(r.get("end_battery", ""))
            req_b = to_float(r.get("request_battery", ""))
            if end_b is not None and req_b is not None:
                battery_delta_sum += end_b - req_b

    return {
        "requests": req,
        "decisions": decisions,
        "started": started,
        "failures": failures,
        "success_completed": success_completed,
        "success_outcome": success_outcome,
        "energy_recovered_sum": energy_recovered_sum,
        "battery_delta_sum": battery_delta_sum,
    }


def queue_integrity(rows: list[dict[str, str]]) -> tuple[int, int]:
    bad = 0
    total = 0
    for r in rows:
        v = to_float(r.get("queue_length_ugv", ""))
        if v is None:
            continue
        total += 1
        if not is_integer_like(v):
            bad += 1
    return bad, total


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate invariants for a run directory")
    p.add_argument("--run_dir", type=Path, required=True, help="Directory containing run CSV artifacts")
    return p.parse_args()


def main() -> int:
    args = parse_args()
    run_dir: Path = args.run_dir
    files = {
        "messages": run_dir / "messages.csv",
        "network": run_dir / "network_timeseries.csv",
        "deaths": run_dir / "death_events.csv",
        "charges": run_dir / "charge_events.csv",
        "queue": run_dir / "charge_queue_timeseries.csv",
    }

    severe: list[str] = []
    missing = [name for name, path in files.items() if not path.exists()]
    if missing:
        print(f"ERROR missing files: {', '.join(missing)}")
        return 2

    msg_rows = read_rows(files["messages"])
    net_rows = read_rows(files["network"])
    death_rows = read_rows(files["deaths"])
    charge_rows = read_rows(files["charges"])
    queue_rows = read_rows(files["queue"])

    pdr_messages, msg_gen, msg_del = pdr_from_messages(msg_rows)
    pdr_windows, win_gen, win_del, invalid_bins = pdr_from_windows(net_rows)
    causes, death_meta = death_checks(death_rows, queue_rows)
    charge = charge_checks(charge_rows)
    non_integer_queue, queue_rows_used = queue_integrity(queue_rows)

    if pdr_messages is None or pdr_windows is None:
        severe.append("Unable to compute overall PDR due to zero/invalid denominator")
    elif abs(pdr_messages - pdr_windows) > 0.05:
        severe.append(f"PDR mismatch too large: messages={pdr_messages:.4f}, windows={pdr_windows:.4f}")

    if invalid_bins > 0:
        severe.append(f"Invalid PDR bins detected: {invalid_bins}")

    if not death_meta["event_based"]:
        severe.append("Cumulative death source is not event-count based")

    if non_integer_queue > 0:
        severe.append(f"Non-integer queue_length_ugv rows: {non_integer_queue}")

    print(f"Run dir: {run_dir}")
    print(
        "PDR | "
        f"messages={pdr_messages if pdr_messages is not None else 'NA'} "
        f"({msg_del}/{msg_gen}) | "
        f"windows={pdr_windows if pdr_windows is not None else 'NA'} "
        f"({win_del:.0f}/{win_gen:.0f})"
    )
    print(f"PDR bins | invalid={invalid_bins}")
    print("Deaths | " + ", ".join(f"{k}:{v}" for k, v in sorted(causes.items())) if causes else "Deaths | none")
    print(
        "Deaths cumulative | "
        f"events={death_meta['death_events']} "
        f"dead_event_count_final={death_meta['final_dead_event_count']} "
        f"current_dead_count_final={death_meta['final_current_dead_count']} "
        f"event_based={death_meta['event_based']}"
    )
    print(
        "Charge | "
        f"requests={charge['requests']} decisions={charge['decisions']} started={charge['started']} "
        f"success_completed={charge['success_completed']} success_outcome={charge['success_outcome']} "
        f"energy_recovered_sum={charge['energy_recovered_sum']:.3f} "
        f"battery_delta_sum={charge['battery_delta_sum']:.3f}"
    )
    failures: Counter = charge["failures"]  # type: ignore[assignment]
    print("Charge failures | " + (", ".join(f"{k}:{v}" for k, v in failures.most_common()) if failures else "none"))
    print(f"Queue integrity | non_integer_queue_length_ugv={non_integer_queue}/{queue_rows_used}")

    if severe:
        print("SEVERE inconsistencies:")
        for item in severe:
            print(f" - {item}")
        return 1

    print("All severe invariants passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
