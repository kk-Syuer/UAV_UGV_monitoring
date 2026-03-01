# Network Monitoring Data Lineage — v2 Code-Grounded Edition

**Source of truth:** `src/network_monitor/src/network_monitor_node.cpp`
All line numbers cited below refer to that file in the current HEAD of branch
`claude/upgrade-data-collection-logging-IVIvi`.

Legend:
- **CONFIRMED** = column value or logic directly visible in cited source lines.
- **INFERRED** = behaviour reconstructed from surrounding code where no single
  line is definitive.
- `sentinel -1.0` / `sentinel -1` = value written when the datum is unknown or
  not applicable; chosen to be distinguishable from any valid measurement.

---

## 1. System-level Data-Flow Map

```
PUBLISHERS                   TOPICS                  MONITOR CALLBACKS
───────────────────────────────────────────────────────────────────────
UAV fleet node               /fanet/network_bus_raw  trafficRawCallback()
Fault injector (clone)       /fanet/network_bus      trafficCallback()
Sink gateway (clone)         /fanet/delivered        deliveredCallback()
UAV fleet node               /fanet/status           statusCallback()
UAV fleet node               /uav_fleet/charge_requests  chargeRequestCallback()
UGV charger node             /ugv/charge_decisions   chargeDecisionCallback()
UGV charger node (JSON str)  /ugv/queue_events       queueEventCallback()
UGV charger node (JSON str)  /ugv/charging_snapshot  chargingSnapshotCallback()
UGV charger node             /fanet/network_bus      trafficCallback() [CHARGE_DECISION]
Weather node                 /environment/weather    weatherCallback()
(any node)                   /fanet/routing_table    routingTableCallback() [STUB]

TIMERS                       PERIOD          WRITER
───────────────────────────────────────────────────────────────────────
csv_timer_                   csv_write_period_sec    writeOutputs()
                             default 10.0 s
charge_timeout_timer_        1 s             checkChargeTimeouts()
status_timeseries_timer_     status_sample_period_sec (default 1.0 s)
                                             writeStatusTimeseriesRow()
weather_timeseries_timer_    status_sample_period_sec (default 1.0 s)
                                             writeWeatherTimeseriesRow()
queue_timeseries_timer_      queue_stats_period_sec (default 1.0 s)
                                             writeChargeQueueTimeseriesRow()
network_timeseries_timer_    status_sample_period_sec (default 1.0 s)
                                             writeNetworkTimeseriesRow()
stats_timer_                 0.5 s           publishNetworkStats()

SHUTDOWN HOOKS
───────────────────────────────────────────────────────────────────────
rclcpp::on_shutdown lambda (L297)   → writeOutputs(true)
~NetworkMonitorNode() destructor (L305) → writeOutputs(true)

OUTPUTS
───────────────────────────────────────────────────────────────────────
All CSV/JSONL files: output_root_ = output_dir_ / run_id_
Live topic: /network_monitor/stats  (std_msgs/String, JSON, 2 Hz)
```

---

## 2. Output Artifacts Inventory

All files under `output_root_ = output_dir / run_id`.
File mode is exclusively `std::ios::app`.  Header is written only if the file is
absent or has byte-size 0 (checked via `openAppend()` at L841-L857).

| # | File | Write function | Primary trigger | Dedup guard | Mode |
|---|------|----------------|-----------------|-------------|------|
| 1 | `packet_generated_events.csv` | `logPacketGeneratedEvent()` L863 | `trafficRawCallback()` per new msg_id | `already_logged_generated_` set | APPEND |
| 2 | `packet_delivered_events.csv` | `logPacketDeliveredEvent()` L892 | `deliveredCallback()` per new msg_id | `already_logged_delivered_` set | APPEND |
| 3 | `packet_drop_events.csv` | `logPacketDropEvent()` L923 | `trafficCallback()` DROP branch per new ref_msg_id | `dropped_ids_` set | APPEND |
| 4 | `packet_ack_events.csv` | `logPacketAckEvent()` L944 | `trafficCallback()` ACK branch per new ref_msg_id | `ack_by_ref_` map (first-insert) | APPEND |
| 5 | `charge_request_events.csv` | `logChargeRequestEvent()` L964 | `trafficCallback()` CHARGE_REQUEST, `is_new_request` guard | `ChargeRecord.request_msg_id` empty check | APPEND |
| 6 | `charge_decision_events.csv` | `logChargeDecisionEvent()` L989 | `deliveredCallback()` CHARGE_DECISION per new ref_msg_id | `already_logged_charge_decision_` set | APPEND |
| 7 | `charge_session_events.csv` | `logChargeSessionEvent()` L1023 | State machine transitions in `statusCallback`, `checkChargeTimeouts`, `handleFailureFromTraffic` | State machine gate (each transition fires at most once) | APPEND |
| 8 | `messages.csv` | `writeMessagesCsv()` L1348 | `writeOutputs()` periodic + shutdown, per unseen msg_id | `exported_messages_` set | APPEND |
| 9 | `qos_metrics.csv` | `writeQosMetricsCsv()` L1397 | `writeOutputs()` periodic + shutdown, all categories every flush | None (timeseries; every flush writes all) | APPEND |
| 10 | `charge_events.csv` | `writeChargeEventsCsv()` L1440 | `writeOutputs()` periodic + shutdown, terminal records only | `exported_charge_requests_` set + `isTerminalOutcome()` | APPEND |
| 11 | `preemption_events.csv` | `writePreemptionEventsCsv()` L1535 | `writeOutputs()` periodic + shutdown, incremental index | `exported_preemption_count_` cursor | APPEND |
| 12 | `recovery_events.csv` | `writeRecoveryEventsCsv()` L1572 | `writeOutputs()` periodic + shutdown, per unseen msg_id | `exported_recovery_events_` set | APPEND |
| 13 | `status_timeseries.csv` | `writeStatusTimeseriesRow()` L1747 | `status_timeseries_timer_` (1 s) + `writeOutputs()` | None (timeseries) | APPEND |
| 14 | `charge_queue_timeseries.csv` | `writeChargeQueueTimeseriesRow()` L1866 | `queue_timeseries_timer_` (1 s) + `writeOutputs()` | None (timeseries) | APPEND |
| 15 | `weather_timeseries.csv` | `writeWeatherTimeseriesRow()` L2022 | `weather_timeseries_timer_` (1 s) + `writeOutputs()` | Guard: `weather_received_` bool | APPEND |
| 16 | `death_events.csv` | `writeDeathEventsCsv()` L2050 | `writeOutputs()` periodic + shutdown | `exported_death_event_count_` cursor | APPEND |
| 17 | `network_timeseries.csv` | `writeNetworkTimeseriesRow()` L2084 | `network_timeseries_timer_` (1 s) + `writeOutputs()` | None (timeseries) | APPEND |
| 18 | `summary_snapshots.jsonl` | `writeSummarySnapshotJsonl()` L2174 | `writeOutputs()` periodic + shutdown | None (one JSON object per call) | APPEND |
| 19 | `/network_monitor/stats` (topic) | `publishNetworkStats()` L1059 | `stats_timer_` (0.5 s) | None | Publish |

---

## 3. Universal Run-Metadata Columns

