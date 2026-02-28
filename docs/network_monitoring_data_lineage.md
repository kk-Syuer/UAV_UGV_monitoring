# Network Monitoring Data Lineage — v2 (Append-Only Event-Sourced Design)

## 0) Scope + Method

- **CONFIRMED** = directly supported by `src/` code after this revision.
- **INFERRED** = best-effort interpretation where code does not enforce constraints.
- Authoritative source tree: `src/network_monitor/src/network_monitor_node.cpp` (v2).

**Design goals of this revision:**

| Goal | Status |
|---|---|
| All artifacts append-only (no `std::ios::trunc`) | ✅ Implemented |
| Event-sourced atomic tables (one primary fact per file) | ✅ New tables added |
| Every row carries `run_id`, `protocol_name`, `replicate_id`, `run_instance_id`, `t_rel_s` | ✅ Implemented |
| `summary.json` → `summary_snapshots.jsonl` (append) | ✅ Implemented |
| `flush_period_sec` launch param aligned to `csv_write_period_sec` node param | ✅ Fixed in launch |

---

## 1) ASCII Trace Map

```text
/fanet/network_bus_raw → [FaultInjectorNode] → /fanet/network_bus
                                           \→ DROP control messages (WEATHER_DROP)

/fanet/network_bus_raw → trafficRawCallback → logPacketGeneratedEvent()
                                           → records_[msg_id] init

/fanet/network_bus  → trafficCallback → DROP branch  → logPacketDropEvent()
                                     → ACK branch   → logPacketAckEvent()
                                     → CHARGE_REQUEST → logChargeRequestEvent()

/fanet/delivered → deliveredCallback → logPacketDeliveredEvent()
                                    → CHARGE_DECISION → logChargeDecisionEvent()

/fanet/status → statusCallback → uav_states_ update
                              → DOCK_START / DOCK_END / PRE-DOCK PREEMPT
                                → logChargeSessionEvent()

charge_timeout_timer → checkChargeTimeouts → TIMEOUT → logChargeSessionEvent()

FAILURE_EVENT (via trafficCallback) → handleFailureFromTraffic
  → ENERGY_DEPLETED → logChargeSessionEvent()

Periodic timers → writeStatusTimeseriesRow / writeWeatherTimeseriesRow /
                  writeChargeQueueTimeseriesRow / writeNetworkTimeseriesRow

csv_timer_ + shutdown → writeOutputs() → append-only snapshot writers
                                       → writeSummarySnapshotJsonl()
```

---

## A) Outputs Inventory

**Path construction:** `output_root_ = output_dir / run_id`

All files opened exclusively with `std::ios::app`. Header written only if file is absent or zero-size (via `openAppend()` helper which checks `filesystem::file_size == 0`).

| File | Mode | Creator | Trigger | Notes |
|---|---|---|---|---|
| `packet_generated_events.csv` | **APPEND** | monitor | per-msg, `trafficRawCallback` first-seen | NEW: event-sourced generation table |
| `packet_delivered_events.csv` | **APPEND** | monitor | per-delivery, `deliveredCallback` | NEW: event-sourced delivery table |
| `packet_drop_events.csv` | **APPEND** | monitor | per-drop, `trafficCallback` DROP branch | NEW: event-sourced drop table |
| `packet_ack_events.csv` | **APPEND** | monitor | per-ACK, `trafficCallback` ACK branch | NEW: event-sourced ACK table |
| `charge_request_events.csv` | **APPEND** | monitor | per-request, `trafficCallback` CHARGE_REQUEST | NEW: atomic charge request table |
| `charge_decision_events.csv` | **APPEND** | monitor | per-decision, `deliveredCallback` CHARGE_DECISION | NEW: atomic charge decision table |
| `charge_session_events.csv` | **APPEND** | monitor | DOCK_START/END/PREEMPTED/TIMEOUT/ENERGY_DEPLETED | NEW: atomic session lifecycle events |
| `messages.csv` | **APPEND** | monitor | per-record, first-seen snapshot, `writeMessagesCsv()` | Supplemental; each record exported once |
| `qos_metrics.csv` | **APPEND** | monitor | snapshot per flush, all known categories | Updated: timeseries of cumulative QoS state |
| `charge_events.csv` | **APPEND** | monitor | per terminal outcome, `writeChargeEventsCsv()` | Updated: only terminal records exported once |
| `preemption_events.csv` | **APPEND** | monitor | incremental, `writePreemptionEventsCsv()` | Updated: no rewrite |
| `recovery_events.csv` | **APPEND** | monitor | per-event, `writeRecoveryEventsCsv()` | Updated: no rewrite |
| `status_timeseries.csv` | **APPEND** | monitor | per `status_timeseries_timer_` tick + flush | Updated: run metadata added |
| `charge_queue_timeseries.csv` | **APPEND** | monitor | per `queue_timeseries_timer_` tick + flush | Updated: invariant check column added |
| `weather_timeseries.csv` | **APPEND** | monitor | per `weather_timeseries_timer_` tick + flush | Updated: run metadata added |
| `death_events.csv` | **APPEND** | monitor | incremental, `writeDeathEventsCsv()` | Updated: no rewrite |
| `network_timeseries.csv` | **APPEND** | monitor | per `network_timeseries_timer_` tick + flush | Updated: run metadata added |
| `summary_snapshots.jsonl` | **APPEND** | monitor | per `writeOutputs()` + shutdown | **REPLACES** `summary.json`; one JSON object per line |

