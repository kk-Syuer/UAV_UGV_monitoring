# Recovery / CH Failure / Network Rebuild Audit

## 1) System Map (topics, messages, timers, parameters)

### 1.1 Recovery manager

| Node | Publishes | Subscribes | Timers | Key parameters |
|---|---|---|---|---|
| `recovery_manager_node` | `/fanet/network_bus_raw` (`uav_msgs/msg/TrafficMessage`) | `/fanet/status` (`uav_msgs/msg/UavStatus`), `/fanet/network_bus` (`uav_msgs/msg/TrafficMessage`), `/routing_manager/alerts` (`std_msgs/msg/String`), `/uav_fleet/failure_events` (`uav_msgs/msg/FailureEvent`), `/coverage_planner/task_points` (`uav_msgs/msg/TaskPointArray`, transient local), `/ch_manager/cluster_info` (`uav_msgs/msg/ClusterInfo`) | `watchdog_timer_` (500 ms), `ack_retry_timer_` (`ack_retry_period_sec`) | `comm_range_m`, `status_timeout_sec`, `heartbeat_timeout_sec`, `recovery_cooldown_sec`, `movement_delta_m`, `control_ttl`, `ack_retry_period_sec`, `max_ack_retries` |

Evidence: setup and wiring in constructor and callback registration. (`src/recovery_manager/src/recovery_manager_node.cpp`, lines 64-110)

### 1.2 CH manager

| Node | Publishes | Subscribes | Timers | Key parameters |
|---|---|---|---|---|
| `ch_manager_node` | `/ch_manager/cluster_info` (`uav_msgs/msg/ClusterInfo`) | `/uav_fleet/failure_events` (`uav_msgs/msg/FailureEvent`) | 1 s periodic cluster info publish | `cluster_id`, `ch_id`, `member_ids` |

Evidence: cluster membership publication and failure log-only callback. (`src/ch_manager/src/ch_manager_node.cpp`, lines 20-37, 45-71)

### 1.3 UAV node

| Node | Publishes | Subscribes | Timers | Key parameters |
|---|---|---|---|---|
| `uav_node` | `/fanet/status` (`UavStatus`), `/fanet/network_bus_raw` (`TrafficMessage`), `/fanet/delivered` (`TrafficMessage`), `/fanet/routing_event` (`String`), `/uav_fleet/charge_requests` (`ChargeRequest`), `/uav_fleet/failure_events` (`FailureEvent`) | `/fanet/network_bus` (`TrafficMessage`), `/fanet/status`, `/ch_manager/cluster_info`, `/environment/weather`, `/coverage_planner/deployment`, `/coverage_planner/task_points` (transient local), `/fanet/routing_table` | status(1 s), heartbeat(1 s), traffic(2 s), plus neighbor timeout, buffer retry, CH status, charge retry, ACK retry, mobility | `battery_threshold`, `charge_decision_timeout_sec`, `hello_timeout_sec`, buffering + telemetry parameters, role + CH identity parameters |

Evidence: pubs/subs/timers/params in constructor. (`src/uav_fleet/src/uav_node.cpp`, lines 149-405)

### 1.4 Routing manager

| Node | Publishes | Subscribes | Timers | Key parameters |
|---|---|---|---|---|
| `routing_manager_node` | `/fanet/routing_table` (`uav_msgs/msg/RoutingTable`), `/routing_manager/alerts` (`std_msgs/msg/String`) | `/fanet/status` (`UavStatus`), `/fanet/routing_event` (`String`) | recompute timer (`recompute_period_sec`) | `comm_range_m`, `hysteresis_margin_m`, `recompute_period_sec`, `status_timeout_sec`, `ch_move_threshold_m`, `sink_id`, `ugv_id` |

Evidence: routing manager constructor and callbacks. (`src/routing_manager/src/routing_manager_node.cpp`, lines 76-101, 110-158)

### 1.5 Network monitor (supporting observability)

