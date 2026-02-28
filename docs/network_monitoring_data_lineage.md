# Network Monitoring Data Lineage + Variable/State Provenance + Column Domain Spec

## 0) Scope + method

- **CONFIRMED** = directly supported by `src/` code.
- **INFERRED** = best-effort interpretation where code does not enforce constraints.
- Authoritative source tree: `src/` only.

Discovery commands run:
- `rg -n "ofstream|fopen|\.csv|\.json|output_dir|run_id|filesystem::path|std::ofstream|fprintf|open\(" src`
- `rg -n "\.csv|\.json|std::ofstream|ofstream|fopen|json" src`
- `rg -n "last_tx_time|last_rx_time" src`
- `rg -n "network_bus_raw|network_bus|/fanet/delivered|creation_time|last_tx_time|last_rx_time" src`
- targeted `nl -ba ... | sed -n ...` on monitor and producer nodes.

## 1) ASCII trace map

```text
/fanet/network_bus_raw -> [FaultInjectorNode] -> /fanet/network_bus
                                            \-> DROP control messages (WEATHER_DROP)
/fanet/network_bus + /fanet/network_bus_raw + /fanet/delivered
  -> NetworkMonitorNode callbacks
  -> in-memory stores (records_, charge_records_, drop_by_ref_, ack_by_ref_, etc.)
  -> periodic timers + shutdown flush
  -> CSV/JSON artifacts under output_dir/run_id
```

---

## A) Outputs Inventory (Artifacts Catalog)

**CONFIRMED:** In `src/`, only `network_monitor_node.cpp` writes experiment files (all via `std::ofstream`).

**Path construction**
- `run_id_` + `output_dir_` parameters declared in monitor constructor.
- `output_root_ = std::filesystem::path(output_dir_) / run_id_`.
- Launch passes `run_id` and `output_dir` to monitor params.

| File | Pattern | Creator | Created | Write style | Flush cadence/triggers | Finalization | Code pointer(s) | Risks |
|---|---|---|---|---|---|---|---|---|
| `messages.csv` | `${output_dir}/${run_id}/messages.csv` | network_monitor_node | first write call | append; final flush truncates+rewrites | `csv_timer_` + shutdown + destructor | `writeOutputs(true)` | `writeMessagesCsv` (1014-1073), `writeOutputs` (984-998), shutdown/destructor (266-276) | crash before final flush leaves stale rows |
| `qos_metrics.csv` | same root | network_monitor_node | first write | append by key; final rewrite | same | same | 1075-1134 | key dedup means updated QoS appears at final rewrite |
| `charge_events.csv` | same root | network_monitor_node | first write | append by request id; final rewrite | same | same | 1136-1237 | pending records may be partial mid-run |
| `preemption_events.csv` | same root | network_monitor_node | lazy (only if events exist) | append by vector suffix; final rewrite | same | same | 1239-1293 | absent when no events |
| `recovery_events.csv` | same root | network_monitor_node | first write | append by msg_id; final rewrite | same | same | 1295-1342 | unordered-map row order unstable |
| `status_timeseries.csv` | same root | network_monitor_node | first timer/call | append-only | `status_timeseries_timer_` + `writeOutputs` | none | 1482-1521, timer 244-247 | duplicate close-in-time rows possible |
| `charge_queue_timeseries.csv` | same root | network_monitor_node | first timer/call | append-only | queue timer + `writeOutputs` | none | 1613-1772, timer 252-255 | mixed-source snapshot inconsistency |
| `weather_timeseries.csv` | same root | network_monitor_node | first weather+write | append-only | weather timer + `writeOutputs` | none | 1774-1810, timer 248-250 | file absent if no weather |
| `death_events.csv` | same root | network_monitor_node | lazy (non-empty death events) | append suffix; final rewrite | periodic+shutdown | final rewrite | 1813-1860 | absent when no deaths |
| `network_timeseries.csv` | same root | network_monitor_node | first timer/call | append-only | network timer + `writeOutputs` | none | 1863-1962, timer 256-258 | window metrics depend on current in-memory set |
| `summary.json` | same root | network_monitor_node | each write | always truncate+rewrite | every `writeOutputs` | final overwrite at shutdown | 1964-2182 | crash during truncation can leave partial JSON |