---

## B) Per-File Schemas

### Universal run-metadata columns (present in ALL files)

| Column | Type | Domain | Notes |
|---|---|---|---|
| `run_id` | string | run label | from launch param |
| `protocol_name` | string | e.g. `FCFS`, `PRIORITY_CH_FIRST`, … | from launch param `protocol_name` |
| `replicate_id` | int | 1..3 (or 0..2) | from launch param `replicate_id` |
| `run_instance_id` | string | nanoseconds since Unix epoch at startup | unique per process execution; disambiguates restarts |
| `t_rel_s` | float | seconds since node start | `(this->now() - start_time_).seconds()`; use for cross-replicate alignment |
| `time_s` | float | ROS clock seconds at write | producer or monitor clock (see timestamp table) |

### B.1 `packet_generated_events.csv` (NEW)

**Definition:** First observation of `msg_id` on `/fanet/network_bus_raw`.
This is the generation proxy: the raw bus carries messages before fault injection.

| Column | Type | Domain | Units | Missing | Key |
|---|---|---|---|---|---|
| *(run metadata)* | — | — | — | — | — |
| `msg_id` | string | unique message id | — | never | PK |
| `flow_type` | int | {0=DATA, 1=CONTROL} | — | never | — |
| `control_type` | string | NONE, CHARGE_REQUEST, … | — | "" | — |
| `src_id` | string | node id | — | "" | — |
| `dst_id` | string | node id | — | "" | — |
| `creation_time_s` | float | `msg.creation_time` (producer ROS clock) | s | 0.0 if unset | — |
| `payload_bytes` | int | ≥0 | bytes | 0 | — |

Guard: `already_logged_generated_` set; only first observation logged.

### B.2 `packet_delivered_events.csv` (NEW)

**Definition:** First delivery observation from `/fanet/delivered`.

| Column | Type | Domain | Units | Missing | Key |
|---|---|---|---|---|---|
| *(run metadata)* | — | — | — | — | — |
| `msg_id` | string | unique message id | — | never | PK |
| `delivered_time_s` | float | prefer `msg.last_rx_time`; fallback to monitor `now()` | s | — | — |
| `receiver_id` | string | `msg.dst_id` | — | "" | — |
| `hop_count` | int | ≥0 or -1 (unknown) | hops | -1 | — |
| `ttl_hops` | int | ≥0 or -1 | hops | -1 | — |
| `delivered_flag` | bool | always `true` | — | — | — |

Guard: `already_logged_delivered_` set.

**Clock note:** `delivered_time_s` uses `msg.last_rx_time` (UAV ROS clock at final receive) when non-zero, else monitor `now()`. Both are ROS clock time. `creation_time_s` lives in `packet_generated_events.csv` — join on `msg_id` for e2e delay.

### B.3 `packet_drop_events.csv` (NEW)