| Node | Publishes | Subscribes | Timers | Key parameters |
|---|---|---|---|---|
| `network_monitor_node` | `/network_monitor/stats` (`String`) + CSV/JSON files | `/fanet/network_bus`, `/fanet/network_bus_raw`, `/fanet/delivered`, `/uav_fleet/charge_requests`, `/ugv/charge_decisions`, `/fanet/status`, `/fanet/routing_table` | CSV write, charge timeout, status/queue timeseries, stats, shutdown check | `routing_table_empty_shutdown_sec`, `stop_on_backbone_loss`, `decision_timeout_sec`, QoS targets |

Evidence: constructor topic wiring and timers. (`src/network_monitor/src/network_monitor_node.cpp`, lines 145-223)

### 1.6 Topics used for requested recovery concerns

- CH heartbeat/status: `/fanet/network_bus` HEARTBEAT control messages + `/fanet/status` role=1 status updates. (`src/recovery_manager/src/recovery_manager_node.cpp`, lines 154-158, 124-145)
- Failure events: `/uav_fleet/failure_events` (`FailureEvent`) from UAV battery death. (`src/uav_fleet/src/uav_node.cpp`, lines 663-674)
- Cluster membership updates: `/ch_manager/cluster_info`; plus explicit `CLUSTER_REASSIGN` control message. (`src/ch_manager/src/ch_manager_node.cpp`, lines 45-52; `src/recovery_manager/src/recovery_manager_node.cpp`, lines 701-721)
- Routing updates: `/fanet/routing_table` from routing manager and `/routing_manager/alerts`. (`src/routing_manager/src/routing_manager_node.cpp`, lines 93-97, 439-459)
- Charge requests/decisions that influence recovery: `CHARGE_REQUEST`/`CHARGE_DECISION` control flow and `/uav_fleet/charge_requests`. (`src/uav_fleet/src/uav_node.cpp`, lines 775-811, 895-963, 3740-3749)

---

## 2) Recovery State Machine (explicit extraction)

### 2.1 Effective recovery states in code
`recovery_manager_node.cpp` has no explicit enum state; behavior is controlled by booleans/timestamps:
- `recovery_requested_`
- `sink_unreachable_`
- `ugv_unreachable_`
- per-CH `alive`
- cooldown via `last_recovery_time_`
- `epoch_`

Evidence: member fields and watchdog gate. (`src/recovery_manager/src/recovery_manager_node.cpp`, lines 227-267, 892-899)

### 2.2 State specification (derived)
- **NORMAL**
  - Guard: no CH timeout, sink reachable, UGV reachable, no explicit recovery request.
  - Trigger out: CH timeout, routing alert unreachable, or failure event.
- **RECOVERY_PENDING_COOLDOWN**
  - Guard: trigger exists but `(now-last_recovery_time_) < recovery_cooldown_sec_`.
  - Action: wait.
- **RECOVERY_RUNNING(epoch++)**
  - Actions:
    1. `RECOVERY_START` broadcast.
    2. Collect alive CH set.
    3. If none: `MEMBER_FALLBACK` messages and `RECOVERY_DONE`.
    4. Else: leader election, membership reassignment, task reassignment, coverage redeployment, optional sink/UGV bridge redeployment, `RECOVERY_DONE`.

Evidence: `watchdog()` and `runRecovery()`. (`src/recovery_manager/src/recovery_manager_node.cpp`, lines 227-308)

### 2.3 Transition diagram (text)
- NORMAL -> RECOVERY_PENDING_COOLDOWN on {status timeout OR heartbeat timeout OR failure event OR SINK/UGV unreachable alert}
- RECOVERY_PENDING_COOLDOWN -> RECOVERY_RUNNING when cooldown elapsed
- RECOVERY_RUNNING -> NORMAL immediately after `publishRecoveryDone(epoch)`

### 2.4 Reset/idempotency check
- **Reset present:** `recovery_requested_` cleared before run; `last_recovery_time_` updated each run. (`src/recovery_manager/src/recovery_manager_node.cpp`, lines 264-266)
- **Idempotency partial:** duplicate triggers within cooldown are suppressed; duplicate control commands are possible across epochs; ACK retry prevents one-shot loss but no semantic dedup by `(member,epoch,command_type)`. (`src/recovery_manager/src/recovery_manager_node.cpp`, lines 260-267, 811-858)