---

## B) Per-file schema + column lineage + domain specification

## B.1 `messages.csv`
Header at 1038-1040.

| Column | Type | Domain | Units | Missing encoding | Key | Source | First stored where/when | Pointer |
|---|---|---|---|---|---|---|---|---|
| run_id | string | run label | n/a | none | composite | param | ctor | 177,1052 |
| msg_id | string | DOMAIN UNKNOWN | n/a | empty possible | msg key | TrafficMessage.msg_id | `records_[msg_id]` init in callbacks | 354-357,381-384,456-459,1053 |
| flow_type | int | INFERRED {0,1} | n/a | default 0 | qos key part | msg.flow_type | rec init | 358,385,459,1054 |
| control_type | string | TrafficMessage control category | n/a | empty possible | qos key part | msg.control_type | rec init | 359,386,460,1055 |
| src_id | string | node id | n/a | empty possible | join key | msg.src_id | rec init | 360,387,461,1056 |
| dst_id | string | node id | n/a | empty possible | join key | msg.dst_id | rec init | 361,388,462,1057 |
| creation_time_s | timestamp(float) | ROS time seconds | s | may be 0 if msg unset | no | msg.creation_time | rec init | 362,389,463,1058 |
| delivered_time_s | timestamp(float) | ROS time seconds | s | `-1.0` | no | derived from delivered path | delivered callback | 409-413,477,1051,1059 |
| delivered | bool | {true,false} | n/a | none | no | rec flag | delivered callback | 476,1060 |
| e2e_delay_ms | float | >=0 expected; negative possible if clock anomalies | ms | `-1.0` undelivered | no | delivered-creation | write compute | 1047-1049,1061 |
| forward_count | int | ℕ0 | count | 0 | no | bus seen count | trafficCallback | 368,1062 |
| hop_count | int | ℕ0 or -1 | hops | `-1` unknown | no | delivered msg.hop_count | delivered callback | 479,1063 |
| ttl_hops | int | ℕ0 or -1 | hops | `-1` unknown | no | delivered msg.ttl | delivered callback | 480,1064 |
| payload_bytes | int | ℕ0 | bytes | 0 | no | payload size | rec init | 363,390,464,1065 |
| dropped | bool | {true,false} | n/a | false default | no | drop_by_ref reconciliation | reconcile pass | 1002-1007,1066 |
| drop_reason | string | free text (`WEATHER_DROP`, `TTL_EXPIRED`, etc.) | n/a | empty string | no | DROP msg.drop_reason | traffic/reconcile | 289-294,1003-1006,1067 |
| dropper_id | string | node id | n/a | empty string | no | DROP msg.src_id | traffic/reconcile | 291,1006,1068 |
| ack_time | timestamp(float) | ROS time seconds | s | `-1.0` | no | ACK seen time | traffic/reconcile | 312-315,1008-1010,1050,1069 |

## B.2 `qos_metrics.csv`
Header at 1098-1100.

Columns and domains:
- `run_id` string.
- `flow_type` int (from key parsing).
- `control_type` string.
- `generated,delivered,dropped` int ℕ0.
- `pdr` float in [0,1] (generated=0 => 0.0).
- `delay_mean_ms,delay_p95_ms,jitter_ms` float ms; sentinel `-1.0` when insufficient samples.
- `throughput_bps,generated_bps` float bps; `-1.0` until duration > 0.
- `qos_score` float [0,1] from weighted objective.

Source pipeline: `records_` -> `buildQosStats` -> `finalizeQosStats` -> writer (1452-1479,1390-1449,1103-1131).