| Column | Type | Domain | Units | Missing | Key |
|---|---|---|---|---|---|
| *(run metadata)* | — | — | — | — | — |
| `ref_msg_id` | string | joinable to `packet_generated_events.msg_id` | — | never | PK |
| `drop_reason` | string | `WEATHER_DROP`, `TTL_EXPIRED`, `UNKNOWN`, … | — | `UNKNOWN` | — |
| `dropper_id` | string | node that emitted DROP | — | "" | — |

Guard: `dropped_ids_` set (first DROP per `ref_msg_id` only).

### B.4 `packet_ack_events.csv` (NEW)

| Column | Type | Domain | Units | Missing | Key |
|---|---|---|---|---|---|
| *(run metadata)* | — | — | — | — | — |
| `ref_msg_id` | string | joinable to `msg_id` | — | never | PK |
| `ack_time_s` | float | monitor `now()` at ACK observation | s | — | — |
| `ack_src_id` | string | `msg.src_id` of ACK message | — | "" | — |

Guard: `ack_by_ref_` map (first ACK per `ref_msg_id`).

### B.5 `charge_request_events.csv` (NEW)

**Trigger:** First time `CHARGE_REQUEST` `msg_id` appears on `/fanet/network_bus`.

| Column | Type | Domain | Units | Missing | Key |
|---|---|---|---|---|---|
| *(run metadata)* | — | — | — | — | — |
| `request_msg_id` | string | = `msg_id` of CHARGE_REQUEST | — | never | PK |
| `uav_id` | string | `msg.src_id` | — | "" | — |
| `role` | int | {0=MEMBER, 1=CH, 2=BACKUP_CH, -1=unknown} | — | -1 | — |
| `battery_at_request` | float | battery % at request time | % | -1.0 | — |
| `request_time_s` | float | `msg.creation_time` (producer ROS clock) | s | -1.0 | — |

### B.6 `charge_decision_events.csv` (NEW)

**Trigger:** First delivery of `CHARGE_DECISION` on `/fanet/delivered`.

| Column | Type | Domain | Units | Missing | Key |
|---|---|---|---|---|---|
| *(run metadata)* | — | — | — | — | — |
| `request_msg_id` | string | = `msg.ref_msg_id` | — | never | FK → charge_request_events |
| `decision_msg_id` | string | = `msg.msg_id` | — | — | PK |
| `decision_time_s` | float | `delivered_wall_time` (monitor, prefer `msg.last_rx_time`) | s | — | — |
| `outcome` | string | {ACCEPTED, REJECTED, PREEMPTED} | — | — | — |
| `failure_reason` | string | REJECTED / PREEMPTED / "" | — | "" | — |
| `decision_latency_ms` | float | `(decision_time - request_time) * 1000` | ms | -1.0 if request_time unknown | — |

Guard: `already_logged_charge_decision_` set.

### B.7 `charge_session_events.csv` (NEW)

**Trigger:** At each lifecycle transition: DOCK_START, DOCK_END, PREEMPTED, TIMEOUT, ENERGY_DEPLETED.

| Column | Type | Domain | Units | Missing | Key |
|---|---|---|---|---|---|
| *(run metadata)* | — | — | — | — | — |
| `request_msg_id` | string | FK → charge_request_events | — | — | FK |
| `uav_id` | string | — | — | "" | — |
| `role` | int | {0,1,2,-1} | — | -1 | — |
| `event_type` | string | {DOCK_START, DOCK_END, PREEMPTED, TIMEOUT, ENERGY_DEPLETED, TERMINAL} | — | — | — |
| `event_time_s` | float | monitor `now()` (ROS clock) | s | — | — |
| `waiting_time_ms` | float | `(dock_start - request_time) * 1000`; set at DOCK_START | ms | -1.0 | — |
| `charge_duration_s` | float | `(charge_end - dock_start)`; set at DOCK_END | s | -1.0 | — |
| `energy_charged_wh` | float | `energy_delta_pct * battery_capacity_wh / 100`; set at DOCK_END | Wh | -1.0 if capacity unknown | — |
| `battery_before` | float | battery % before event | % | -1.0 | — |
| `battery_after` | float | battery % after event | % | -1.0 | — |
| `battery_capacity_wh` | float | `UavStatus.battery_capacity` (cached) | Wh | 0.0 if unset | — |

**Energy formula:** `energy_charged_wh = (battery_after - battery_before) * battery_capacity_wh / 100.0` when `battery_capacity_wh > 0` and delta ≥ 0; else -1.0.