---

## 3) CH failure / low-battery trigger paths

### 3.1 Explicit `FailureEvent`
- UAV publishes `failure_type=1` only at battery depletion (`battery_energy_<=0`). (`src/uav_fleet/src/uav_node.cpp`, lines 659-675)
- Recovery manager reacts only to `failure_type==1`, marks CH dead if known, and requests recovery. (`src/recovery_manager/src/recovery_manager_node.cpp`, lines 189-201)

### 3.2 Missing CH status heartbeat in recovery manager
- CH considered down if `/fanet/status` stale > `status_timeout_sec`. (`src/recovery_manager/src/recovery_manager_node.cpp`, lines 236-239)
- CH considered down if HEARTBEAT stale > `heartbeat_timeout_sec`. (`src/recovery_manager/src/recovery_manager_node.cpp`, lines 247-252)

### 3.3 Routing-derived endpoint degradation
- `SINK_UNREACHABLE` / `UGV_UNREACHABLE` from routing manager triggers recovery even with alive CHs. (`src/recovery_manager/src/recovery_manager_node.cpp`, lines 176-187, 301-305)

### 3.4 Low battery / charging treated as availability
- CH intent-to-leave is included in status (`intent_to_leave` true when charging flow active), but recovery manager ignores `intent_to_leave` and `charging_state`; it only checks battery>0 and freshness. This conflates "alive" with "available for CH duties".
  - Status content: includes `intent_to_leave`, `charging_state`. (`src/uav_fleet/src/uav_node.cpp`, lines 727-755)
  - Recovery state update ignores these fields. (`src/recovery_manager/src/recovery_manager_node.cpp`, lines 137-145)

**Conclusion:** system cannot distinguish "temporarily unavailable (charging/to-UGV)" from "failed" in recovery logic.

### 3.5 Configurability and time base
- Thresholds configured from launch config (`experiment.launch.py`) and defaults YAML; runtime dynamic updates are not handled in code.
  - recovery params pass-through from YAML. (`src/system_bringup/launch/experiment.launch.py`, lines 317-332)
  - defaults values. (`src/system_bringup/config/experiment_defaults.yaml`, lines 78-83)
- Code uses `this->now()` and message timestamps; whether sim time is used depends on global ROS parameter setup (not explicit in inspected nodes).

---

## 4) Member UAV behavior when CH is gone

### 4.1 Detection path in member
- Member checks CH reachability via neighbor table in `publishStatus`; if CH not reachable and energy threshold crossed, may emergency return to UGV.
  (`src/uav_fleet/src/uav_node.cpp`, lines 691-702)

### 4.2 Discovery and rejoin
- Primary CH identity comes from:
  1. `CLUSTER_REASSIGN` control command (`my_ch_id_ = payload`), or
  2. periodic `ClusterInfo` containing this UAV in `member_ids`.
  (`src/uav_fleet/src/uav_node.cpp`, lines 1687-1704, 1895-1902)
- There is **no autonomous CH scanning/selection algorithm** in member logic beyond passively consuming commands/info.

### 4.3 All-CH-unavailable behavior
- Recovery manager sends `MEMBER_FALLBACK` with target UGV or sink pose when alive CH set is empty. (`src/recovery_manager/src/recovery_manager_node.cpp`, lines 283-287, 571-620)
- Member receiving fallback enters fallback mode, clears tasks, and moves toward fallback target. (`src/uav_fleet/src/uav_node.cpp`, lines 1735-1761)

### 4.4 Buffering behavior
- Generic message buffer with TTL + retry is present (`buffer_manager_`), and telemetry has dedicated buffering policy when out of CH range. (`src/uav_fleet/src/uav_node.cpp`, lines 74-136, 1136-1218, 1298-1312)

### 4.5 Hysteresis / anti-flap
- No explicit CH-switch hysteresis for member reassignment (`CLUSTER_REASSIGN` is immediate apply). (`src/uav_fleet/src/uav_node.cpp`, lines 1687-1704)