## B.3 `charge_events.csv`
Header at 1159-1167.

All 34 columns written 1201-1233 in this exact order. Sentinel usage:
- time fields often emit `rec.<time>.seconds()` (zero if unset), except explicit `-1.0` for `charge_end_time` and `terminal_time` when unset.
- derived durations and energy: `-1.0` sentinel when insufficient inputs.
- role: `-1` when `role_known==false`.

Outcome domain from `chargeOutcomeToString`: `PENDING, ACCEPTED, REJECTED, ROUTING_DROP, TIMEOUT, STARTED, PREEMPTED, ENERGY_DEPLETED, UNKNOWN`.

Primary lineage inputs:
- CHARGE_REQUEST on traffic bus sets request identity and `request_time` from message `creation_time` (321-348).
- Delivered CHARGE_DECISION sets `decision_time`, accepted/rejected/preempted states + parsed rationale (414-454,856-909).
- `statusCallback` marks STARTED/charge completion/pre-dock return preemption (660-683).
- timeout timer marks TIMEOUT (686-700).
- failure path marks ENERGY_DEPLETED (592-607,812-828).

## B.4 `preemption_events.csv`
Header 1266-1269; rows 1275-1286.

Columns: `run_id,time,victim_uav_id,winner_uav_id,victim_role,winner_role,victim_priority,winner_priority,delta_priority,victim_charge_time_s,policy`.

Source:
- mostly parsed from `/ugv/queue_events` string payload (931-955).
- event time is monitor local `this->now()` (935), not embedded stamp.

## B.5 `recovery_events.csv`
Header 1318-1319; rows 1326-1339.

Columns: `run_id,msg_id,control_type,src_id,dst_id,epoch,member_id,ch_id,task_count,x,y,z,creation_time`.

Source:
- `trackRecoveryEvent` filters control types and parses payload variants (2184-2235).
- `epoch` parse fail => `-1` (2210-2214).
- task_count derived by semicolon token count (2237-2251).
- deployment pose parsed from first 3 comma-separated numbers, default 0,0,0 on parse failure (2253-2284).

## B.6 `status_timeseries.csv`
Header 1503; rows 1508-1519.

Columns: `run_id,time,uav_id,role,charging_state,battery_level,backbone_active,x,y,z,energy_consumption_rate`.

Source:
- `uav_states_` written from `/fanet/status` callback (630-643).
- `time` uses monitor now (1506).

## B.7 `charge_queue_timeseries.csv`
Header 1635-1640; rows 1751-1770.

Columns (20):
`run_id,time,queue_length_ugv,queue_length_ch,queue_length_member,queue_length_unknown,active_charging_ugv_sessions,ugv_dock_capacity,ugv_dock_utilization,mean_wait_ch_ms,mean_wait_member_ms,active_mission_count,active_charging_uav_status,going_to_ugv_count_uav_status,returning_count_uav_status,current_dead_count,dead_event_count,fleet_size,over_capacity_ugv,status_vs_ugv_gap`.

Domains:
- counts are ℕ0 except `status_vs_ugv_gap` signed.
- utilization ratio, sentinel `-1.0` when capacity<=0.
- mean waits ms, sentinel `-1.0` no samples.

Sources:
- queue from non-terminal charge records not started (1650-1667).
- wait means from started records (1668-1716).
- active state counts from `uav_states_` excluding dead_uavs_ (1683-1698).
- optional UGV snapshot overrides from parsed string fields (1722-1732,1592-1611).

## B.8 `weather_timeseries.csv`
Header 1799; row 1802-1808.

Columns: `run_id,time,regime,temperature_c,wind_speed,wind_direction_deg,rain_intensity`.

Source: cached weather fields from `/environment/weather` callback (834-853).

## B.9 `death_events.csv`
Header 1840; rows 1849-1857.