**No deduplication guard** on session events — each transition fires exactly once per lifecycle because the state machine in `statusCallback`/`checkChargeTimeouts`/`handleFailureFromTraffic` gates entry.

### B.8 `messages.csv` (UPDATED)

Supplemental; each record exported **once** (first-seen snapshot, append-only). Reconciled drop/ack info may be incomplete if DROP arrived after the record was exported — use `packet_drop_events.csv` for authoritative drop data.

Added columns: `protocol_name`, `replicate_id`, `run_instance_id`, `t_rel_s`, `time_s`.
Renamed: `ack_time` → `ack_time_s`.

### B.9 `qos_metrics.csv` (UPDATED)

Now a **snapshot timeseries**: every periodic flush writes one row per known `(flow_type, control_type)` category with current cumulative stats. Consumer should take the last row per `(run_id, flow_type, control_type)` for final values.

Added columns: `protocol_name`, `replicate_id`, `run_instance_id`, `t_rel_s`, `time_s`.

### B.10 `charge_events.csv` (UPDATED)

Now exports only **terminal** records (STARTED, REJECTED, DROPPED, TIMEOUT, PREEMPTED, ENERGY_DEPLETED) — each record exported once on first terminal state.
Non-terminal (PENDING/ACCEPTED) records are NOT present here; they are covered by the atomic event tables.
Renamed `energy_recovered` → `energy_recovered_pct`.

Added columns: `protocol_name`, `replicate_id`, `run_instance_id`, `t_rel_s`, `time_s`.

### B.11 `preemption_events.csv` (UPDATED)

Added columns: `protocol_name`, `replicate_id`, `run_instance_id`, `t_rel_s`.
Renamed existing `time` → `time_s`.

### B.12 `recovery_events.csv` (UPDATED)

Added columns: `protocol_name`, `replicate_id`, `run_instance_id`, `t_rel_s`.
Renamed `creation_time` → `creation_time_s`.

### B.13 `status_timeseries.csv` (UPDATED)

Added columns: `protocol_name`, `replicate_id`, `run_instance_id`, `t_rel_s`.
Renamed existing `time` → `time_s`.

Columns: `run_id, protocol_name, replicate_id, run_instance_id, t_rel_s, time_s, uav_id, role, charging_state, battery_level, backbone_active, x, y, z, energy_consumption_rate`

### B.14 `charge_queue_timeseries.csv` (UPDATED)

Added columns: `protocol_name`, `replicate_id`, `run_instance_id`, `t_rel_s`, `queue_length_total_check`.
Renamed `time` → `time_s`, `queue_length_ugv` → `queue_length_total`.

**Invariant column:**
`queue_length_total_check = queue_length_total - (queue_length_ch + queue_length_member + queue_length_unknown)`

Should be 0; non-zero values indicate a counting anomaly (snapshot from UGV vs. monitor-inferred disagreement).

**Mean wait note:** `mean_wait_ch_ms` and `mean_wait_member_ms` represent the mean across all STARTED records in the current run (not just those currently waiting). This is a cumulative mean, not an instantaneous queue wait.

### B.15 `weather_timeseries.csv` (UPDATED)

Added columns: `protocol_name`, `replicate_id`, `run_instance_id`, `t_rel_s`.
Renamed `time` → `time_s`.

Columns: `run_id, protocol_name, replicate_id, run_instance_id, t_rel_s, time_s, regime, temperature_c, wind_speed, wind_direction_deg, rain_intensity`

### B.16 `death_events.csv` (UPDATED)

Added columns: `protocol_name`, `replicate_id`, `run_instance_id`, `t_rel_s`.
Renamed `time` → `time_s`.

### B.17 `network_timeseries.csv` (UPDATED)

Added columns: `protocol_name`, `replicate_id`, `run_instance_id`, `t_rel_s`.
Renamed `time` → `time_s`.

### B.18 `summary_snapshots.jsonl` (NEW — replaces `summary.json`)

Append-only JSON Lines file. Each `writeOutputs()` call appends one complete JSON object followed by `\n`.