Every CSV row begins with the following six columns (written by `runMeta()` L833
plus the caller's `t_rel_s` and `time_s`).

| Column | C++ variable | Source | Type | Notes |
|--------|-------------|--------|------|-------|
| `run_id` | `run_id_` | `declare_parameter("run_id","run0")` L201 | string | Passed from launch; identifies the experiment run. |
| `protocol_name` | `protocol_name_` | `declare_parameter("protocol_name","unknown")` L203 | string | Charging-scheduling protocol label (e.g. `FCFS`, `PRIORITY_CH_FIRST`). Passed from launch. |
| `replicate_id` | `replicate_id_` | `declare_parameter("replicate_id",0)` L204 | int | Which replicate within the protocol experiment (1-based or 0-based, determined by launch config). |
| `run_instance_id` | `run_instance_id_` | `std::chrono::system_clock::now()` at constructor L221-L223 | string (nanoseconds) | Wall-clock ns since Unix epoch at process startup. Unique per process execution; distinguishes crashes/restarts sharing the same `run_id`. |
| `t_rel_s` | computed by `tRelS()` / `tRelSAt()` L819-L828 | `(event_time - start_time_).seconds()` | float64 | Seconds since node constructor ran (`start_time_ = this->now()` L219). Use for cross-replicate time alignment. |
| `time_s` | computed inline per writer | `this->now().seconds()` or event-specific ROS clock | float64 | Absolute ROS clock seconds at write time or at the event. See per-file notes for exact value used. |

---

## 4. Primary In-Memory Data Structures

Understanding these structures is essential for tracing every output column to
its source.

### 4.1 `MsgRecord` (L27-L51)

One entry per unique `msg_id` in `records_` (L2511).

| Field | Type | Populated by | Source value |
|-------|------|-------------|-------------|
| `msg_id` | string | `trafficRawCallback` L423 / `trafficCallback` L396 / `deliveredCallback` L502 | `msg->msg_id` |
| `ref_msg_id` | string | same callbacks | `msg->ref_msg_id` (non-empty for ACK/DROP/DECISION) |
| `flow_type` | uint8 | same callbacks | `msg->flow_type` (0=DATA, 1=CONTROL) |
| `control_type` | string | same callbacks | `msg->control_type` |
| `src_id` | string | same callbacks | `msg->src_id` |
| `dst_id` | string | same callbacks | `msg->dst_id` |
| `creation_time` | rclcpp::Time | same callbacks | `rclcpp::Time(msg->creation_time)` — producer ROS clock |
| `first_seen_bus_time` | rclcpp::Time | `trafficRawCallback` L431 | `this->now()` — monitor ROS clock |
| `delivered_time` | rclcpp::Time | `deliveredCallback` L521 | `delivered_wall_time` (see §6.1) |
| `ack_time` | rclcpp::Time | `reconcileCausality()` L1343 | `ack_by_ref_[msg_id]` = monitor `now()` at ACK observation |
| `forward_count` | int | `trafficCallback` L408 | incremented every time `msg_id` is seen on `/fanet/network_bus` |
| `hop_count` | int | `deliveredCallback` L523 | `msg->hop_count` (set to -1 until delivery) |
| `ttl_hops` | int | `deliveredCallback` L524 | `msg->ttl` (remaining TTL at delivery) |
| `delivered` | bool | `deliveredCallback` L520 | `true` on first delivery |
| `dropped` | bool | `reconcileCausality()` L1337 | `true` if `msg_id` appears in `drop_by_ref_` |
| `drop_reason` | string | `reconcileCausality()` L1339 | `drop_by_ref_[msg_id].first` |
| `dropper_id` | string | `reconcileCausality()` L1340 | `drop_by_ref_[msg_id].second` |
| `payload_bytes` | size_t | `trafficRawCallback` L430 | `msg->payload.size()` |
| `generated_counted` | bool | `trafficRawCallback` L432-L435 / `deliveredCallback` L511-L514 | set `true` when counted in `total_generated_` |

### 4.2 `ChargeRecord` (L103-L136)

One entry per unique CHARGE_REQUEST `msg_id` in `charge_records_` (L2571).

| Field | Type | Populated by | Source value |
|-------|------|-------------|-------------|
| `request_msg_id` | string | `trafficCallback` CHARGE_REQUEST L359 | `msg->msg_id` |
| `uav_id` | string | `trafficCallback` L360 / `deliveredCallback` L462 | `msg->src_id` (REQUEST) or `msg->dst_id` (DECISION) |
| `ugv_id` | string | `trafficCallback` L361 / `deliveredCallback` L464 | `msg->dst_id` (REQUEST) or `msg->src_id` (DECISION) |
| `request_time` | rclcpp::Time | `trafficCallback` L364 | `rclcpp::Time(msg->creation_time)` — producer ROS clock of the CHARGE_REQUEST message |
| `decision_time` | rclcpp::Time | `deliveredCallback` L465 | `delivered_wall_time` at CHARGE_DECISION delivery |
| `dock_start_time` | rclcpp::Time | `statusCallback` L725 | monitor `this->now()` when charging_state transitions from ACCEPTED to 1 or 2 |
| `charge_end_time` | rclcpp::Time | `statusCallback` L741 | monitor `this->now()` when charging_state transitions out of 2 (CHARGING) |
| `outcome` | ChargeOutcome | multiple (see §8) | state machine |
| `failure_reason` | string | multiple | "PREEMPTED", "REJECTED", "RETURNED_BEFORE_DOCK", "NO_DECISION", "ENERGY_DEPLETED", "UNKNOWN_DROP", or drop_reason string |
| `role` | uint8 | `statusCallback` L716 / `trafficCallback` via `latest_role_by_uav_` L366 | `UavStatus.role` — 0=MEMBER, 1=CH, 2=BACKUP_CH |
| `role_known` | bool | same | set `true` once role is populated |
| `decision_policy` | string | `parseDecisionRationale()` L1207 | key `policy` in CHARGE_DECISION payload semicolon-delimited |
| `decision_priority` | int | `parseDecisionRationale()` L1212 | key `priority` — scheduling priority computed by UGV |
| `decision_tte_sec` | double | `parseDecisionRationale()` L1217 | key `tte_sec` — estimated time-to-empty at request |
| `decision_score` | double | `parseDecisionRationale()` L1219 | key `score` — composite priority score |
| `decision_rank_index` | int | `parseDecisionRationale()` L1213 | key `rank_index` — position in sorted queue |
| `decision_queue_size` | int | `parseDecisionRationale()` L1215 | key `queue_size` — queue length at decision time |
| `decision_ctrl_pdr` | double | `fillDecisionNetworkContext()` L1237 | `QosMetrics.pdr` for CONTROL/CHARGE_DECISION category |
| `decision_ctrl_delay_mean_ms` | double | `fillDecisionNetworkContext()` L1238 | `QosMetrics.delay_mean_ms` |
| `decision_ctrl_delay_p95_ms` | double | `fillDecisionNetworkContext()` L1239 | `QosMetrics.delay_p95_ms` |
| `decision_ctrl_drop_reasons` | string | `fillDecisionNetworkContext()` L1240 | pipe-delimited `reason:count` pairs |
| `request_battery` | double | `trafficCallback` via `latest_request_battery_by_uav_` L376 | `ChargeRequest.battery_level` (received on `/uav_fleet/charge_requests`) |
| `start_battery` | double | `statusCallback` L726 | `UavStatus.battery_level` at DOCK_START |
| `end_battery` | double | `statusCallback` L742 | `UavStatus.battery_level` at DOCK_END (charging_state exits 2) |
| `charge_completed` | bool | `statusCallback` L740 | `true` when charging_state transitions out of 2 |
| `preempted_flag` | bool | `deliveredCallback` L486 / `recordPreemptionFromDecision()` L1307 | `true` if reason=PREEMPTED in payload |
| `preempt_count` | int | `recordPreemptionFromDecision()` L1308 | incremented per preemption DECISION delivered |
| `terminal_time` | rclcpp::Time | multiple (see §8) | monitor `this->now()` at the moment outcome becomes terminal |

### 4.3 `UavState` (L138-L149)

Keyed by `uav_id` in `uav_states_` (L2575). Fully overwritten on each `/fanet/status` message.

| Field | Source msg field | Code line |
|-------|-----------------|-----------|
| `charging_state` | `UavStatus.charging_state` (uint8; 0=ACTIVE,1=GOING,2=CHARGING,3=RETURNING) | L696 |
| `battery_level` | `UavStatus.battery_level` (float32, 0-100%) | L697 |
| `role` | `UavStatus.role` | L698 |
| `backbone_active` | `UavStatus.backbone_active` | L699 |
| `x`, `y`, `z` | `UavStatus.pose.position.{x,y,z}` | L700-L702 |
| `energy_consumption_rate` | `UavStatus.energy_consumption_rate` (float32, Wh/s) | L703 |
| `battery_capacity` | `UavStatus.battery_capacity` (float32, Wh total) | L704 |

### 4.4 `QosAggregate` (L151-L162)

Built transiently by `buildQosStats()` (L1717) from `records_`. Keyed by
`"flow_type:control_type"` string. Never persisted directly; `finalizeQosStats()`
(L1655) derives `QosMetrics` from it.

| Field | Source |
|-------|--------|
| `generated` | count of all `MsgRecord` entries for this key |
| `delivered` | count where `MsgRecord.delivered == true` |
| `dropped` | count where `MsgRecord.dropped == true` |
| `generated_bytes` | sum of `MsgRecord.payload_bytes` |
| `delivered_bytes` | sum of `MsgRecord.payload_bytes` where delivered |
| `delays_ms` | vector of `(delivered_time, delay_ms)` pairs where delivered |
| `drop_reasons` | map from `drop_reason` string to count |
| `first_delivered` / `last_delivered` | earliest/latest `delivered_time` |

### 4.5 `PreemptionEvent` (L164-L177)

Stored in `preemption_events_` vector (L2548). Populated by `queueEventCallback()`
(L1261) parsing the `event=PREEMPTION` string from `/ugv/queue_events`.

| Field | Source |
|-------|--------|
| `time` | monitor `this->now()` at callback invocation L1267 |
| `victim_uav_id` | key `victim_uav_id` in event string |
| `winner_uav_id` | key `winner_uav_id` |
| `victim_role`, `winner_role` | keys `victim_role`, `winner_role` (uint8) |
| `victim_priority`, `winner_priority` | keys (double) |
| `delta_priority` | key (double; winner_priority − victim_priority typically) |
| `victim_charge_time_s` | key (double; time victim has been waiting) |
| `policy` | key (string; scheduling policy name) |
| `ugv_id` | NOT populated by queueEventCallback (remains empty) |

### 4.6 `DeathEvent` (L179-L187)

Stored in `death_events_` vector (L2552). Populated by `handleFailureFromTraffic()`
(L610) when `failure_type == 1` (BATTERY_DEAD).

| Field | Source |
|-------|--------|
| `time` | `rclcpp::Time(msg.creation_time)` — producer clock of FAILURE_EVENT L619 |
| `uav_id` | `msg.src_id` L662 |
| `role` | `uav_states_[uav_id].role` L666 → then overridden by `latest_role_by_uav_` L672 if available |
| `role_known` | `true` if either source above found |
| `cause` | hard-coded `"BATTERY_DEAD"` L663 |
| `battery_at_death` | `uav_states_[uav_id].battery_level` L668 |

### 4.7 `RecoveryEvent` (L53-L67)

Stored in `recovery_events_` map (L2525). Populated by `trackRecoveryEvent()` (L2406).

| Field | control_types | Source |
|-------|--------------|--------|
| `msg_id` | all | `msg.msg_id` |
| `control_type` | all | `msg.control_type` |
| `src_id` | all | `msg.src_id` |
| `dst_id` | all | `msg.dst_id` |
| `creation_time` | all | `rclcpp::Time(msg.creation_time)` |
| `epoch` | RECOVERY_START, RECOVERY_DONE | `std::stoi(msg.payload)` |
| `member_id` | CLUSTER_REASSIGN, TASK_ASSIGN, RESPAWN_COMPLETED | `msg.dst_id` (CLUSTER_REASSIGN/TASK_ASSIGN) or `msg.src_id` (RESPAWN_COMPLETED) |
| `ch_id` | CLUSTER_REASSIGN, NEW_DEPLOYMENT | `msg.payload` (CLUSTER_REASSIGN) or `msg.dst_id` (NEW_DEPLOYMENT) |
| `task_count` | TASK_ASSIGN | `countTaskAssignPoints(msg.payload)` — semicolon-separated segments |
| `x`, `y`, `z` | NEW_DEPLOYMENT | `parseDeploymentPose(msg.payload, ...)` — comma-separated floats |

### 4.8 `UgvChargingSnapshot` (L2576-L2588)

Single-instance (`latest_ugv_charging_snapshot_`). Updated by `chargingSnapshotCallback()` (L1845) parsing JSON from `/ugv/charging_snapshot`.

| Field | Key in JSON payload |
|-------|-------------------|
| `active_sessions_count` | `"active_sessions_count"` (int) |
| `queue_length` | `"queue_length"` (int) |
| `max_parallel_spots` | `"max_parallel_spots"` (int) |
| `time` | `"time"` (double) |

Each field has a companion `_valid` boolean set to `true` only when the key was
successfully parsed (L1854-L1861). The UGV-side snapshot takes precedence over
the monitor's own inference in `writeChargeQueueTimeseriesRow()` (L1962-L1972).

---

## 5. Per-File Column Schemas

### 5.1 `packet_generated_events.csv`

**Trigger:** `trafficRawCallback()` on `/fanet/network_bus_raw`, first time a
`msg_id` is seen (guard: `already_logged_generated_` set at L866).
**Drop/ACK messages are skipped** before the guard (L417-L419).
**Semantic:** One row = one generated packet (raw bus = pre-fault-injection).

| Column | Code location | Source value | Clock | Missing sentinel |
|--------|-------------|-------------|-------|-----------------|
| *(run metadata ×6)* | L878-L879 | see §3 | — | — |
| `msg_id` | L880 | `msg.msg_id` | — | never empty (guard) |
| `flow_type` | L881 | `msg.flow_type` as int (0=DATA, 1=CONTROL) | — | never |
| `control_type` | L882 | `msg.control_type`; empty string for DATA packets | — | `""` for DATA |
| `src_id` | L883 | `msg.src_id` (UAV/UGV/user/sink node id) | — | `""` if unset |
| `dst_id` | L884 | `msg.dst_id` (intended final recipient) | — | `""` if broadcast |
| `creation_time_s` | L877, L885 | `rclcpp::Time(msg.creation_time).seconds()` | **Producer ROS clock** | 0.0 if timestamp unset |
| `payload_bytes` | L886 | `msg.payload.size()` (bytes) | — | 0 if empty payload |

**`time_s`** (L875) = monitor `this->now().seconds()` at callback invocation —
slightly later than `creation_time_s` due to network transit.

**`t_rel_s`** (L876) = `tRelSAt(observed_at)` = `(observed_at − start_time_).seconds()`.

---

### 5.2 `packet_delivered_events.csv`

**Trigger:** `deliveredCallback()` on `/fanet/delivered`, first time a `msg_id`
is seen (guard: `already_logged_delivered_` at L896).
**Semantic:** One row = one successfully end-to-end delivered packet.

| Column | Code location | Source value | Clock | Missing sentinel |
|--------|-------------|-------------|-------|-----------------|
| *(run metadata ×6)* | L910-L911 | see §3 | — | — |
| `msg_id` | L912 | `msg.msg_id` | — | never |
| `delivered_time_s` | L907-L909, L913 | If `msg.last_rx_time` is non-zero: `rclcpp::Time(msg.last_rx_time).seconds()`; else: `delivered_at.seconds()` | **UAV ROS clock** (preferred) or **Monitor ROS clock** (fallback) | — |
| `receiver_id` | L914 | `msg.dst_id` — final recipient node id | — | `""` |
| `hop_count` | L915 | `rec.hop_count` = `msg.hop_count` from delivered message | — | -1 if not yet set at logging time |
| `ttl_hops` | L916 | `rec.ttl_hops` = `msg.ttl` from delivered message | — | -1 |
| `delivered_flag` | L917 | hard-coded `"true"` | — | always `true` |

**`time_s`** (L904) = monitor `this->now().seconds()` at `deliveredCallback`.
**`t_rel_s`** (L905) = `tRelSAt(delivered_at)`.

**E2E delay computation:** `delivery_delay = delivered_time_s − creation_time_s`
where `creation_time_s` comes from `packet_generated_events.csv` (join on `msg_id`).
Both use the ROS clock which should be synchronised (use_sim_time).

---

### 5.3 `packet_drop_events.csv`

**Trigger:** `trafficCallback()` DROP branch (L320-L342) on `/fanet/network_bus`.
Guard: `dropped_ids_` set — only the first DROP per `ref_msg_id` is logged.
**Semantic:** One row = one unique packet lost in transit.

| Column | Code location | Source value | Clock | Missing sentinel |
|--------|-------------|-------------|-------|-----------------|
| *(run metadata ×6)* | L934-L935 | see §3 | — | — |
| `ref_msg_id` | L936 | `msg.ref_msg_id` — references the original `msg_id` that was dropped | — | never (DROP always has ref) |
| `drop_reason` | L937 | `msg.drop_reason` if non-empty, else `"UNKNOWN"` | — | `"UNKNOWN"` |
| `dropper_id` | L938 | `msg.src_id` — node that emitted the DROP control message | — | `""` |

**`time_s`** (L932) = monitor `this->now().seconds()` at DROP observation.
**`t_rel_s`** (L933) = `tRelSAt(observed_at)`.

**Known drop reasons (from fault injector / router):**
`WEATHER_DROP`, `TTL_EXPIRED`, `NO_ROUTE`, `BUFFER_FULL` (varies by simulator).

---

### 5.4 `packet_ack_events.csv`

**Trigger:** `trafficCallback()` ACK branch (L345-L354). Guard: `ack_by_ref_`
map — `insert` only succeeds on first ACK per `ref_msg_id`.
**Semantic:** One row = one link-layer or app-layer acknowledgement observed.

| Column | Code location | Source value | Clock | Missing sentinel |
|--------|-------------|-------------|-------|-----------------|
| *(run metadata ×6)* | L954-L955 | see §3 | — | — |
| `ref_msg_id` | L956 | `msg.ref_msg_id` | — | never |
| `ack_time_s` | L957 | `time_s` (= monitor `now()`) — **Monitor ROS clock** | Monitor | — |
| `ack_src_id` | L958 | `msg.src_id` — ACK sender | — | `""` |

**`time_s`** (L952) = monitor `this->now().seconds()`.
**Note:** `ack_time_s` and `time_s` are the same value (L957 copies `time_s`).

---

### 5.5 `charge_request_events.csv`

**Trigger:** `trafficCallback()` CHARGE_REQUEST branch (L356-L388), guarded by
`is_new_request` local bool (L358) — only fires when `ChargeRecord.request_msg_id`
was empty (first-time population).
**Semantic:** One row = one new charge request entered the monitor's awareness.

| Column | Code location | Source value | Clock | Missing sentinel |
|--------|-------------|-------------|-------|-----------------|
| *(run metadata ×6)* | L977-L978 | see §3 | — | — |
| `request_msg_id` | L979 | `msg.msg_id` — CHARGE_REQUEST msg_id | — | never |
| `uav_id` | L980 | `rec.uav_id` = `msg.src_id` | — | never |
| `role` | L976, L981 | `rec.role` if `role_known`; else `-1` | — | -1 |
| `battery_at_request` | L982 | `rec.request_battery` = `latest_request_battery_by_uav_[uav_id]` populated from `/uav_fleet/charge_requests` ChargeRequest.battery_level (L559) | — | -1.0 if no matching ChargeRequest received before the bus message |
| `request_time_s` | L975, L983 | `rec.request_time.seconds()` = `rclcpp::Time(msg.creation_time).seconds()` | **Producer ROS clock** | -1.0 if nanoseconds == 0 |

**`time_s`** (L973) = monitor `now()` at observation (later than `request_time_s`
by transit time).

**Role resolution order (L365-L375):**
1. `latest_role_by_uav_[uav_id]` (from `/uav_fleet/charge_requests` ChargeRequest.role)
2. `uav_states_[uav_id].role` (from `/fanet/status` UavStatus.role)
3. -1 (unknown)

---

### 5.6 `charge_decision_events.csv`

**Trigger:** `deliveredCallback()` CHARGE_DECISION branch (L456-L498).
Guard: `already_logged_charge_decision_` set keyed on `msg.ref_msg_id` (L995).
**Semantic:** One row = one charge scheduling decision delivered to a UAV.

| Column | Code location | Source value | Clock | Missing sentinel |
|--------|-------------|-------------|-------|-----------------|
| *(run metadata ×6)* | L1010-L1011 | see §3 | — | — |
| `request_msg_id` | L1012 | `msg.ref_msg_id` — back-reference to the CHARGE_REQUEST | — | never (DECISION always has ref) |
| `decision_msg_id` | L1013 | `msg.msg_id` — the DECISION message itself | — | never |
| `decision_time_s` | L1014 | `time_s` = `delivered_at.seconds()` | Monitor / UAV ROS clock (see §6.1) | — |
| `outcome` | L1008, L1015 | `"ACCEPTED"`, `"REJECTED"`, or `"PREEMPTED"` | — | never |
| `failure_reason` | L1009, L1016 | `"REJECTED"` or `"PREEMPTED"` or `""` (accepted) | — | `""` if accepted |
| `decision_latency_ms` | L1006-L1007, L1017 | `(delivered_at − rec.request_time).seconds() * 1000`; -1 if `rec.request_time` is zero | — | -1.0 |

**Preemption detection (L468):**
`is_preemption = msg.payload.find("reason=PREEMPTED") != npos`

**Acceptance detection (L469):**
`accepted = !is_preemption && msg.payload.find("accepted=0") == npos`

---

### 5.7 `charge_session_events.csv`

**Trigger:** `logChargeSessionEvent()` (L1023), called from four locations
for five distinct `ChargeSessionEventType` values.

| Event type | Calling site | Condition |
|-----------|-------------|-----------|
| `DOCK_START` | `statusCallback()` L731 | `rec.outcome == ACCEPTED` AND `msg->charging_state ∈ {1,2}` |
| `DOCK_END` | `statusCallback()` L750 | `rec.outcome == STARTED` AND `prev.charging_state == 2` AND `msg->charging_state != 2` |
| `PREEMPTED` | `statusCallback()` L763 | `rec.outcome ∈ {ACCEPTED,PENDING}` AND `msg->charging_state == 3` |
| `TIMEOUT` | `checkChargeTimeouts()` L785 | `rec.outcome ∈ {PENDING,ACCEPTED}` AND `rec.decision_time == 0` AND `wait > decision_timeout_sec_` |
| `ENERGY_DEPLETED` | `handleFailureFromTraffic()` L652 | `failure_type == 1` (BATTERY_DEAD) |

**Semantic:** One row = one lifecycle transition in a charging session.

| Column | Code location | Source value | Clock | Populated at | Missing sentinel |
|--------|-------------|-------------|-------|-------------|-----------------|
| *(run metadata ×6)* | L1043-L1044 | see §3 | — | all events | — |
| `request_msg_id` | L1045 | `rec.request_msg_id` | — | all | `""` if no matching request |
| `uav_id` | L1046 | `rec.uav_id` | — | all | `""` |
| `role` | L1042, L1047 | `rec.role` if `role_known`; else -1 | — | all | -1 |
| `event_type` | L1048 | `chargeSessionEventTypeToString(event_type)` | — | all | — |
| `event_time_s` | L1040, L1049 | `event_at.seconds()` — monitor `now()` (same as `time_s`) | Monitor ROS clock | all | — |
| `waiting_time_ms` | L1050 | At DOCK_START: `max(0, (now − rec.request_time).seconds() * 1000)` L729-L730; else -1 | — | DOCK_START only | -1.0 |
| `charge_duration_s` | L1051 | At DOCK_END: `max(0, (now − rec.dock_start_time).seconds())` L743-L744; else -1 | — | DOCK_END only | -1.0 |
| `energy_charged_wh` | L1052 | At DOCK_END: `(end_battery − start_battery) * battery_capacity / 100.0` if `battery_capacity > 0` AND `energy_delta >= 0`; else -1 (L745-L748) | — | DOCK_END only | -1.0 |
| `battery_before` | L1053 | At DOCK_START: `msg->battery_level`; at DOCK_END: `rec.start_battery`; else -1 | — | DOCK_START, DOCK_END | -1.0 |
| `battery_after` | L1054 | At DOCK_END: `msg->battery_level`; else -1 | — | DOCK_END only | -1.0 |
| `battery_capacity_wh` | L1055 | `state.battery_capacity` from `uav_states_[uav_id].battery_capacity` (float, Wh total) | — | all | 0.0 if never received |

**Energy formula (exact, L745-L748):**
```cpp
double energy_delta = (rec.start_battery >= 0.0)
    ? (msg->battery_level - rec.start_battery)  // percentage points
    : -1.0;
double energy_wh = (energy_delta >= 0.0 && state.battery_capacity > 0.0f)
    ? energy_delta * static_cast<double>(state.battery_capacity) / 100.0
    : -1.0;
```

---

### 5.8 `messages.csv`

**Trigger:** `writeMessagesCsv()` (L1348) called from `writeOutputs()`.
Each record is exported **once** (guarded by `exported_messages_` set L1364),
as a snapshot of `MsgRecord` state at the time of first export.
Records may be exported before DROP/ACK reconciliation completes, so
`dropped` / `drop_reason` / `ack_time_s` can be stale.  Use the atomic event
tables for authoritative drop/ACK data.

| Column | Code location | Source value | Clock | Missing sentinel |
|--------|-------------|-------------|-------|-----------------|
| *(run metadata ×6)* | L1372-L1373 | see §3; `t_rel_s` and `time_s` are `writeOutputs()` wall time | Monitor | — |
| `msg_id` | L1374 | `rec.msg_id` | — | — |
| `flow_type` | L1375 | `rec.flow_type` as int | — | — |
| `control_type` | L1376 | `rec.control_type` | — | `""` for DATA |
| `src_id` | L1377 | `rec.src_id` | — | `""` |
| `dst_id` | L1378 | `rec.dst_id` | — | `""` |
| `creation_time_s` | L1379 | `rec.creation_time.seconds()` | Producer ROS clock | 0.0 |
| `delivered_time_s` | L1371, L1380 | `rec.delivered_time.seconds()` if `rec.delivered`; else -1.0 | Monitor/UAV clock | -1.0 |
| `delivered` | L1381 | `"true"` or `"false"` | — | `"false"` |
| `e2e_delay_ms` | L1367-L1369 | `(rec.delivered_time − rec.creation_time).seconds() * 1000` if delivered; else -1.0 | — | -1.0 |
| `forward_count` | L1383 | `rec.forward_count` (times seen on `/fanet/network_bus`) | — | 0 |
| `hop_count` | L1384 | `rec.hop_count` (-1 until delivery) | — | -1 |
| `ttl_hops` | L1385 | `rec.ttl_hops` (remaining TTL at delivery) | — | -1 |
| `payload_bytes` | L1386 | `rec.payload_bytes` | — | 0 |
| `dropped` | L1387 | `"true"` if `rec.dropped` (from reconcileCausality) | — | `"false"` |
| `drop_reason` | L1388 | `rec.drop_reason` | — | `""` |
| `dropper_id` | L1389 | `rec.dropper_id` | — | `""` |
| `ack_time_s` | L1370, L1390 | `rec.ack_time.seconds()` if non-zero; else -1.0 | Monitor ROS clock | -1.0 |

---

### 5.9 `qos_metrics.csv`

**Trigger:** `writeQosMetricsCsv()` (L1397) called from `writeOutputs()`.
**Mode:** Snapshot timeseries — every flush writes one row per known
`(flow_type, control_type)` category with cumulative stats to that point.
Consumer should take the **last** row per `(run_id, run_instance_id, flow_type, control_type)`.

| Column | Code location | Computation | Missing sentinel |
|--------|-------------|-------------|-----------------|
| *(run metadata ×6)* | L1419-L1420 | — | — |
| `flow_type` | L1421 | from `buildQosStats()` key split | — |
| `control_type` | L1422 | from `buildQosStats()` key split | — |
| `generated` | L1423 | `QosAggregate.generated` = count of MsgRecords in this category | 0 |
| `delivered` | L1424 | `QosAggregate.delivered` | 0 |
| `dropped` | L1425 | `QosAggregate.dropped` | 0 |
| `pdr` | L1426 | `delivered / generated`; 0.0 if generated == 0 (L1665-L1667) | 0.0 |
| `delay_mean_ms` | L1427 | mean of all `(delivered_time − creation_time) * 1000` for delivered msgs (L1675-L1678) | -1.0 |
| `delay_p95_ms` | L1428 | 95th percentile via linear interpolation `percentile()` L1609 | -1.0 if no delivered msgs |
| `jitter_ms` | L1429 | mean absolute consecutive delay difference `computeJitterMs()` L1625 (sorted by delivered_time) | -1.0 if < 2 delivered |
| `throughput_bps` | L1430 | `delivered_bytes * 8 / duration_sec` (L1692); `duration_sec` = last−first delivered or run elapsed | -1.0 |
| `generated_bps` | L1431 | `generated_bytes * 8 / duration_sec` (L1693) | -1.0 |
| `qos_score` | L1432 | weighted composite: `(score_pdr * w_pdr + score_delay * w_delay + score_jitter * w_jitter) / weight_sum` (L1710-L1712) | 0.0 |

**QoS score sub-components (L1696-L1712):**
```
score_pdr   = min(pdr / qos_target_pdr_,   1.0)  [param default: target=0.95, weight=0.5]
score_delay = min(qos_target_delay_ms_ / delay_mean_ms, 1.0)  [target=200ms, weight=0.3]
score_jitter= min(qos_target_jitter_ms_ / jitter_ms,    1.0)  [target=50ms,  weight=0.2]
qos_score   = (score_pdr * w_pdr + score_delay * w_delay + score_jitter * w_jitter) / Σw
```
All three component scores are 0.0 if their denominator is ≤ 0.

---

### 5.10 `charge_events.csv`

**Trigger:** `writeChargeEventsCsv()` (L1440) from `writeOutputs()`.
Only records where `isTerminalOutcome(rec.outcome)` are exported (L1466-L1468).
Each record exported once via `exported_charge_requests_` (L1462, L1531).

`isTerminalOutcome()` (L1119-L1127) returns `true` for:
`STARTED`, `REJECTED`, `DROPPED`, `TIMEOUT`, `PREEMPTED`, `ENERGY_DEPLETED`.

| Column | Code location | Computation | Missing sentinel |
|--------|-------------|-------------|-----------------|
| *(run metadata ×6)* | L1496-L1497 | `t_rel_s` and `time_s` = `writeOutputs()` wall time | — |
| `request_msg_id` | L1498 | `rec.request_msg_id` | — |
| `uav_id` | L1499 | `rec.uav_id` | `""` |
| `ugv_id` | L1500 | `rec.ugv_id` | `""` |
| `role` | L1501 | `rec.role` if `role_known`; else -1 | -1 |
| `outcome` | L1502 | `chargeOutcomeToString(rec.outcome)`: `"PENDING"`,`"ACCEPTED"`,`"REJECTED"`,`"ROUTING_DROP"`,`"TIMEOUT"`,`"STARTED"`,`"PREEMPTED"`,`"ENERGY_DEPLETED"` | — |
| `failure_reason` | L1503 | `rec.failure_reason` | `""` if no failure |
| `request_time_s` | L1504 | `rec.request_time.seconds()` | 0.0 if never set |
| `decision_time_s` | L1505 | `rec.decision_time.seconds()` | 0.0 if no decision |
| `dock_start_time_s` | L1506 | `rec.dock_start_time.seconds()` | 0.0 if never docked |
| `charge_end_time_s` | L1507 | `rec.charge_end_time.seconds()` if non-zero; else -1.0 | -1.0 |
| `terminal_time_s` | L1493-L1494, L1508 | `rec.terminal_time.seconds()` if non-zero; else -1.0 | -1.0 |
| `decision_latency_ms` | L1469-L1471, L1509 | `(decision_time − request_time) * 1000`; -1 if either is zero | -1.0 |
| `waiting_time_ms` | L1472-L1474, L1510 | `(dock_start_time − request_time) * 1000`; -1 if either is zero | -1.0 |
| `charge_duration_ms` | L1475-L1477, L1511 | `(charge_end_time − dock_start_time) * 1000`; -1 if incomplete | -1.0 |
| `effective_wait_ms` | L1485-L1491, L1512 | `waiting_time_ms` if ≥ 0; else `(terminal_time − request_time) * 1000`; captures wait for all outcomes | -1.0 |
| `charge_completed` | L1513 | `"true"` / `"false"` | `"false"` |
| `request_battery` | L1514 | `rec.request_battery` (%) | -1.0 |
| `start_battery` | L1515 | `rec.start_battery` (%) | -1.0 |
| `end_battery` | L1516 | `rec.end_battery` (%) | -1.0 |
| `energy_recovered_pct` | L1478-L1480, L1517 | `rec.end_battery − rec.start_battery` (percentage points); -1 if incomplete | -1.0 |
| `preempted_flag` | L1518 | `"true"` / `"false"` | `"false"` |
| `preempt_count` | L1519 | `rec.preempt_count` | 0 |
| `decision_policy` | L1520 | `rec.decision_policy` from payload key `policy` | `""` |
| `decision_priority` | L1521 | `rec.decision_priority` from payload key `priority` | -1 |
| `decision_tte_sec` | L1522 | `rec.decision_tte_sec` from payload key `tte_sec` | -1.0 |
| `decision_score` | L1523 | `rec.decision_score` from payload key `score` | -1.0 |
| `decision_rank_index` | L1524 | `rec.decision_rank_index` from payload key `rank_index` | -1 |
| `decision_queue_size` | L1525 | `rec.decision_queue_size` from payload key `queue_size` | -1 |
| `decision_ctrl_pdr` | L1526 | cumulative PDR for CONTROL/CHARGE_DECISION from `fillDecisionNetworkContext()` | -1.0 |
| `decision_ctrl_delay_mean_ms` | L1527 | mean delay for CONTROL/CHARGE_DECISION | -1.0 |
| `decision_ctrl_delay_p95_ms` | L1528 | p95 delay for CONTROL/CHARGE_DECISION | -1.0 |
| `decision_ctrl_drop_reasons` | L1529 | pipe-delimited `reason:count` | `""` |

---

### 5.11 `preemption_events.csv`

**Trigger:** `writePreemptionEventsCsv()` (L1535) from `writeOutputs()`.
Writes new entries since `exported_preemption_count_` cursor (L1553).
Source: `PreemptionEvent` structs pushed by `queueEventCallback()` (L1289).

| Column | Code location | Source value | Missing sentinel |
|--------|-------------|-------------|-----------------|
| *(run metadata ×6)* | L1556-L1557 | `t_rel_s` = `tRelSAt(pe.time)` | — |
| `victim_uav_id` | L1558 | `pe.victim_uav_id` | `""` |
| `winner_uav_id` | L1559 | `pe.winner_uav_id` | `""` |
| `victim_role` | L1560 | `pe.victim_role` as int | 0 (default, may be unknown) |
| `winner_role` | L1561 | `pe.winner_role` as int | 0 |
| `victim_priority` | L1562 | `pe.victim_priority` (double) | 0.0 |
| `winner_priority` | L1563 | `pe.winner_priority` (double) | 0.0 |
| `delta_priority` | L1564 | `pe.delta_priority` (double) | 0.0 |
| `victim_charge_time_s` | L1565 | `pe.victim_charge_time_s` (double; time victim waited) | 0.0 |
| `policy` | L1566 | `pe.policy` | `""` |

**`time_s`** = `pe.time.seconds()` = monitor `now()` at `queueEventCallback()`.
**Note:** `ugv_id` field of `PreemptionEvent` is NOT written to CSV.

---

### 5.12 `recovery_events.csv`

**Trigger:** `writeRecoveryEventsCsv()` (L1572) from `writeOutputs()`.
Each `RecoveryEvent` exported once via `exported_recovery_events_` set (L1585).
Source: `trackRecoveryEvent()` called from `trafficCallback()` for
`flow_type == 1` control messages (L390-L392).

Recognised `control_type` values: `RECOVERY_START`, `RECOVERY_DONE`,
`CLUSTER_REASSIGN`, `TASK_ASSIGN`, `NEW_DEPLOYMENT`,
`EMERGENCY_RETURN_TRIGGERED`, `RESPAWN_COMPLETED`.

| Column | Code location | Source value | Missing sentinel |
|--------|-------------|-------------|-----------------|
| *(run metadata ×6)* | L1590-L1591 | `t_rel_s` = `tRelSAt(rec.creation_time)` | — |
| `msg_id` | L1592 | `rec.msg_id` | — |
| `control_type` | L1593 | `rec.control_type` | — |
| `src_id` | L1594 | `rec.src_id` | `""` |
| `dst_id` | L1595 | `rec.dst_id` | `""` |
| `epoch` | L1596 | `rec.epoch` (from payload int for RECOVERY_START/DONE) | -1 for other types |
| `member_id` | L1597 | `rec.member_id` (for CLUSTER_REASSIGN, TASK_ASSIGN, RESPAWN_COMPLETED) | `""` |
| `ch_id` | L1598 | `rec.ch_id` (for CLUSTER_REASSIGN, NEW_DEPLOYMENT) | `""` |
| `task_count` | L1599 | `rec.task_count` (semicolon-separated task count for TASK_ASSIGN) | 0 |
| `x`, `y`, `z` | L1600-L1602 | `rec.x/y/z` (comma-separated floats for NEW_DEPLOYMENT) | 0.0 |
| `creation_time_s` | L1589, L1603 | `rec.creation_time.seconds()` = producer ROS clock from msg | 0.0 |

**Note:** `time_s` (L1591) = `creation_s` (same as `creation_time_s`) here,
not monitor `now()`.

---

### 5.13 `status_timeseries.csv`

**Trigger:** `writeStatusTimeseriesRow()` (L1747) on `status_timeseries_timer_`
every `status_sample_period_sec_` seconds (default 1 s) and at `writeOutputs()`.
One row per UAV per tick.

| Column | Code location | Source value | Missing sentinel |
|--------|-------------|-------------|-----------------|
| *(run metadata ×6)* | L1761-L1762 | `t_rel_s` = `tRelS()` at tick; `time_s` = `this->now().seconds()` | — |
| `uav_id` | L1763 | key of `uav_states_` map | — |
| `role` | L1764 | `st.role` as int (0=MEMBER, 1=CH, 2=BACKUP_CH) | 0 |
| `charging_state` | L1765 | `st.charging_state` as int (0=ACTIVE, 1=GOING, 2=CHARGING, 3=RETURNING) | 0 |
| `battery_level` | L1766 | `st.battery_level` (float, 0-100%) | 0.0 |
| `backbone_active` | L1767 | `"true"` / `"false"` | `"false"` |
| `x` | L1768 | `st.x` = `UavStatus.pose.position.x` | 0.0 |
| `y` | L1769 | `st.y` = `UavStatus.pose.position.y` | 0.0 |
| `z` | L1770 | `st.z` = `UavStatus.pose.position.z` | 0.0 |
| `energy_consumption_rate` | L1771 | `st.energy_consumption_rate` (Wh/s) | 0.0 |

**Note:** `battery_capacity` is NOT written to status_timeseries — it is
only used internally for `charge_session_events.energy_charged_wh`.

---

### 5.14 `charge_queue_timeseries.csv`

**Trigger:** `writeChargeQueueTimeseriesRow()` (L1866) on `queue_timeseries_timer_`
every `queue_stats_period_sec_` seconds (default 1 s) and at `writeOutputs()`.
One row per tick (not per UAV).

**Data sources (precedence):**
The UGV-side snapshot (`latest_ugv_charging_snapshot_`) overrides monitor-inferred
values for `queue_length_total`, `active_charging_ugv_sessions`, and `ugv_dock_capacity`
if the snapshot was received AND the relevant fields parsed successfully (L1962-L1972).

| Column | Code location | Computation | Missing sentinel |
|--------|-------------|-------------|-----------------|
| *(run metadata ×6)* | L1998-L1999 | `t_rel_s` = `tRelS()`; `time_s` = `this->now().seconds()` | — |
| `queue_length_total` | L2000 | `queue_length_ugv`: monitor-inferred count of ChargeRecords with `request_time != 0`, not yet docked, not terminal (L1890-L1906) — OR UGV-side `queue_length` if snapshot received | 0 |
| `queue_length_ch` | L2001 | sub-count of above where `rec.role == 1` (CH) | 0 |
| `queue_length_member` | L2002 | sub-count where `rec.role != 1` (MEMBER or BACKUP_CH) | 0 |
| `queue_length_unknown` | L2003 | sub-count where `!rec.role_known` | 0 |
| `queue_length_total_check` | L1992-L1994, L2004 | `queue_length_total − (queue_length_ch + queue_length_member + queue_length_unknown)` — should be 0; non-zero = counting anomaly (UGV snapshot vs monitor inference mismatch) | 0 |
| `active_charging_ugv_sessions` | L2005 | monitor-inferred: `count(uav_states_[x].charging_state == 2)` for non-dead UAVs (L1923-L1938) — OR UGV-side `active_sessions_count` if snapshot received | 0 |
| `ugv_dock_capacity` | L2006 | `ugv_dock_capacity_` param — OR UGV-side `max_parallel_spots` if snapshot received | from param |
| `ugv_dock_utilization` | L2007 | `active_charging_ugv_sessions / ugv_dock_capacity`; -1 if capacity == 0 (L1982-L1985) | -1.0 |
| `mean_wait_ch_ms` | L2008 | mean of `(dock_start_time − request_time) * 1000` for all STARTED CH records in `charge_records_` (L1908-L1920, L1940-L1946) — **cumulative, not instantaneous** | -1.0 |
| `mean_wait_member_ms` | L2009 | same for MEMBER records | -1.0 |
| `active_mission_count` | L2010 | `count(uav_states_[x].charging_state == 0)` excluding dead UAVs | 0 |
| `active_charging_uav_status` | L2011 | `count(charging_state == 2)` from `uav_states_` excluding dead | 0 |
| `going_to_ugv_count_uav_status` | L2012 | `count(charging_state == 1)` | 0 |
| `returning_count_uav_status` | L2013 | `count(charging_state == 3)` | 0 |
| `current_dead_count` | L2014 | `dead_uavs_.size()` (UAVs that sent BATTERY_DEAD and not respawned) | 0 |
| `dead_event_count` | L2015 | `dead_uav_event_history_.size()` (cumulative deaths, not decremented on respawn) | 0 |
| `fleet_size` | L2016 | `uav_states_.size()` (UAVs seen at least once on `/fanet/status`) | 0 |
| `over_capacity_ugv` | L2017 | `max(0, active_charging_ugv_sessions − ugv_dock_capacity)` (L1974-L1978) | 0 |
| `status_vs_ugv_gap` | L2018 | `active_charging_uav_status − active_charging_ugv_sessions` (signed, L1979-L1980) | 0 |

**`queue_length_total_check` invariant:** When the UGV snapshot provides
`queue_length_total` and the monitor infers the CH/member/unknown breakdown, a
mismatch will appear as a non-zero `queue_length_total_check`. This does not
indicate a bug but a measurement-plane boundary: the UGV knows the authoritative
queue, the monitor infers role breakdown from ChargeRecords.

---

### 5.15 `weather_timeseries.csv`

**Trigger:** `writeWeatherTimeseriesRow()` (L2022) on `weather_timeseries_timer_`
every `status_sample_period_sec_` seconds and at `writeOutputs()`.
Guard: `weather_received_` (L2024) — no rows until first message from `/environment/weather`.

| Column | Code location | Source value | Missing sentinel |
|--------|-------------|-------------|-----------------|
| *(run metadata ×6)* | L2039-L2040 | `t_rel_s` = `tRelS()`; `time_s` = `this->now().seconds()` | — |
| `regime` | L2041 | `current_weather_regime_` = `WeatherStatus.regime` | `""` |
| `temperature_c` | L2042 | `current_weather_temp_c_` = `WeatherStatus.temperature_c` | 0.0 |
| `wind_speed` | L2043 | `current_weather_wind_speed_` = `WeatherStatus.wind_speed` | 0.0 |
| `wind_direction_deg` | L2044 | `current_weather_wind_dir_` = `WeatherStatus.wind_direction_deg` | 0.0 |
| `rain_intensity` | L2045 | `current_weather_rain_` = `WeatherStatus.rain_intensity` | 0.0 |

**Regime transitions** are logged to console at `RCLCPP_INFO` level (L1174) but
not to a separate CSV.

---

### 5.16 `death_events.csv`

**Trigger:** `writeDeathEventsCsv()` (L2050) from `writeOutputs()`.
Incremental cursor `exported_death_event_count_` (L2066, L2080).

| Column | Code location | Source value | Clock | Missing sentinel |
|--------|-------------|-------------|-------|-----------------|
| *(run metadata ×6)* | L2070-L2071 | `t_rel_s` = `tRelSAt(de.time)` | — | — |
| `uav_id` | L2072 | `de.uav_id` = `msg.src_id` of FAILURE_EVENT | — | — |
| `role` | L2073 | `de.role` if `role_known`; else -1 | — | -1 |
| `cause` | L2074 | `de.cause` = hard-coded `"BATTERY_DEAD"` | — | — |
| `battery_at_death` | L2075 | `de.battery_at_death` = `uav_states_[uav_id].battery_level` at time of failure event | — | -1.0 if UAV state not yet seen |
| `fleet_size` | L2076 | `uav_states_.size()` at write time (NOT at event time) | — | 0 |
| `alive_count` | L2068, L2077 | `fleet_size − min(death_index + 1, fleet_size)` (approximation; decrements by position in death_events_ vector) | — | 0 |

**`time_s`** (L2071) = `de.time.seconds()` = `rclcpp::Time(msg.creation_time).seconds()`
= **producer ROS clock** at FAILURE_EVENT creation.

**`alive_count` caveat:** Uses the vector index at write time, so if deaths are
written in batch, earlier deaths in the batch will have underestimated `alive_count`.
The correct Kaplan-Meier alive count requires sorting by `time_s` externally.

---

### 5.17 `network_timeseries.csv`

**Trigger:** `writeNetworkTimeseriesRow()` (L2084) on `network_timeseries_timer_`
every `status_sample_period_sec_` seconds and at `writeOutputs()`.
**Window:** Only records with `creation_time >= now − rate_window_sec_` (default 10 s, L2107).

| Column | Code location | Computation | Missing sentinel |
|--------|-------------|-------------|-----------------|
| *(run metadata ×6)* | L2152-L2153 | `t_rel_s` = `tRelSAt(now)`; `time_s` = `now.seconds()` | — |
| `window_sec` | L2154 | `rate_window_sec_` (param `network_stats_window_sec`, default 10.0) | — |
| `window_generated` | L2155 | count of records in window (L2110) | 0 |
| `window_delivered` | L2156 | count where `rec.delivered` in window (L2111-L2115) | 0 |
| `window_dropped` | L2157 | count where `rec.dropped` in window (L2116-L2118) | 0 |
| `window_pdr` | L2158 | `window_delivered / window_generated`; -1 if generated == 0 | -1.0 |
| `window_delay_mean_ms` | L2159 | mean of `(delivered_time − creation_time) * 1000` in window | -1.0 |
| `window_delay_p95_ms` | L2160 | p95 of same | -1.0 |
| `window_jitter_ms` | L2161 | `computeJitterMs()` on window delays | -1.0 |
| `ctrl_charge_req_generated` | L2162 | count of CHARGE_REQUEST in window | 0 |
| `ctrl_charge_req_delivered` | L2163 | count delivered among those | 0 |
| `ctrl_charge_req_pdr` | L2164 | ratio; -1 if generated == 0 | -1.0 |
| `ctrl_charge_dec_generated` | L2165 | count of CHARGE_DECISION in window | 0 |
| `ctrl_charge_dec_delivered` | L2166 | count delivered | 0 |
| `ctrl_charge_dec_pdr` | L2167 | ratio; -1 if generated == 0 | -1.0 |

**Note:** `window_generated` counts records by `creation_time` falling in window,
NOT by observation time. Packets generated within the window but not yet delivered
at write time are included in `window_generated` but not `window_delivered`.

---

### 5.18 `summary_snapshots.jsonl`

**Trigger:** `writeSummarySnapshotJsonl()` (L2174) from `writeOutputs()`.
One pretty-printed JSON object per call, appended with a trailing newline.
**Consumer:** Read last object with matching `run_instance_id` for final state.

**Top-level fields:**
- `run_id`, `protocol_name`, `replicate_id`, `run_instance_id`, `t_rel_s`, `time_s` (L2256-L2261)
- `fleet` object: `fleet_size`, `alive_count`, `current_dead_count`, `dead_event_count`, `survival_rate` (L2262-L2268)
- `charging` object:
  - `requests_total` = `charge_records_.size()`
  - `accepted`, `rejected`, `dropped`, `timeouts`, `started`, `preempted`, `energy_depleted` = per-outcome counts (L2184-L2208)
  - `success_rate` = `started / requests_total`
  - `decision_latency_ms` = `{mean, p95}` over all records with both times set
  - `waiting_time_ms` = `{mean}` over STARTED records
  - `energy_recovered` = `{mean}` of `(end_battery − start_battery)` for completed charges
- `charging_fairness` object:
  - `rejections_by_uav`: map of uav_id → rejection count
  - `timeouts_by_uav`: map of uav_id → timeout count
  - `max_waiting_time_ms_by_uav`: map of uav_id → max wait across all started sessions
- `network` object:
  - `qos_targets` and `qos_weights` (from params)
  - `by_category`: array of per-(flow_type, control_type) QosMetrics objects
- `recovery` object: `start`, `done`, `cluster_reassign`, `task_assign`, `new_deployment` counts

---

### 5.19 `/network_monitor/stats` Topic

**Trigger:** `publishNetworkStats()` (L1059) on `stats_timer_` at 2 Hz (0.5 s period).
Published as `std_msgs/String` (JSON) on `/network_monitor/stats`.
**NOT persisted to disk** — for live dashboards only.

Fields (L1080-L1112):
```json
{
  "generated_total": <total_generated_>,
  "delivered_total": <total_delivered_>,
  "drop_total": <drop_total_>,
  "ack_total": <ack_total_>,
  "generated_rate": <window-smoothed rate, msg/s>,
  "delivered_rate": <same>,
  "drop_rate": <same>,
  "window_sec": <rate_window_sec_>,
  "last_msg_age": <seconds since last raw bus msg>,
  "last_drop_age": <seconds since last drop>,
  "last_delivered_age": <seconds since last delivery>,
  "control_type_counts": { "<type>": <count>, ... },
  "drop_reason_counts": { "<reason>": <count>, ... }
}
```

---

## 6. Clock Domains and Timestamp Truth Table

Three distinct clock sources appear in the output data.

| Clock source | Domain | Who stamps it | How it appears in outputs |
|-------------|--------|--------------|--------------------------|
| **Producer ROS clock** | ROS time (sim or wall) of the UAV/UGV/sink node that created the message | `msg.creation_time` field in TrafficMessage | `creation_time_s` (generated events, messages.csv), `request_time_s` (charge events), `event_time` of FAILURE_EVENT (death_events) |
| **UAV receive ROS clock** | ROS time of the UAV node at message reception | `msg.last_rx_time` field (non-zero when set by forwarder) | `delivered_time_s` in packet_delivered_events (preferred); `deliveredCallback`'s `delivered_wall_time` fallback |
| **Monitor ROS clock** | ROS time of the monitor node itself | `this->now()` inside monitor callbacks and timers | `t_rel_s`, `time_s` (all tables), `event_time_s` (session events), `ack_time_s`, observation timestamps |
| **Monitor wall clock** | `std::chrono::system_clock` | at node constructor only | `run_instance_id_` string |

### 6.1 `delivered_wall_time` computation (L451-L454)

```cpp
rclcpp::Time delivered_wall_time =
  (msg->last_rx_time.sec == 0 && msg->last_rx_time.nanosec == 0)
  ? this->now()          // Monitor ROS clock fallback
  : rclcpp::Time(msg->last_rx_time);  // UAV receive ROS clock
```

This value is used as:
- `rec.delivered_time` (MsgRecord)
- `charge_rec.decision_time` (ChargeRecord, for CHARGE_DECISION)
- `time_s` in `packet_delivered_events.csv`
- `decision_time_s` in `charge_decision_events.csv`

### 6.2 Clock alignment guidance

All nodes should share `/use_sim_time=true` (or all `false`).
- If sim-time: all clocks are driven by the same `/clock` topic → sub-millisecond skew.
- If wall-time: UAV producer clock and monitor clock may diverge by OS jitter (typically < 1 ms on same host; larger if distributed).

**Use `t_rel_s` for cross-replicate alignment**, not `time_s`.
`t_rel_s = (event_time − start_time_).seconds()` computed entirely within the
monitor ROS clock domain, so it is independent of producer clock drift.

---

## 7. ChargeOutcome State Machine

The `ChargeOutcome` enum (L69-L78) transitions as follows.
`isTerminalOutcome()` (L1119-L1127) = `{STARTED, REJECTED, DROPPED, TIMEOUT, PREEMPTED, ENERGY_DEPLETED}`.

```
           trafficCallback
           CHARGE_REQUEST
                │
                ▼
          ┌──PENDING──┐
          │            │  checkChargeTimeouts (1 s)
          │            │  wait > decision_timeout_sec_ && decision_time == 0
          │            ▼
          │         TIMEOUT ★
          │
          │  deliveredCallback  CHARGE_DECISION
          │  accepted=0
          ├──────────────────► REJECTED ★
          │
          │  deliveredCallback  CHARGE_DECISION
          │  reason=PREEMPTED
          ├──────────────────► PREEMPTED ★
          │
          │  deliveredCallback  CHARGE_DECISION
          │  accepted (neither of above)
          ▼
        ACCEPTED
          │
          │  statusCallback
          │  charging_state ∈ {1,2}
          ├──────────────────► STARTED ★
          │     │
          │     │  statusCallback
          │     │  charging_state == 3 (RETURNING)
          │     ▼  while ACCEPTED/PENDING
          │  PREEMPTED ★  (RETURNED_BEFORE_DOCK)
          │
          │  trafficCallback
          │  DROP on CHARGE_REQUEST ref_msg_id
          ▼
        DROPPED ★
          │
          │  handleFailureFromTraffic
          │  BATTERY_DEAD
          ▼
      ENERGY_DEPLETED ★  (set on whichever active record the UAV had)

★ = terminal
```

`terminal_time` is set to monitor `this->now()` at the moment of each terminal transition.
For `STARTED`, `terminal_time` is set at DOCK_START (L727) and is NOT the final end of charging.

---

## 8. QoS Computation Detail

### 8.1 `buildQosStats()` (L1717-L1745)

Iterates all `records_` entries. Groups by key `"<flow_type>:<control_type>"`.
For each record:
- Increments `generated` and `generated_bytes`.
- If `delivered`: increments `delivered`, `delivered_bytes`; appends `(delivered_time, delay_ms)`.
- If `dropped`: increments `dropped`, increments `drop_reasons[drop_reason]`.

### 8.2 `finalizeQosStats()` (L1655-L1715)

Consumes a `QosAggregate` and produces `QosMetrics`:

```
pdr           = delivered / generated                     [0.0, 1.0]
delay_mean_ms = mean of delays_ms values                  [-1 if none]
delay_p95_ms  = percentile(delays_ms, 95.0)               [-1 if none]
jitter_ms     = mean(|delay[i] - delay[i-1]|) sorted by time  [-1 if <2]
duration_sec  = last_delivered - first_delivered; fallback to run elapsed
throughput_bps = delivered_bytes * 8 / duration_sec
generated_bps  = generated_bytes * 8 / duration_sec

score_pdr    = min(pdr / target_pdr, 1.0)
score_delay  = min(target_delay_ms / delay_mean_ms, 1.0)
score_jitter = min(target_jitter_ms / jitter_ms, 1.0)
qos_score    = (score_pdr * w_pdr + score_delay * w_delay + score_jitter * w_jitter)
               / (w_pdr + w_delay + w_jitter)
```

Score components are 0.0 when the denominator is ≤ 0 (no data or no-op target).

### 8.3 `fillDecisionNetworkContext()` (L1228-L1241)

Called from `deliveredCallback()` when processing a CHARGE_DECISION.
Looks up key `"1:CHARGE_DECISION"` in `buildQosStats()` and populates
`ChargeRecord.decision_ctrl_*` fields with the **cumulative** (not windowed)
QoS snapshot for the CHARGE_DECISION control-plane at the moment of this decision.

---

## 9. CHARGE_DECISION Payload Format

The UGV charger encodes its decision rationale in `TrafficMessage.payload` as
semicolon-delimited `key=value` pairs. `parseDecisionRationale()` (L1188-L1226)
extracts:

| Payload key | ChargeRecord field | Type | Semantics |
|------------|-------------------|------|-----------|
| `policy` | `decision_policy` | string | Scheduling algorithm name |
| `priority` | `decision_priority` | int | UAV's scheduling priority score at decision time |
| `rank_index` | `decision_rank_index` | int | 0-based position in sorted queue |
| `queue_size` | `decision_queue_size` | int | Queue length at decision time |
| `tte_sec` | `decision_tte_sec` | double | Time-to-empty (estimated flight time remaining, seconds) |
| `score` | `decision_score` | double | Composite priority score |
| `accepted` | (direct in deliveredCallback) | bool (0/1) | Set in payload; `accepted=0` = REJECTED |
| `reason` | (direct in deliveredCallback) | string | `reason=PREEMPTED` triggers preemption path |

Example payload: `policy=PRIORITY_CH_FIRST;priority=2;rank_index=0;queue_size=3;tte_sec=45.2;score=0.87`

---

## 10. Deduplication Guards Summary

| Guard | Type | Scope | Guards against |
|-------|------|-------|---------------|
| `already_logged_generated_` | unordered_set\<string\> | per run_instance | Double-logging same msg_id to packet_generated_events |
| `already_logged_delivered_` | unordered_set\<string\> | per run_instance | Double-logging same msg_id to packet_delivered_events |
| `already_logged_charge_decision_` | unordered_set\<string\> | per run_instance | Double-logging same ref_msg_id to charge_decision_events |
| `dropped_ids_` | unordered_set\<string\> | per run_instance | Double-counting drops; first DROP per ref_msg_id only |
| `ack_by_ref_` | unordered_map\<string,Time\> | per run_instance | Double-counting ACKs; first ACK per ref_msg_id only |
| `seen_failure_ids_` | unordered_set\<string\> | per run_instance | Processing same FAILURE_EVENT msg_id twice |
| `seen_control_msg_ids_` | unordered_set\<string\> | per run_instance | Double-counting in `control_type_counts_` |
| `exported_messages_` | unordered_set\<string\> | per run_instance | Re-exporting already-written MsgRecords |
| `exported_charge_requests_` | unordered_set\<string\> | per run_instance | Re-exporting already-written terminal ChargeRecords |
| `exported_recovery_events_` | unordered_set\<string\> | per run_instance | Re-exporting already-written RecoveryEvents |
| `exported_preemption_count_` | size_t cursor | per run_instance | Re-exporting already-written PreemptionEvents |
| `exported_death_event_count_` | size_t cursor | per run_instance | Re-exporting already-written DeathEvents |
| `recovery_events_` map (key = msg_id) | unordered_map | per run_instance | Tracking same RecoveryEvent twice in `trackRecoveryEvent` |
| `ChargeRecord.request_msg_id` empty check | L358 `is_new_request` | per msg_id | Firing logChargeRequestEvent more than once per request |
| `ChargeRecord.delivered` flag | L516 `if (rec.delivered) return;` | per msg_id | Processing same delivery twice |
| State machine gate (isTerminalOutcome) | L718, L470, L757 | per ChargeRecord | Overwriting terminal outcomes with new transitions |
| `ChargeRecord.charge_completed` | L737 | per ChargeRecord | Processing DOCK_END more than once per charging session |

**Append-only header guard** (`openAppend()` L841-L857):
```cpp
bool need_header = !std::filesystem::exists(path, ec) ||
                   std::filesystem::file_size(path, ec) == 0;
```
Header is written exactly once per file per OS-level file existence. Across
process restarts writing to the same `output_root_`, the header will NOT be
re-written because the file already exists with content.

---

## 11. Timer Schedule and Write Cadence

| Timer | Period | Created at | Writes to |
|-------|--------|------------|----------|
| `csv_timer_` | `csv_write_period_sec` (default 10.0 s) | L267 | `writeOutputs()` → all flush-driven files |
| `charge_timeout_timer_` | 1 s | L271 | `checkChargeTimeouts()` → may emit to charge_session_events.csv |
| `status_timeseries_timer_` | `status_sample_period_sec_` (1.0 s) | L275 | `status_timeseries.csv` |
| `weather_timeseries_timer_` | `status_sample_period_sec_` (1.0 s) | L279 | `weather_timeseries.csv` |
| `queue_timeseries_timer_` | `queue_stats_period_sec_` (1.0 s) | L283 | `charge_queue_timeseries.csv` |
| `network_timeseries_timer_` | `status_sample_period_sec_` (1.0 s) | L287 | `network_timeseries.csv` |
| `stats_timer_` | 0.5 s | L293 | `/network_monitor/stats` topic |

**Shutdown sequence:**
1. `rclcpp::on_shutdown` lambda (L297-L299) calls `writeOutputs(true)`.
2. `~NetworkMonitorNode()` destructor (L305-L307) also calls `writeOutputs(true)`.
Both calls are safe because all writers are idempotent (append-only + per-record export guards).

---

## 12. Parameter Reference

| Parameter name | C++ member | Default | Unit | Notes |
|---------------|------------|---------|------|-------|
| `run_id` | `run_id_` | `"run0"` | — | — |
| `output_dir` | `output_dir_` | `"log"` | — | Base dir; output_root = output_dir/run_id |
| `protocol_name` | `protocol_name_` | `"unknown"` | — | NEW: passed from launch |
| `replicate_id` | `replicate_id_` | `0` | — | NEW: passed from launch |
| `csv_write_period_sec` | (local at L206) | `10.0` | s | Formerly `flush_period_sec` (renamed in launch) |
| `decision_timeout_sec` | `decision_timeout_sec_` | `30.0` | s | TIMEOUT trigger in checkChargeTimeouts |
| `status_sample_period_sec` | `status_sample_period_sec_` | `1.0` | s | Status/weather/network timeseries period |
| `queue_stats_period_sec` | `queue_stats_period_sec_` | `1.0` | s | Queue timeseries period |
| `ugv_dock_capacity` | `ugv_dock_capacity_` | `1` | slots | Fallback if UGV snapshot not received |
| `network_stats_window_sec` | `rate_window_sec_` | `10.0` | s | Sliding window for network_timeseries |
| `qos_target_pdr` | `qos_target_pdr_` | `0.95` | — | Target PDR for QoS score |
| `qos_target_delay_ms` | `qos_target_delay_ms_` | `200.0` | ms | Target e2e delay |
| `qos_target_jitter_ms` | `qos_target_jitter_ms_` | `50.0` | ms | Target jitter |
| `qos_weight_pdr` | `qos_weight_pdr_` | `0.5` | — | QoS score weight |
| `qos_weight_delay` | `qos_weight_delay_` | `0.3` | — | QoS score weight |
| `qos_weight_jitter` | `qos_weight_jitter_` | `0.2` | — | QoS score weight |

---

## 13. Known Limitations and Coverage Gaps

| # | Description | Impact | Where |
|---|-------------|--------|-------|
| 1 | `routingTableCallback()` is an empty stub (L1162-L1164) | No `routing_table_timeseries.csv` produced | `/fanet/routing_table` topic ignored |
| 2 | `messages.csv` exported before `reconcileCausality()` if export fires at same flush as drop reconciliation | `dropped`/`drop_reason`/`ack_time_s` may be incorrect; use `packet_drop_events.csv` as authoritative drop source | `writeMessagesCsv()` / `reconcileCausality()` ordering |
| 3 | `alive_count` in `death_events.csv` uses vector index, not actual alive count at event time | Kaplan-Meier estimates will be off if deaths are exported in batch; sort by `time_s` and recompute externally | `writeDeathEventsCsv()` L2068 |
| 4 | `mean_wait_ch_ms` / `mean_wait_member_ms` in queue timeseries are cumulative (all sessions since run start), not instantaneous queue wait | Cannot distinguish current queue wait from historical average | `writeChargeQueueTimeseriesRow()` L1908-L1956 |
| 5 | `drop_reason` is free text from simulator/router; no normalization or enum | Analysis requires string matching; values may vary across simulator versions | `TrafficMessage.drop_reason` field |
| 6 | `battery_capacity_wh` will be 0.0 if `UavStatus.battery_capacity` is not set or arrives after first DOCK_END | `energy_charged_wh` = -1.0 in that case | `UavState.battery_capacity` init L148 |
| 7 | `telemetry_delivered_`, `telemetry_avg_delay_sec_`, `telemetry_dropped_` members (L2519-L2522) are tracked but never written to any output file | Telemetry-specific metrics silently discarded | `deliveredCallback()` L534-L537 |
| 8 | `delivered_by_flow_control_` (L2523) tracked but not written to any output | Fine-grained flow×control delivery counts not captured in CSV | `deliveredCallback()` L539 |
| 9 | `drop_reasons_` map (L2568) maintained separately from `drop_reason_counts_` without output | Redundant member | — |
| 10 | `PreemptionEvent.ugv_id` populated to `""` (queueEventCallback does not parse it) | Cannot join preemption events to UGV dock directly | `queueEventCallback()` L1266 |
| 11 | `qos_metrics.csv` writes **all known categories at every flush** (not incremental) | Growing analysis overhead as number of (flow_type, control_type) combinations grows | `writeQosMetricsCsv()` L1412 |

---

## 14. Precise Join Keys for All Analysis Plots

### 14.1 E2E Packet Delay

```python
import pandas as pd

gen = pd.read_csv("log/<run_id>/packet_generated_events.csv")
del_ = pd.read_csv("log/<run_id>/packet_delivered_events.csv")

# Exact join on (run_id, run_instance_id, msg_id)
merged = del_.merge(
    gen[["run_id","run_instance_id","msg_id","creation_time_s"]],
    on=["run_id","run_instance_id","msg_id"],
    how="inner")

merged["delay_ms"] = (merged["delivered_time_s"] - merged["creation_time_s"]) * 1000.0
# Filter out negative (clock skew artefacts)
merged = merged[merged["delay_ms"] >= 0]
```

**Join key:** `(run_id, run_instance_id, msg_id)` — exact string match.
**Clock note:** `creation_time_s` = producer ROS clock; `delivered_time_s` =
UAV or monitor ROS clock. Both should be the same domain if `/use_sim_time=true`.

### 14.2 PDR by Protocol and Replicate

**Option A — window-based (preferred):**
```python
net = pd.read_csv("log/<run_id>/network_timeseries.csv")
# Last row per (run_id, run_instance_id) for cumulative PDR:
#   use qos_metrics.csv instead
# For windowed PDR over time:
net_filtered = net[net["protocol_name"] == "PRIORITY_CH_FIRST"]
# pdr = window_delivered / window_generated (already pre-computed)
```

**Option B — event-based:**
```python
gen = pd.read_csv("packet_generated_events.csv")
del_ = pd.read_csv("packet_delivered_events.csv")
for (proto, rep), g in gen.groupby(["protocol_name","replicate_id"]):
    d_count = del_[(del_.protocol_name==proto) & (del_.replicate_id==rep)].msg_id.nunique()
    pdr = d_count / g.msg_id.nunique()
```

### 14.3 Charge Waiting Time by Role

```python
sess = pd.read_csv("log/<run_id>/charge_session_events.csv")
dock_start = sess[
    (sess.event_type == "DOCK_START") &
    (sess.waiting_time_ms >= 0)
]
# Role: 0=MEMBER, 1=CH, 2=BACKUP_CH, -1=unknown
avg_wait = dock_start.groupby(["protocol_name","replicate_id","role"]).waiting_time_ms.agg(
    ["mean","median","std","count"])
```

**Alternative via direct join:**
```python
req = pd.read_csv("charge_request_events.csv")
merged = dock_start.merge(
    req[["run_id","run_instance_id","request_msg_id","request_time_s"]],
    on=["run_id","run_instance_id","request_msg_id"],
    how="left")
merged["wait_recomputed_ms"] = (merged["event_time_s"] - merged["request_time_s"]) * 1000
```

### 14.4 Energy Charged Per Session (Wh)

```python
sess = pd.read_csv("charge_session_events.csv")
dock_end = sess[
    (sess.event_type == "DOCK_END") &
    (sess.energy_charged_wh >= 0)  # excludes -1 sentinel
]
energy_per_run = dock_end.groupby(["run_id","run_instance_id","protocol_name","replicate_id"])\
    .energy_charged_wh.sum()
energy_per_uav = dock_end.groupby(["run_id","run_instance_id","uav_id"])\
    .energy_charged_wh.sum()
# Jain fairness index
def jain(x): return (x.sum()**2) / (len(x) * (x**2).sum())
fairness = dock_end.groupby(["protocol_name","replicate_id"])["energy_charged_wh"]\
    .apply(lambda g: jain(g.groupby("uav_id").sum()))
```

### 14.5 Decision Latency Distribution

```python
dec = pd.read_csv("charge_decision_events.csv")
valid = dec[dec.decision_latency_ms >= 0]
# Box plot per protocol
valid.boxplot(column="decision_latency_ms", by=["protocol_name","replicate_id"])

# Or via join for full context:
req = pd.read_csv("charge_request_events.csv")
merged = dec.merge(
    req[["run_id","run_instance_id","request_msg_id","role","battery_at_request"]],
    on=["run_id","run_instance_id","request_msg_id"],
    how="left")
```

### 14.6 UAV Battery Over Time

```python
status = pd.read_csv("status_timeseries.csv")
# Align replicates on t_rel_s
import matplotlib.pyplot as plt
for (proto, uav), g in status.groupby(["protocol_name","uav_id"]):
    # One line per (protocol, uav) with t_rel_s on x-axis
    plt.plot(g["t_rel_s"], g["battery_level"], label=f"{proto}/{uav}")
```

### 14.7 Queue Length Over Time with Invariant Check

```python
q = pd.read_csv("charge_queue_timeseries.csv")
# Verify invariant — should be 0 everywhere; alert on anomalies
anomalies = q[q.queue_length_total_check != 0]
if not anomalies.empty:
    print("WARNING: queue_length_total_check non-zero in", len(anomalies), "rows")

# Plot by protocol
q.groupby("protocol_name").apply(
    lambda df: df.set_index("t_rel_s")[
        ["queue_length_total","queue_length_ch","queue_length_member"]
    ].plot(title=df.name))
```

### 14.8 PDR vs Weather Regime

```python
weather = pd.read_csv("weather_timeseries.csv")
gen = pd.read_csv("packet_generated_events.csv")
del_ = pd.read_csv("packet_delivered_events.csv")

# Asof join: assign nearest-past weather regime to each generated packet
def join_weather(df, w):
    return pd.merge_asof(
        df.sort_values("t_rel_s"),
        w[["run_id","run_instance_id","t_rel_s","regime"]].sort_values("t_rel_s"),
        on="t_rel_s",
        by=["run_id","run_instance_id"],
        direction="backward",
        tolerance=2.0)   # 2 s max gap

gen_w = join_weather(gen, weather)
del_w = join_weather(del_, weather)
# PDR per regime
gen_cnt = gen_w.groupby(["protocol_name","regime"]).msg_id.nunique()
del_cnt = del_w.groupby(["protocol_name","regime"]).msg_id.nunique()
pdr_by_regime = (del_cnt / gen_cnt).fillna(0).rename("pdr")
```

### 14.9 Kaplan-Meier UAV Survival

```python
death = pd.read_csv("death_events.csv")
# Sort by event time for correct alive_count (recompute externally)
death_sorted = death.sort_values("time_s")
death_sorted["alive_count_corrected"] = (
    death_sorted.groupby(["run_id","run_instance_id"])
    .cumcount(ascending=False)
)
# KM curve:
from lifelines import KaplanMeierFitter
kmf = KaplanMeierFitter()
for (proto, rep), g in death_sorted.groupby(["protocol_name","replicate_id"]):
    # event_times = time_s; event_observed = True for all (all are deaths)
    kmf.fit(g.time_s, event_observed=[True]*len(g), label=f"{proto} rep{rep}")
    kmf.plot_survival_function()
```

### 14.10 Preemption Rate vs Protocol

```python
pre = pd.read_csv("preemption_events.csv")
# Count events per run and normalize by run duration
summary = pd.read_json("summary_snapshots.jsonl", lines=True)
# Use last snapshot per run_instance_id
final = summary.sort_values("t_rel_s").groupby("run_instance_id").last().reset_index()

pre_counts = pre.groupby(["run_id","run_instance_id","protocol_name"]).size().rename("preemption_count")
merged = final.merge(pre_counts, on=["run_id","run_instance_id","protocol_name"], how="left")
merged["preemption_rate_per_min"] = merged["preemption_count"].fillna(0) / (merged["t_rel_s"] / 60.0)
```

### 14.11 Radar Plot — Policy Comparison

Axis values (aggregate across replicates; median across replicates of per-replicate metric):

| Axis | Table | Columns | Computation |
|------|-------|---------|-------------|
| PDR | `qos_metrics.csv` (last row per run) | `pdr` where `control_type = ""` (DATA) | `median(pdr)` per protocol |
| Mean e2e delay | `packet_generated_events` + `packet_delivered_events` | `delivered_time_s − creation_time_s` | `median(mean_delay_ms)` per protocol |
| Charging efficiency | `charge_session_events` DOCK_END | `energy_charged_wh / charge_duration_s` | `median(mean_efficiency)` per protocol |
| Fairness (Jain) | `charge_session_events` DOCK_END | `energy_charged_wh` per UAV | `median(jain_index)` per protocol |
| Survival rate | `summary_snapshots.jsonl` | `fleet.survival_rate` | `median(survival_rate)` per protocol |

---

## 15. Smoke Test for Append-Only Correctness

```bash
# 1. Run experiment once
ros2 launch system_bringup experiment.launch.py \
    run_id:=smoke01 protocol_name:=FCFS replicate_id:=1 \
    output_dir:=/tmp/test_log

# 2. Record file sizes
ls -lh /tmp/test_log/smoke01/*.csv /tmp/test_log/smoke01/*.jsonl

# 3. Restart (same run_id, new run_instance_id)
ros2 launch system_bringup experiment.launch.py \
    run_id:=smoke01 protocol_name:=FCFS replicate_id:=1 \
    output_dir:=/tmp/test_log

# 4. Files must GROW, never shrink
ls -lh /tmp/test_log/smoke01/*.csv /tmp/test_log/smoke01/*.jsonl

# 5. Header appears exactly once per file
grep -c "^run_id" /tmp/test_log/smoke01/packet_generated_events.csv
# Expected: 1

# 6. Two distinct run_instance_ids
python3 -c "
import pandas as pd
df = pd.read_csv('/tmp/test_log/smoke01/packet_generated_events.csv')
print(f'run_instance_ids: {df.run_instance_id.nunique()} (expect 2)')
print(df.groupby('run_instance_id').size())
"

# 7. JSONL summary has one object per writeOutputs call
wc -l /tmp/test_log/smoke01/summary_snapshots.jsonl
python3 -c "
import json
with open('/tmp/test_log/smoke01/summary_snapshots.jsonl') as f:
    for i, line in enumerate(f):
        if line.strip():
            obj = json.loads(line)
            print(i, obj['run_instance_id'][:20], 't_rel_s=', obj['t_rel_s'])
"
```