Columns: `run_id,time,uav_id,role,cause,battery_at_death,fleet_size,alive_count`.

Domain:
- `cause` currently `BATTERY_DEAD` from failure handler path.
- `role` sentinel `-1` when unknown.
- `alive_count` computed as `fleet_size - min(i+1,fleet_size)` at export order.

## B.10 `network_timeseries.csv`
Header 1885-1889; row 1945-1960.

Columns:
`run_id,time,window_sec,window_generated,window_delivered,window_dropped,window_pdr,window_delay_mean_ms,window_delay_p95_ms,window_jitter_ms,ctrl_charge_req_generated,ctrl_charge_req_delivered,ctrl_charge_req_pdr,ctrl_charge_dec_generated,ctrl_charge_dec_delivered,ctrl_charge_dec_pdr`.

Domain:
- window_* counts: ℕ0 over `records_` where `creation_time >= cutoff`.
- pdr fields: ratio; sentinel `-1.0` for zero denominators in window/control subgroups.
- delay/jitter ms: `-1.0` sentinel if no delivery samples.

## B.11 `summary.json`
Writer 1964-2182.

Schema (dot-path fields):
- `run_id` string
- `fleet.fleet_size`, `fleet.alive_count`, `fleet.current_dead_count`, `fleet.dead_event_count` (int)
- `fleet.survival_rate` float [0,1]
- `charging.requests_total,accepted,rejected,dropped,timeouts,started,preempted,energy_depleted` int
- `charging.success_rate` float [0,1]
- `charging.decision_latency_ms.mean,p95` float (`-1` possible)
- `charging.waiting_time_ms.mean` float (`-1` possible)
- `charging.energy_recovered.mean` float (`-1` possible)
- `charging_fairness.rejections_by_uav` object map string->int
- `charging_fairness.timeouts_by_uav` object map string->int
- `charging_fairness.max_waiting_time_ms_by_uav` object map string->float
- `network.qos_targets.{pdr,delay_ms,jitter_ms}` float params
- `network.qos_weights.{pdr,delay,jitter}` float params
- `network.by_category[]` objects with:
  - `flow_type` int, `control_type` string
  - `generated,delivered,dropped` int
  - `pdr` float
  - `delay_ms.mean,p95` float
  - `jitter_ms,throughput_bps,generated_bps,qos_score` float
  - `drops` map reason->count
- `recovery.start,done,cluster_reassign,task_assign,new_deployment` int

---

## C) Timestamp provenance + truth table