**Additional top-level fields in each snapshot:**
```json
{
  "run_id": "...",
  "protocol_name": "...",
  "replicate_id": 0,
  "run_instance_id": "...",
  "t_rel_s": 42.5,
  "time_s": 1234567890.5,
  "fleet": { ... },
  "charging": { ... },
  "charging_fairness": { ... },
  "network": { ... },
  "recovery": { ... }
}
```

Consumer: load the last JSON object with matching `run_instance_id` for the final run state.

---

## C) Timestamp Truth Table

| Field | Producer node | Event | Clock domain | Representation | Persisted to |
|---|---|---|---|---|---|
| `TrafficMessage.creation_time` | Producer nodes (UAV/UGV/Sink/User) | message creation before publish | ROS clock of producer | `builtin_interfaces/Time` → float s | `packet_generated_events.creation_time_s`, `charge_request_events.request_time_s`, `messages.creation_time_s` |
| `TrafficMessage.last_rx_time` | UAV forwarder / delivery clone | at receive or delivery | UAV ROS clock | `builtin_interfaces/Time` → float s | `packet_delivered_events.delivered_time_s` (preferred) |
| `delivered_wall_time` (monitor local) | NetworkMonitorNode | on `/fanet/delivered` callback | Monitor ROS clock (fallback when `last_rx_time` == 0) | `rclcpp::Time` → float s | `packet_delivered_events.delivered_time_s` (fallback), `charge_decision_events.decision_time_s`, `messages.delivered_time_s` |
| `t_rel_s` | NetworkMonitorNode | at row write time | Monitor ROS clock, relative to `start_time_` | float s | ALL tables |
| `time_s` | NetworkMonitorNode | at row write time (or event time) | Monitor ROS clock | float s | ALL tables |
| `event_time_s` in `charge_session_events` | NetworkMonitorNode | at lifecycle transition | Monitor ROS clock | float s | `charge_session_events.event_time_s` |
| `UavStatus.stamp` | UAV node | when publishing status | UAV ROS clock | `builtin_interfaces/Time` | Not directly persisted |
| `DeathEvent.time` | NetworkMonitorNode | from `FAILURE_EVENT msg.creation_time` | Producer ROS clock | `rclcpp::Time` → float s | `death_events.time_s` |

**Clock alignment note:**
All nodes should share `/use_sim_time=true` (or all false). If clocks diverge, `t_rel_s` computed at the monitor is the safest alignment key across replicates because it is relative to each run's `start_time_`. For cross-run alignment, use `t_rel_s` rather than `time_s`.

---

## D) Variable/State Provenance

### D.1 New event-logging "already_logged" sets

| Set | Member name | Keys | Guards |
|---|---|---|---|
| Packet generated | `already_logged_generated_` | `msg_id` | `packet_generated_events.csv` — one row per `msg_id` |
| Packet delivered | `already_logged_delivered_` | `msg_id` | `packet_delivered_events.csv` — one row per `msg_id` |
| Charge decision | `already_logged_charge_decision_` | `request_msg_id` (= `ref_msg_id` of decision) | `charge_decision_events.csv` — one row per request |
| Drop events | `dropped_ids_` (existing) | `ref_msg_id` | `packet_drop_events.csv` — one row per `ref_msg_id` |
| ACK events | `ack_by_ref_` (existing, first-insert) | `ref_msg_id` | `packet_ack_events.csv` — one row per `ref_msg_id` |
| Charge session | state machine in `statusCallback` / `checkChargeTimeouts` / `handleFailureFromTraffic` | implicit (each transition fires at most once) | `charge_session_events.csv` |

### D.2 `run_instance_id_` generation

```
run_instance_id_ = std::to_string(
  std::chrono::duration_cast<std::chrono::nanoseconds>(
    std::chrono::system_clock::now().time_since_epoch()).count())
```

This is a wall-clock nanosecond timestamp as a string. It is unique per process execution and is included in every row of every table.

### D.3 `battery_capacity_wh` provenance

Cached from `UavStatus.battery_capacity` (float32, in Wh) in `uav_states_[uav_id].battery_capacity`.
Updated on every `/fanet/status` message. Used in `charge_session_events.csv` to compute `energy_charged_wh`.
If `battery_capacity == 0.0`, `energy_charged_wh = -1.0` (sentinel).

### D.4 Existing state maps (unchanged)