### 4.6 stuck flags checks
- `waiting_for_charge_response_` has timeout/retry clear path (`chargeRequestRetryTick`) => not infinite by design. (`src/uav_fleet/src/uav_node.cpp`, lines 3684-3714)
- `fallback_mode_active_` is cleared on `CLUSTER_REASSIGN` and `TASK_ASSIGN`. (`src/uav_fleet/src/uav_node.cpp`, lines 1697-1699, 1720-1722)

---

## 5) CH re-election / reassignment policy

### 5.1 Election type
- **Centralized** in recovery manager, not distributed CH node.
- Leader score = graph degree within `comm_range_m` * normalized battery percent.
  (`src/recovery_manager/src/recovery_manager_node.cpp`, lines 310-338)

### 5.2 Member reassignment policy
- For each alive member, assign nearest alive CH (tie-break higher CH battery). (`src/recovery_manager/src/recovery_manager_node.cpp`, lines 341-368)
- No explicit capacity constraint, min cluster size, or load cap.

### 5.3 Constraints present/missing
- Geographic/range constraints are partially used for bridge/backbone checks, but member->CH assignment uses nearest CH regardless of comm range. (`src/recovery_manager/src/recovery_manager_node.cpp`, lines 352-366, 648-667)

### 5.4 Edge-case audit
- **Split-brain (two CH elected simultaneously):** no distributed election, so split-brain inside this module is unlikely; however CH manager independently publishes static membership and can conflict semantically with recovery control commands. (`src/ch_manager/src/ch_manager_node.cpp`, lines 45-52)
- **No eligible CH:** handled by MEMBER_FALLBACK broadcast path. (`src/recovery_manager/src/recovery_manager_node.cpp`, lines 283-287)
- **Reassignment during charging decisions:** no coordination lock between recovery reassignment and charge state machine.
- **Outlier member outside CH range:** still assigned nearest CH; may be unreachable until mobility changes.

---

## 6) Routing invalidation and rebuild

### 6.1 How routes are built
- Routing manager recomputes from active nodes (`status_timeout_sec` freshness), classifies CH vs endpoint, builds CH graph with hysteresis, computes shortest paths, publishes per-node routing table. (`src/routing_manager/src/routing_manager_node.cpp`, lines 160-319, 336-437)

### 6.2 Invalidation trigger
- Implicit invalidation by stale status filtering and periodic/event-driven recompute.
- Event-driven trigger comes from `/fanet/routing_event` (e.g. NO_ROUTE reports from UAV nodes). (`src/routing_manager/src/routing_manager_node.cpp`, lines 143-148)

### 6.3 Recovery-to-routing coordination
- No explicit "clear routing" or "rebuild now" control type from recovery manager.
- Recovery indirectly influences routing by moving CHs and changing member CH IDs; routing then converges on next recompute period.

### 6.4 Risks
- Temporal race: `CLUSTER_REASSIGN` can be sent before routing table converges to routes via new CH.
- Stale next-hop risk mitigated by periodic refresh, but short windows remain.
- Loops: runtime loop detection using `recent_hops` exists in UAV forwarding path. (`src/uav_fleet/src/uav_node.cpp`, lines 1569-1572)

---

## 7) Timers/concurrency/race table

Single-threaded executor is likely by default (`rclcpp::spin(node)`), but asynchronous timers+callbacks still produce ordering races.

| Variable/State | Writers | Readers | Risk | Fix |
|---|---|---|---|---|
| `ch_states_[].alive` | recovery `statusCallback`, `failureCallback`, `watchdog` | `runRecovery` | False positive down due stale `stamp` source or delayed status; rapid flip in/out across cooldown windows | Treat charging/unavailable separately; incorporate multiple consecutive missed samples before dead mark |
| `member_states_[id].ch_id` | recovery `clusterInfoCallback`, `recomputeMembership` | `recomputeTasks`, `publishTaskAssign` next hop choice | Order-dependent task assignment via stale CH ID | Tag assignment epoch and apply only if newer |
| UAV `my_ch_id_` | `clusterInfoCallback`, `CLUSTER_REASSIGN`, deployment handler | forwarding, heartbeat, requestCharge | CH oscillation/flap due competing sources | Add reassignment hysteresis + source priority (recovery command > periodic cluster info for N sec) |
| `routing_table_` | UAV `routingTableCallback` (full clear+replace) | forwarding/resolveNextHop | transient empty table between updates can drop control packets | Use double-buffer swap and preserve old table until new non-empty table arrives |
| recovery `pending_acks_` | sendWithAck, heartbeat ACK callback, retry timer | retry timer | ack storms/retry overlap manageable, but no epoch binding | include epoch in payload and ignore stale ACK/domain mismatch |