| Field | Defined by node | Created at event | Stored in variable | Persisted to | Clock domain | Representation | Pointers |
|---|---|---|---|---|---|---|---|
| `TrafficMessage.creation_time` | producer nodes (UAV/UGV/Sink/User/Fault drop reports) | message creation before publish | message field | many monitor CSVs via `rec.creation_time` / charge `request_time` | ROS clock of producer (`this->now()`) | builtin Time (sec,nanosec) | e.g. UAV 4594; UGV 2589/2225; Sink 253/323; User 58 |
| `TrafficMessage.last_tx_time` | **mostly UAV forwarder/delivery path** | before forwarding / at delivered clone in UAV | message field | not directly persisted by monitor | ROS clock | builtin Time | UAV stamp 3019,3129 |
| `TrafficMessage.last_rx_time` | UAV forwarding and delivered clone | receive-forward boundary / delivery | message field | monitor uses as delivered timestamp fallback-preferred | ROS clock | builtin Time | UAV 3020/3043/3127; monitor 409-413 |
| `delivered_wall_time` (monitor local variable) | network_monitor_node | on delivered callback | local var | `messages.delivered_time_s`; delay calculations; charge decision time | ROS clock of monitor unless last_rx_time present | rclcpp::Time | 407-413,423,477,482 |
| `MsgRecord.creation_time` | monitor | when rec first initialized from message | `records_[id].creation_time` | `messages.creation_time_s`; QoS and network window delay bases | inherits producer-stamped msg time | rclcpp::Time | 362/389/463,1058 |
| `MsgRecord.delivered_time` | monitor | first delivery observation | `records_[id].delivered_time` | `messages.delivered_time_s`, QoS delay, network window delay | monitor chosen delivered_wall_time | rclcpp::Time | 476-483,1464,1907 |
| `ChargeRecord.request_time` | monitor | on CHARGE_REQUEST traffic msg | `charge_records_[msg_id].request_time` | `charge_events.request_time` + derived waits/latencies | from message creation_time | rclcpp::Time | 321-329,1208 |
| `ChargeRecord.decision_time` | monitor | on delivered CHARGE_DECISION | `charge_records_[ref].decision_time` | `charge_events.decision_time`, `decision_latency_ms` | delivered_wall_time | rclcpp::Time | 414-424,1174-1176,1209 |
| `ChargeRecord.dock_start_time` | monitor | on status transition ACCEPTED -> charging | `charge_records_[id].dock_start_time` | `charge_events.dock_start_time`, waiting metrics | monitor now | rclcpp::Time | 660-666,1210 |
| `ChargeRecord.charge_end_time` | monitor | on status transition charging->not charging | `charge_records_[id].charge_end_time` | `charge_events.charge_end_time`, charge_duration | monitor now | rclcpp::Time | 668-675,1211 |
| `ChargeRecord.terminal_time` | monitor | on terminal outcomes | `charge_records_[id].terminal_time` | `charge_events.terminal_time`, `effective_wait_ms` | monitor now | rclcpp::Time | 306,431,434,665,682,697,827,1212 |
| `ChargeRequest.stamp` (direct topic) | UAV node | when publishing ChargeRequest | request message field | monitor `request_times_` only (not file column directly) | UAV ROS clock | builtin Time | UAV 878-883; monitor 508-513 |
| `DeathEvent.time` | monitor from failure message | when FAILURE_EVENT handled | `death_events_[].time` | `death_events.time` | from failure msg creation_time | rclcpp::Time | 564-574,599-601,1849-1850 |

### Explicit answers requested

1) **What is `time_generated`?**
- **NOT FOUND as a named field.**
- **CONFIRMED nearest equivalent** is `TrafficMessage.creation_time` (producer-side stamp) and monitor `messages.creation_time_s`. Set in producer nodes before publishing (`uav_node`, `ugv_charger_node`, `sink_gateway_node`, `user_device_node`).

2) **What is `time_delivered`?**
- **NOT FOUND as a named field.**
- **CONFIRMED equivalent** is monitor `delivered_wall_time` then `MsgRecord.delivered_time`, persisted as `messages.delivered_time_s`.
- It is computed in monitor delivered callback, preferring incoming `msg.last_rx_time`; fallback to monitor `now()` if missing.

3) **send_time / receive_time packed fields**
- **send_time analogue:** `last_tx_time` set in UAV forwarding (`stampForSend`) and delivery clone in UAV path.
- **receive_time analogue:** `last_rx_time` updated in UAV forward path and at UAV delivery clone.
- **Intermediate hops?** In UAV forward path `last_rx_time` is updated before resend; thus yes, intermediate updates occur where UAV handles forwarding.
- **Monitor extraction:** monitor only reads `last_rx_time` on delivered callback for delivered timestamp; it does not persist raw `last_tx_time`/`last_rx_time` columns.

Clock-risk notes:
- monitor timers use node `this->now()`; producer stamps also use each node `this->now()`.
- if `/use_sim_time` differs across nodes or time jumps backward, negative delay can appear at `(delivered_time - creation_time)`.
- monitor clamps some derived ages/waits to non-negative in specific helpers, but not all delay calculations (e.g., messages/e2e_delay_ms not clamped).

---

## D) Variable/State provenance appendix (logging contributors)