- `records_`, `drop_by_ref_`, `ack_by_ref_` — unchanged; `messages.csv` and new event tables both read them.
- `charge_records_` — unchanged; `charge_events.csv` (terminal snapshots) and new event tables both reference it.
- `uav_states_` — updated to cache `battery_capacity`.
- `recovery_events_`, `preemption_events_`, `death_events_` — unchanged; writers now append-only.

---

## E) Join Guidance for All Plots

### E.1 Plot 1 — Per-UAV battery time series (per-run validation)

```
Table:  status_timeseries.csv
Filter: run_id = <run>
Group:  uav_id
Plot:   battery_level vs t_rel_s
Color:  charging_state (0=active, 1=going, 2=charging, 3=returning)
```

**Alignment:** Use `t_rel_s` (not `time_s`) so that multi-replicate plots share a common time axis.

### E.2 Plot 2 — Per-UAV battery time series (statistical, per protocol)

```
Tables: status_timeseries.csv (filtered by protocol_name)
Group:  protocol_name, uav_id
Align:  t_rel_s (interpolate to common grid per replicate_id)
Agg:    median / mean over replicate_id for each (uav_id, t_rel_bin)
```

**Required columns:** `protocol_name`, `replicate_id`, `t_rel_s`, `uav_id`, `battery_level`, `role`.

### E.3 Plot 3 — PDR distribution + box plot

**Option A (preferred):** Window samples from `network_timeseries.csv`
```
Table:  network_timeseries.csv
Filter: protocol_name, replicate_id
Column: window_pdr
Group:  protocol_name
Agg:    distribution / box plot across (replicate_id, t_rel_s windows)
```

**Option B:** Compute per-bin PDR from event tables
```python
gen = pd.read_csv("packet_generated_events.csv")
del_ = pd.read_csv("packet_delivered_events.csv")
# bin by t_rel_s in 10s windows
gen["t_bin"] = (gen.t_rel_s // 10).astype(int)
del_["t_bin"] = (del_.t_rel_s // 10).astype(int)
gen_cnt = gen.groupby(["run_id","protocol_name","replicate_id","t_bin"]).size()
del_cnt = del_.groupby(["run_id","protocol_name","replicate_id","t_bin"]).size()
pdr = (del_cnt / gen_cnt).fillna(0)
```

### E.4 Plot 4 — End-to-end delay distribution

```python
gen = pd.read_csv("packet_generated_events.csv")
del_ = pd.read_csv("packet_delivered_events.csv")
merged = del_.merge(gen[["run_id","msg_id","creation_time_s"]],
                    on=["run_id","msg_id"], how="inner")
merged["delay_ms"] = (merged.delivered_time_s - merged.creation_time_s) * 1000.0
# Filter out negative delays (clock skew)
merged = merged[merged.delay_ms >= 0]
```

**Join key:** `(run_id, msg_id)` exact.
**Clock note:** `creation_time_s` = producer ROS clock; `delivered_time_s` = UAV `last_rx_time` or monitor clock. Clock domains are the same (`/use_sim_time=true`) but skew < 1 ms typically.

### E.5 Plot 5 — Total energy charged per run/protocol

```python
sess = pd.read_csv("charge_session_events.csv")
dock_end = sess[sess.event_type == "DOCK_END"]
# energy_charged_wh is -1.0 when battery_capacity_wh == 0; exclude those
valid = dock_end[dock_end.energy_charged_wh >= 0]
total = valid.groupby(["run_id","protocol_name","replicate_id","uav_id"])\
             .energy_charged_wh.sum()
```

**Fallback:** If `energy_charged_wh == -1.0`, use `charge_events.csv` field `energy_recovered_pct` × mean battery capacity.

### E.6 Plot 6 — Average queue waiting time (by role)

```python
sess = pd.read_csv("charge_session_events.csv")
dock_start = sess[sess.event_type == "DOCK_START"]
# waiting_time_ms is set at DOCK_START
wait = dock_start[dock_start.waiting_time_ms >= 0]
avg_wait = wait.groupby(["protocol_name","replicate_id","role"])\
               .waiting_time_ms.mean()
```

**Alternative via join:**
```python
req = pd.read_csv("charge_request_events.csv")
dock_start = sess[sess.event_type == "DOCK_START"][
    ["run_id","request_msg_id","event_time_s","role"]]
merged = dock_start.merge(req[["run_id","request_msg_id","request_time_s"]],
                           on=["run_id","request_msg_id"])
merged["waiting_ms"] = (merged.event_time_s - merged.request_time_s) * 1000
```