Evidence: callbacks/timers touching these structures. (`src/recovery_manager/src/recovery_manager_node.cpp`, lines 124-174, 215-267, 811-858; `src/uav_fleet/src/uav_node.cpp`, lines 1687-1704, 1867-1925)

---

## 8) Deadlock/livelock checks

### 8.1 Hard/soft waits audited
- `waiting_for_charge_response_` has bounded fallback (ACK timeout and decision timeout resend). (`src/uav_fleet/src/uav_node.cpp`, lines 3684-3714)
- Recovery ACK waits are bounded by `max_ack_retries_` then drop. (`src/recovery_manager/src/recovery_manager_node.cpp`, lines 841-848)

### 8.2 Potential livelocks
1. **Repeated recovery thrash** if routing alert flaps + cooldown too short; no hysteresis on unreachable alerts.
2. **Member CH flapping** between periodic cluster_info and recovery reassignment (no hysteresis).
3. **Fallback invisibility in observability**: monitor tracks recovery controls but omits `MEMBER_FALLBACK`, making diagnosis incomplete.
   (`src/network_monitor/src/network_monitor_node.cpp`, lines 1636-1640)

### 8.3 Required fixes
- add event debouncing and min-hold timers;
- add assignment epoch checks;
- log and monitor fallback events;
- add timeout fallback for members waiting on CH release/task after reassignment.

---

## 9) Observability requirements

### 9.1 Existing strengths
- Recovery manager logs sent commands, ack receipt, retries. (`src/recovery_manager/src/recovery_manager_node.cpp`, lines 824-846)
- Network monitor aggregates recovery command counts for key control types. (`src/network_monitor/src/network_monitor_node.cpp`, lines 1483-1487, 1628-1670)

### 9.2 Gaps and minimal required log set
Required additional fields per recovery event:
1. trigger reason (`status_timeout`, `heartbeat_timeout`, `failure_event`, `routing_alert`)
2. old CH -> new CH per member
3. reassigned member count
4. routing epoch/version observed at reassignment apply
5. recovery latency (`RECOVERY_DONE` - `RECOVERY_START` per epoch)

### 9.3 Concrete logging additions
- `runRecovery()` start log should print trigger bitmap and alive/dead CH lists.
- `publishClusterReassign` should include previous CH id and epoch.
- `uav_node` on receiving reassignment should log old/new CH and local routing table version.
- `network_monitor` should parse and count `MEMBER_FALLBACK` as recovery event type.

---

## 10) Patch plan (concrete, no new message types)

1. **Introduce explicit CH availability classes in recovery manager**
   - File: `recovery_manager_node.cpp`
   - Add fields in `ChState`: `charging_state`, `intent_to_leave`, `availability` enum.
   - In `statusCallback`, copy `msg->charging_state` and `msg->intent_to_leave`.
   - In `runRecovery`, build:
     - `alive_chs` (not dead)
     - `eligible_chs` (alive && available_for_ch_role)
   - Reassignment should use `eligible_chs`; fallback only if `eligible_chs` empty.

2. **Add recovery trigger reason/epoch propagation**
   - Reuse existing `TrafficMessage.payload` fields (comma-separated `epoch,reason,...`).
   - Update `publishRecoveryStart/Done`, `publishClusterReassign`, `publishTaskAssign` payload/metadata.