### D.1 Core message lifecycle state

- `records_ : unordered_map<string, MsgRecord>`
  - stores per-msg lineage: ids, flow/control, creation/delivery, counts, drop/ack flags.
  - defined as member; updated in `trafficRawCallback`, `trafficCallback`, `deliveredCallback`, `reconcileCausality`.
  - lifecycle: created lazily by msg_id; never evicted.
  - reverse deps: `messages.csv`, `qos_metrics.csv`, `network_timeseries.csv`, parts of `summary.json`.

- `drop_by_ref_ : unordered_map<string,pair<string,string>>`
  - `ref_msg_id -> (drop_reason, dropper_id)` from DROP control msgs.
  - set in `trafficCallback` DROP branch.
  - consumed by `reconcileCausality` to set record dropped fields.
  - reverse deps: `messages.csv`, QoS drop counts, summary drop maps.

- `ack_by_ref_ : unordered_map<string,rclcpp::Time>`
  - ref msg ack observation times from ACK messages.
  - set in ACK branch.
  - consumed in `reconcileCausality` => `MsgRecord.ack_time`.
  - reverse deps: `messages.csv`.

- `exported_messages_`, `exported_qos_keys_`, `exported_charge_requests_`, `exported_recovery_events_`, `exported_preemption_count_`, `exported_death_event_count_`
  - export cursors for incremental append mode.
  - set in each writer after row emission.
  - reset behavior: final flush truncates files and rewrites from all records (logic uses `final_flush`).

### D.2 Charge-state lineage state

- `charge_records_ : unordered_map<string, ChargeRecord>`
  - key: request message id.
  - population paths: CHARGE_REQUEST on `/fanet/network_bus`, CHARGE_DECISION on `/fanet/delivered`, status transitions, timeout timer, failure hooks, preemption parsing.
  - no eviction.
  - reverse deps: `charge_events.csv`, `charge_queue_timeseries.csv`, `summary.json` charging/fairness sections.

- `latest_request_by_uav_ : unordered_map<string,string>`
  - UAV -> latest request msg id.
  - set in traffic CHARGE_REQUEST and delivered CHARGE_DECISION path.
  - used by `statusCallback`, `markChargeFailureForUav`, preemption helper.

- `latest_role_by_uav_`, `latest_request_battery_by_uav_`
  - set by direct `/uav_fleet/charge_requests` callback.
  - used to backfill charge record role/request battery.

- `request_times_`
  - from direct ChargeRequest topic; used only for aggregate wait stats in `chargeDecisionCallback` (not persisted directly).

### D.3 Other logging state

- `uav_states_ : unordered_map<string,UavState>` from `/fanet/status`.
  - reverse deps: `status_timeseries.csv`, charge transition logic, death role/battery enrichment, queue timeseries counts.

- `preemption_events_ : vector<PreemptionEvent>` from `/ugv/queue_events` parser.
  - reverse deps: `preemption_events.csv`.

- `death_events_ : vector<DeathEvent>` from `FAILURE_EVENT` handling.
  - reverse deps: `death_events.csv`.

- weather cache (`current_weather_*`, `weather_received_`) from `/environment/weather`.
  - reverse deps: `weather_timeseries.csv`.

- `latest_ugv_charging_snapshot_` from `/ugv/charging_snapshot` string parser.
  - reverse deps: `charge_queue_timeseries.csv` override columns.

Concurrency/staleness:
- no mutexes; assumes non-concurrent callback execution (single-threaded executor typical).
- maps are append/update only; no TTL eviction => long runs grow memory.

---

## E) Join keys & data model

Recommended stable join keys:
- `run_id` (all files).
- `msg_id` (messages/recovery).
- `request_msg_id` in `charge_events` joins to `messages.msg_id` and CHARGE_DECISION `ref_msg_id` semantics.
- entity ids: `uav_id`, `ugv_id`, `src_id`, `dst_id`.
- safest time for lifecycle join = `messages.creation_time_s` + `messages.delivered_time_s` (same table), then align with timeseries using `merge_asof`.