### E.7 Plot 7 — Charge queue length timeseries

```python
q = pd.read_csv("charge_queue_timeseries.csv")
# Verify invariant
assert (q.queue_length_total_check == 0).all(), "Invariant violated"
# Plot
q.groupby(["protocol_name","replicate_id"]).apply(
    lambda df: df.set_index("t_rel_s")[["queue_length_total","queue_length_ch",
                                         "queue_length_member"]].plot())
```

**Join key for cross-run comparison:** `protocol_name`, `replicate_id`, `t_rel_s` (use `merge_asof` tolerance=0.5s).

### E.8 Plot 8 — Waiting time CDF for CH vs Members

```python
sess = pd.read_csv("charge_session_events.csv")
dock_start = sess[(sess.event_type == "DOCK_START") & (sess.waiting_time_ms >= 0)]
ch   = dock_start[dock_start.role == 1].waiting_time_ms
mem  = dock_start[dock_start.role == 0].waiting_time_ms
# CDF
import numpy as np
for data, label in [(ch,"CH"),(mem,"Member")]:
    x = np.sort(data)
    y = np.arange(1, len(x)+1) / len(x)
    plt.plot(x, y, label=label)
```

### E.9 Plot 9 — Average decision latency

```python
dec = pd.read_csv("charge_decision_events.csv")
# decision_latency_ms is pre-computed; filter out sentinel
valid = dec[dec.decision_latency_ms >= 0]
avg = valid.groupby(["protocol_name","replicate_id"]).decision_latency_ms.mean()
```

**Alternative via join:**
```python
req = pd.read_csv("charge_request_events.csv")
merged = dec.merge(req[["run_id","request_msg_id","request_time_s"]],
                    on=["run_id","request_msg_id"])
merged["latency_ms"] = (merged.decision_time_s - merged.request_time_s) * 1000
```

### E.10 Plot 10 — Cumulative charged energy over mission time

```python
sess = pd.read_csv("charge_session_events.csv")
dock_end = sess[(sess.event_type=="DOCK_END") & (sess.energy_charged_wh >= 0)]
dock_end = dock_end.sort_values("t_rel_s")
dock_end["cum_energy"] = dock_end.groupby(
    ["run_id","protocol_name","replicate_id"]).energy_charged_wh.cumsum()
```

### E.11 Plot 11 — PDR and delay by weather regime

```python
weather = pd.read_csv("weather_timeseries.csv")
gen = pd.read_csv("packet_generated_events.csv")
del_ = pd.read_csv("packet_delivered_events.csv")
# Assign weather regime to each packet via asof join on t_rel_s
gen_w = pd.merge_asof(gen.sort_values("t_rel_s"),
                       weather[["run_id","t_rel_s","regime"]].sort_values("t_rel_s"),
                       on="t_rel_s", by="run_id", direction="backward")
del_w = pd.merge_asof(del_.sort_values("t_rel_s"),
                       weather[["run_id","t_rel_s","regime"]].sort_values("t_rel_s"),
                       on="t_rel_s", by="run_id", direction="backward")
# PDR per regime
gen_cnt = gen_w.groupby(["protocol_name","replicate_id","regime"]).size()
del_cnt = del_w.groupby(["protocol_name","replicate_id","regime"]).size()
pdr_by_regime = (del_cnt / gen_cnt).fillna(0)
```

### E.12 Plot 12 — Battery drain rate vs weather regime

```python
status = pd.read_csv("status_timeseries.csv")
weather = pd.read_csv("weather_timeseries.csv")
# Asof join: assign regime to each status sample
status_w = pd.merge_asof(
    status.sort_values("t_rel_s"),
    weather[["run_id","t_rel_s","regime"]].sort_values("t_rel_s"),
    on="t_rel_s", by="run_id", direction="backward", tolerance=2.0)
# Drain rate = energy_consumption_rate (if available) or compute battery slope
drain = status_w.groupby(["protocol_name","replicate_id","uav_id","regime"])\
                .energy_consumption_rate.mean()
```