3. **Member-side hysteresis for CH switching**
   - File: `uav_node.cpp`
   - Add params: `ch_switch_min_hold_sec`, `ch_switch_min_improvement_m`.
   - On `CLUSTER_REASSIGN`, accept only if hold expired or old CH unreachable.

4. **Order-safe reassignment vs routing updates**
   - File: `uav_node.cpp`
   - Add `routing_table_version_` local counter incremented in `routingTableCallback`.
   - On reassignment, defer traffic generation for short grace window until at least one routing table refresh.

5. **Recovery monitor completeness**
   - File: `network_monitor_node.cpp`
   - Include `MEMBER_FALLBACK` in `trackRecoveryEvent` filter and summary counters.

6. **CH manager role cleanup (optional)**
   - File: `ch_manager_node.cpp`
   - Clarify static nature in docs/logs, or stop publishing authoritative membership if recovery manager is active to prevent conflicting control planes.

---

## 11) Reproduction and validation

## Preconditions
```bash
cd /workspace/UAV_UGV_monitoring
colcon build --symlink-install
source install/setup.bash
```

Start baseline scenario:
```bash
ros2 launch system_bringup experiment.launch.py \
  config:=system_bringup/config/runs/example_run.yaml \
  run_id:=recovery_audit \
  output_dir:=/tmp/fanet_logs
```
(Launch pattern documented in README.) (`README.md`, lines 395-399)

### Scenario A: CH battery low and goes to charge
1. Run with aggressive battery threshold in run config (set CH `battery_threshold` high enough to trigger early request).
2. Observe charge flow:
```bash
ros2 topic echo /uav_fleet/charge_requests
ros2 topic echo /fanet/network_bus --qos-reliability reliable | rg "CHARGE_DECISION|CLUSTER_REASSIGN|MEMBER_FALLBACK|RECOVERY_"
ros2 topic echo /fanet/status | rg "uav_1|charging_state|intent_to_leave"
```
3. Expected current behavior:
   - CH does **not** immediately count as failed unless status/heartbeat timeout or battery dead.
   - If CH leaves and members lose CH reachability + low energy, members may emergency return to UGV.

### Scenario B: CH hard failure (kill node)
1. Identify CH node:
```bash
ros2 node list
```
2. Kill CH process (example):
```bash
pkill -f "uav_node.*uav_1"
```
3. Observe triggers and recovery:
```bash
ros2 topic echo /uav_fleet/failure_events
ros2 topic echo /routing_manager/alerts
ros2 topic echo /fanet/network_bus | rg "RECOVERY_START|RECOVERY_DONE|CLUSTER_REASSIGN|TASK_ASSIGN|NEW_DEPLOYMENT|MEMBER_FALLBACK"
```
4. Expected transitions:
   - After `status_timeout_sec` / `heartbeat_timeout_sec`, recovery watchdog runs.
   - `RECOVERY_START` emitted, reassignment/deployment commands sent, then `RECOVERY_DONE`.

### Scenario C: multi-member CH loss storm
1. Force cluster connectivity loss by reducing communication radius in run config (`network.comm_radius_m`) then relaunch.
2. Optionally kill current CH as in Scenario B.
3. Observe convergence and loss:
```bash
ros2 topic echo /fanet/routing_table
ros2 topic echo /network_monitor/stats
ros2 topic echo /routing_manager/alerts
```
4. Metrics to track:
   - recovery command counts (`RECOVERY_START/DONE`, reassignment count),
   - routing table non-empty continuity,
   - packet drop reasons and delay from network monitor outputs in `/tmp/fanet_logs/<run_id>/`.

---

## High-risk findings (ranked)

1. **No explicit distinction between CH failed vs CH temporarily unavailable (charging/intent_to_leave)** -> can produce wrong reassign timing or no proactive reassignment.
2. **No explicit routing epoch coupling with cluster reassignment** -> transient blackholes during convergence windows.
3. **No CH-switch hysteresis in member** -> potential flapping from mixed control sources.
4. **Recovery observability gap (`MEMBER_FALLBACK` not tracked in monitor)** -> hidden failure mode in postmortems.
5. **CH manager static membership publisher may conflict with recovery control-plane intent** in dynamic failure scenarios.