Primary key guidance:
- `messages`: (`run_id`,`msg_id`) expected unique; duplicates possible if producer reuses ids (not enforced).
- `charge_events`: (`run_id`,`request_msg_id`) expected unique.
- `preemption_events`: no natural strict key; use (`run_id`,`time`,`victim_uav_id`,`winner_uav_id`).
- `status_timeseries`: (`run_id`,`time`,`uav_id`) not guaranteed unique (extra writes on `writeOutputs`).
- `network_timeseries`, `weather_timeseries`, `queue_timeseries`: `time` not strictly unique.

Join strategy:
- exact joins for id-based tables (`messages`↔`charge_events`↔`recovery_events`).
- time-tolerant joins (`merge_asof`) for timeseries with tolerance 0.5–1.0s (INFERRED from timer periods ~1s).

---

## F) Concrete union/intersection recommendations

1) **Message lifecycle dataset**
- sources: `messages.csv` (+ optionally filter control_type in same table), `qos_metrics.csv`, `network_timeseries.csv`.
- keys: exact (`run_id`,`msg_id`); group by (`flow_type`,`control_type`).
- computed: latency_ms, delivered flag, dropped reason categories, hop distributions, per-control PDR.
- pitfall: dropped and delivered can both appear false for in-flight messages.
- pseudo-query:
```sql
SELECT m.run_id,m.msg_id,m.control_type,m.src_id,m.dst_id,m.e2e_delay_ms,m.hop_count,m.dropped,m.drop_reason
FROM messages m;
```

2) **Charging episode dataset**
- sources: `charge_events.csv` + `charge_queue_timeseries.csv` + `status_timeseries.csv` + `death_events.csv`.
- keys: exact on (`run_id`,`request_msg_id`) for message linkage; `merge_asof` on (`run_id`,`uav_id`,`time`).
- computed: waiting distributions, fairness by role, preemption victim/winner rates, success/failure.
- pitfall: role may be unknown (`-1`), many duration fields `-1` sentinel.

3) **Routing stability / recovery dataset**
- sources: `recovery_events.csv` + `messages.csv` (DROP reasons, delivery) + `network_timeseries.csv`.
- keys: exact msg_id for recovery control messages; time-asof for network quality context.
- computed: recovery churn by epoch/control_type, route-break symptom correlation via drop reasons.

---

## G) Coverage gaps & instrumentation TODO (no patches)

1. **Missing raw hop timestamps in exports**
- Why: monitor uses `last_rx_time` for delivery but does not persist `last_rx_time`/`last_tx_time` columns.
- Patch location: `writeMessagesCsv`.
- Add columns: `last_rx_time_s`, `last_tx_time_s` (float seconds, `-1` sentinel).

2. **No explicit clock-domain column**
- Why: difficult to diagnose sim/wall-time mismatch.
- Patch: all writers; add `clock_domain` enum string (`ROS_TIME`/`SYSTEM_TIME`).

3. **No explicit drop reason taxonomy enforcement**
- Why: free-text drop reasons complicate joins.
- Patch: drop producers (`uav_node`, `fault_injector_node`, `ugv_charger_node`) and monitor normalization map.
- Add: `drop_reason_code` categorical + `drop_reason_text`.

4. **`flush_period_sec` launch param mismatch**
- Why: launch sets `flush_period_sec`, monitor declares `csv_write_period_sec`; potential silent default use.
- Patch: `experiment.launch.py` monitor param name or monitor declare alias.

5. **No routing table snapshot artifacts despite subscribed topic**
- Why: routing stability analysis lacks ground-truth route state.
- Patch: `routingTableCallback` currently empty; add `routing_table_timeseries.csv` with route epoch/next-hop matrix metadata.