### E.13 Radar Plot — Policy comparison (per protocol, aggregated over 3 replicates)

**Axis inputs:**

| Axis | Source | Computation |
|---|---|---|
| Mean e2e delay | `packet_generated_events` + `packet_delivered_events` joined on `msg_id` | `mean(delivered_time_s - creation_time_s) * 1000` |
| Overall PDR | same two tables | `count(delivered) / count(generated)` |
| Fairness (Jain's index) | `charge_session_events` DOCK_END, per-UAV `energy_charged_wh` sum | `(Σ x_i)² / (n Σ x_i²)` |
| Mean charged energy | `charge_session_events` DOCK_END | `mean(energy_charged_wh)` per session |
| Death count (less death) | `death_events` | `count(uav_id)` per run |

**Aggregation over replicates:** compute the metric for each of 3 replicates, then take median across replicates for the radar value.

---

## F) Append-Only Correctness Verification

### Smoke test commands

```bash
# 1. Build
cd /home/user/UAV_UGV_monitoring
colcon build --symlink-install
source install/setup.bash

# 2. Find launch files
find src/system_bringup/launch -name "*.launch.py"

# 3. Run smoke test (short duration)
ros2 launch system_bringup experiment.launch.py run_id:=smoke01 protocol_name:=FCFS replicate_id:=1

# 4. Verify append-only after first run
ls -lh log/smoke01/
wc -l log/smoke01/*.csv log/smoke01/*.jsonl

# 5. Re-run (restart same run_id) — files should GROW, not shrink
ros2 launch system_bringup experiment.launch.py run_id:=smoke01 protocol_name:=FCFS replicate_id:=1

# 6. Verify files appended (sizes larger than step 4)
ls -lh log/smoke01/
# Confirm header appears only ONCE per file (file-size-0 guard at process start)
head -n 5 log/smoke01/packet_generated_events.csv
grep -c "^run_id" log/smoke01/packet_generated_events.csv  # should be 1

# 7. Verify run_instance_id disambiguates the two runs
python3 -c "
import pandas as pd
df = pd.read_csv('log/smoke01/packet_generated_events.csv')
print(df.run_instance_id.nunique(), 'distinct run_instance_ids (expect 2)')
print(df.groupby('run_instance_id').size())
"

# 8. Verify JSONL summary appends
wc -l log/smoke01/summary_snapshots.jsonl  # should increase after restart
python3 -c "
import json
with open('log/smoke01/summary_snapshots.jsonl') as f:
    for line in f:
        obj = json.loads(line)
        print(obj['run_instance_id'], obj['t_rel_s'])
"
```

---

## G) Coverage Gaps Resolved vs. Remaining

### Resolved in this revision

| # | Gap | Resolution |
|---|---|---|
| 1 | `messages.csv` + `charge_events.csv` truncate on final flush | Replaced with pure append; terminal-only export for charge_events |
| 2 | No separate generated/delivered/drop/ACK tables | New event tables added |
| 3 | No `run_instance_id` for restart safety | Generated at startup, included in every row |
| 4 | No `t_rel_s` for replicate alignment | Added to every table |
| 5 | No `protocol_name` / `replicate_id` in rows | Added to every table |
| 6 | `summary.json` truncated on every write | Replaced with `summary_snapshots.jsonl` (append) |
| 7 | `flush_period_sec` launch param ignored by node | Fixed: launch passes `csv_write_period_sec` |
| 8 | No invariant check in queue timeseries | `queue_length_total_check` column added |
| 9 | No energy_charged_Wh for plot 5/10 | Computed from `battery_capacity` (from `UavStatus`) at DOCK_END |
| 10 | No fairness index inputs | `energy_charged_wh` per session enables per-UAV Jain's index |

### Remaining gaps (not addressed in this revision)

| # | Gap | Suggested patch |
|---|---|---|
| 1 | No routing table snapshot | `routingTableCallback` is empty; add `routing_table_timeseries.csv` |
| 2 | `drop_reason` is free text | Add normalization map → `drop_reason_code` enum |
| 3 | `messages.csv` rows may lack drop info (reconciled after export) | Use `packet_drop_events.csv` as authoritative drop source |
| 4 | No per-message `cluster_id` or `category` in generated events | Add from `UavStatus.cluster_id` if needed for per-cluster PDR |
