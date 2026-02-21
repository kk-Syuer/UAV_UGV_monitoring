# UAV–UGV Cooperative FANET Simulator (ROS 2)

**Executive Summary (read first)**  
This repository is a ROS 2 simulation workspace for **UAV–UGV cooperative disaster monitoring** where UAVs (cluster-head + members) must exchange **telemetry, coordination, recovery commands, and charging decisions through an explicit FANET overlay** implemented at the application layer. The FANET is modeled via a **two-stage bus** (`/fanet/network_bus_raw` → fault injection → `/fanet/network_bus`) and an explicit **delivery tap** (`/fanet/delivered`) used by the network monitor to compute end-to-end metrics. Routing is **centralized**: a `routing_manager_node` builds per-node next-hop tables from status beacons and publishes them as `/fanet/routing_table`. A `coverage_planner_node` computes initial deployment + task point assignment and injects deployment/start-mobility control into the FANET. A `ugv_charger_node` implements a multi-policy charging scheduler (including **preemptive variants**) and communicates decisions via FANET control messages, while also publishing a small set of **out-of-band mirror topics** for observability. A `recovery_manager_node` watches for backbone failures/unreachability and injects recovery control messages for reassignment and redeployment. A `network_monitor_node` logs concrete CSV/JSON artifacts per run for protocol comparison.

## Executive Summary and Glossary

### Glossary

- **FANET**: Flying Ad-hoc Network. In this repo: an application-layer routing + forwarding model built on top of ROS topics.
- **Bus (raw vs processed)**:  
  - **Raw bus**: `/fanet/network_bus_raw` — where nodes publish newly generated and forwarded packets.  
  - **Processed bus**: `/fanet/network_bus` — the same packets after impairment (fault injection).
- **Delivered channel**: `/fanet/delivered` — published by the final destination node when a packet reaches it; used for latency and delivery metrics.
- **CH (Cluster Head)**: UAV role that participates in the **backbone** and forwards packets (multi-hop routing).
- **Member**: UAV role that performs area tasks and sends telemetry; members **do not forward** packets.
- **Sink / Gateway**: The “outside world” endpoint (`sink_gateway_node`). Control/telemetry ultimately targets `sink_gateway` as a destination ID.
- **Next hop**: The immediate receiver for this hop (`TrafficMessage.next_hop_id`).
- **TTL**: Hop limit (`TrafficMessage.ttl`). `0` indicates “unlimited” by message definition.
- **ACK**: Application-layer acknowledgements for control packets (`TrafficMessage.control_type == "ACK"`).
- **PDR**: Packet Delivery Ratio, computed by the network monitor per `{flow_type, control_type}` category.
- **FCFS**: First-Come First-Served charging queue selection policy.
- **EDF**: Earliest Deadline First charging queue selection policy (based on time-to-empty estimate).
- **Role Priority**: Charging policy that prioritizes CHs over members.
- **Dynamic Score**: Charging policy that computes a composite score from role + battery + wait time.
- **Preemption**: A policy mode where the UGV can interrupt an ongoing charging session to serve a higher-priority UAV.

## Project Overview

### What this simulator is for

This project models a UAV–UGV disaster monitoring scenario where UAVs form clusters (CH + members) to monitor an area and deliver telemetry to a sink gateway while coping with **battery limitations**, **charging constraints**, **weather-driven impairments**, and **network partitions**.

The codebase is explicitly designed so that **inter-agent packets** (coordination control, telemetry, charging requests/decisions, recovery control) are processed through the FANET overlay using `uav_msgs/msg/TrafficMessage`.  
Important exception to keep in mind: some observability/state topics are intentionally **out-of-band** (direct ROS topics) such as status beacons and weather broadcasts.

### What questions it helps answer

- How do different charging scheduling policies (FCFS / EDF / role-priority / dynamic-score, plus preemptive variants) affect:
  - charging success rate,
  - queue waiting time,
  - preemption frequency,
  - mission survival (battery-dead rates)?
- How do routing and FANET conditions affect:
  - end-to-end latency,
  - delivery ratio,
  - forwarding overhead / hop counts,
  - control-plane reliability (ACK/DROP behavior)?
- How do environmental regimes (Markov weather) alter:
  - packet drops (fault injector),
  - UAV battery drain (weather-sensitive consumption).

### Relevant files
- `src/system_bringup/launch/experiment.launch.py`
- `src/uav_fleet/src/uav_node.cpp`
- `src/routing_manager/src/routing_manager_node.cpp`
- `src/ugv_charger/src/ugv_charger_node.cpp`
- `src/network_monitor/src/network_monitor_node.cpp`
- `src/weather_server/src/weather_node.cpp`

### Key parameters (YAML/ROS params)
- `global.termination_mode`, `global.experiment_timeout_s`
- `global.rng_seed` (fan-out seeding is implemented in the launch file; see it for exact mapping)
- `network.comm_range_m` (routing + physical reachability reference)

### Key topics
- `/fanet/network_bus_raw`, `/fanet/network_bus`, `/fanet/delivered`
- `/fanet/status`, `/fanet/routing_table`
- `/environment/weather`

## Repository Layout

This repo is a ROS 2 workspace with source packages under `src/`. It also contains `build/` and `install/` directories checked in, which are build artifacts; the authoritative implementation is under `src/`.

### Workspace tree (packages)

```
.
├── src/
│   ├── uav_msgs/            # custom messages/services/actions
│   ├── uav_fleet/           # UAV behavior node (CH + member)
│   ├── ugv_charger/         # UGV charging scheduler + dock model
│   ├── routing_manager/     # centralized routing table computation
│   ├── fault_injector/      # weather-driven packet dropper (raw -> processed bus)
│   ├── weather_server/      # Markov (or fixed) weather generator
│   ├── coverage_planner/    # deployment + task point generation + control injection
│   ├── sink_gateway/        # sink endpoint node (delivery + CH status intake)
│   ├── ch_manager/          # CH membership publisher + reacts to reassign control
│   ├── recovery_manager/    # recovery watchdog + reassignment/redeployment control injection
│   ├── user_devices_sim/    # synthetic user devices injecting traffic
│   ├── network_monitor/     # metrics + CSV/JSON logging per run
│   ├── planner_viz/         # optional matplotlib visualizer (Python)
│   └── system_bringup/      # launch + run YAML configs + taskpoint files
├── tools/
│   └── charging_protocol_compare.py  # postprocessing charts from monitor logs
└── docs/
    ├── recovery_cluster_rebuild_audit.md
    └── uav_market_charging_report.md
```

### Package-by-package details

#### `uav_msgs`
Purpose: shared message/service/action interfaces.  
Main artifacts:  
- Messages under `src/uav_msgs/msg/` (e.g., `TrafficMessage`, `UavStatus`, `RoutingTable`, `TaskPointArray`, etc.)
- Services under `src/uav_msgs/srv/`
- Action under `src/uav_msgs/action/`

Key files:
- `src/uav_msgs/CMakeLists.txt`
- `src/uav_msgs/msg/TrafficMessage.msg`
- `src/uav_msgs/msg/UavStatus.msg`

#### `uav_fleet`
Purpose: the UAV behavioral model (both CH and member roles), including movement, telemetry generation, forwarding (CH only), charging FSM, buffering, and ACK logic.  
Main executable:
- `uav_node` (`src/uav_fleet/src/uav_node.cpp`)

Key files:
- `src/uav_fleet/src/uav_node.cpp`
- `src/uav_fleet/CMakeLists.txt`

#### `ugv_charger`
Purpose: UGV charger + queue/scheduler model; sends `CHARGE_DECISION` control messages via FANET, publishes queue snapshots for viz, and mirrors a subset of charge info on direct ROS topics.  
Main executable:
- `ugv_charger_node` (`src/ugv_charger/src/ugv_charger_node.cpp`)

Key files:
- `src/ugv_charger/src/ugv_charger_node.cpp`

#### `routing_manager`
Purpose: centralized routing: build CH backbone graph, assign endpoints to nearest CH gateway, compute per-node next-hop tables, publish `/fanet/routing_table`, emit alerts on sink/UGV reachability.  
Main executable:
- `routing_manager_node` (`src/routing_manager/src/routing_manager_node.cpp`)

Key files:
- `src/routing_manager/src/routing_manager_node.cpp`

#### `fault_injector`
Purpose: impair packets (drop probability as a function of weather), transforming `/fanet/network_bus_raw` into `/fanet/network_bus`.  
Main executable:
- `fault_injector_node` (`src/fault_injector/src/fault_injector_node.cpp`)

Key files:
- `src/fault_injector/src/fault_injector_node.cpp`

#### `weather_server`
Purpose: publish weather state, either fixed-regime or Markov-regime transitions.  
Main executable:
- `weather_node` (`src/weather_server/src/weather_node.cpp`)

Key files:
- `src/weather_server/src/weather_node.cpp`

#### `coverage_planner`
Purpose: compute initial deployment (CH/member positions, sink/UGV placement) and task points; inject deployment and motion start control into FANET; publish task points as transient-local.  
Main executable:
- `coverage_planner_node` (`src/coverage_planner/src/coverage_planner_node.cpp`)
Library:
- `coverage_planner_lib` (`src/coverage_planner/src/coverage_planner.cpp`, `.hpp`)

Key files:
- `src/coverage_planner/src/coverage_planner_node.cpp`
- `src/coverage_planner/src/coverage_planner.cpp`
- `src/coverage_planner/src/coverage_planner.hpp`

#### `sink_gateway`
Purpose: sink endpoint; receives delivered traffic, sends ACKs when required, tracks CH status updates, publishes its own status beacon.  
Main executable:
- `sink_gateway_node` (`src/sink_gateway/src/sink_gateway_node.cpp`)

Key files:
- `src/sink_gateway/src/sink_gateway_node.cpp`

#### `ch_manager`
Purpose: publishes `ClusterInfo` (cluster membership snapshots). Also listens for recovery reassignment commands on the FANET bus and updates membership accordingly.  
Main executable:
- `ch_manager_node` (`src/ch_manager/src/ch_manager_node.cpp`)

Key files:
- `src/ch_manager/src/ch_manager_node.cpp`

#### `recovery_manager`
Purpose: watchdog + recovery coordinator: detects CH failure/unreachability and injects control messages for reassignment, task redistribution, and redeployment.  
Main executable:
- `recovery_manager_node` (`src/recovery_manager/src/recovery_manager_node.cpp`)

Key files:
- `src/recovery_manager/src/recovery_manager_node.cpp`
- `docs/recovery_cluster_rebuild_audit.md` (deep audit notes)

#### `user_devices_sim`
Purpose: inject synthetic user traffic into the FANET (periodic messages aimed at the sink).  
Main executable:
- `user_device_node` (`src/user_devices_sim/src/user_device_node.cpp`)

Key files:
- `src/user_devices_sim/src/user_device_node.cpp`

#### `network_monitor`
Purpose: subscribe to FANET bus + delivered events + status + charging mirrors; write CSV and JSON logs to `<output_dir>/<run_id>/`.  
Main executable:
- `network_monitor_node` (`src/network_monitor/src/network_monitor_node.cpp`)

Key files:
- `src/network_monitor/src/network_monitor_node.cpp`

#### `planner_viz`
Purpose: optional matplotlib-based visualization of fleet positions, task points, cluster membership, charging queue, and network stats.  
Main executable (Python entrypoint):
- `fleet_viz_node.py` (installed as `fleet_viz_node`)

Key files:
- `src/planner_viz/planner_viz/fleet_viz_node.py`

#### `system_bringup`
Purpose: orchestrate the full simulation. Provides:
- Launch file `experiment.launch.py`
- YAML run configurations under `config/runs/`
- Taskpoint files under `config/taskpoints/`

Key files:
- `src/system_bringup/launch/experiment.launch.py`
- `src/system_bringup/config/runs/example_run.yaml`
- `src/system_bringup/config/taskpoints/fixed_taskpoints.yaml`

## System Architecture

### Node graph by role

Conceptually, the system is composed of:

- **UAV nodes** (`uav_node`), running in two roles:
  - **Member UAV**: executes task points, generates telemetry, requests charging.
  - **CH UAV**: does all member behavior plus **forwarding**.
- **UGV charger** (`ugv_charger_node`): schedules charging, replies with `CHARGE_DECISION`.
- **Sink gateway** (`sink_gateway_node`): final destination for telemetry/control logs, provides an “external world” endpoint.
- **Central system services**:
  - `routing_manager_node`: builds next-hop routing tables.
  - `coverage_planner_node`: deployment + task generation; injects deployment/motion start control.
  - `weather_node`: weather regime generator.
  - `fault_injector_node`: packet drops applied between raw and processed bus.
  - `recovery_manager_node`: recovery control-plane when failures/unreachability occurs.
  - `network_monitor_node`: logging and experiment metrics.
  - `ch_manager_node`(s): cluster membership publishers (one per CH in typical scenarios).
  - optional `fleet_viz_node` for visualization.

### Key topics and message types

#### FANET bus topics
- `/fanet/network_bus_raw` — `uav_msgs/msg/TrafficMessage`  
  Where nodes publish newly generated traffic and forwarded traffic.
- `/fanet/network_bus` — `uav_msgs/msg/TrafficMessage`  
  The post-impairment bus (fault injector output). All routing/forwarding decisions are applied by nodes that subscribe here.
- `/fanet/delivered` — `uav_msgs/msg/TrafficMessage`  
  Published by a final destination node to “tap” delivery for metrics.

#### Status / routing topics (out-of-band control plane for routing)
- `/fanet/status` — `uav_msgs/msg/UavStatus`  
  Periodic beacons from UAVs, sink, and UGV (used for: physical reachability, routing computation, monitor timeseries).
- `/fanet/routing_table` — `uav_msgs/msg/RoutingTable`  
  Per-node routing table snapshots; consumers update local next-hop cache.
- `/fanet/routing_event` — `std_msgs/msg/String`  
  Nodes can emit routing events (used by routing manager to trigger recompute).
- `/routing_manager/alerts` — `std_msgs/msg/String`  
  Includes sink/UGV reachability alerts (consumed by recovery manager).

#### Coverage planning topics
- `/coverage_planner/task_points` — `uav_msgs/msg/TaskPointArray` (transient-local QoS)
- `/coverage_planner/deployment` — `uav_msgs/msg/UavDeployment`  
  **Only meaningful if `coverage_planner.accept_direct_deployment` is enabled**; otherwise deployments are carried as FANET `TrafficMessage` control.

#### Weather topics
- `/environment/weather` — `uav_msgs/msg/WeatherStatus`  
  Consumed by UAVs (battery drain) and fault injector (drop probability), and logged by monitor/viz.

#### Charging-related topics
Charging is primarily done via FANET control messages:
- `TrafficMessage(flow_type=1, control_type="CHARGE_REQUEST")`
- `TrafficMessage(flow_type=1, control_type="CHARGE_DECISION")`

Out-of-band mirrors for observability:
- `/uav_fleet/charge_requests` — `uav_msgs/msg/ChargeRequest`
- `/ugv/charge_decisions` — `uav_msgs/msg/ChargeDecision` (legacy mirror)
- `/ugv/queue_events` — `std_msgs/msg/String` (preemption and queue events)
- `/ugv/charging_snapshot` — `std_msgs/msg/String` (JSON snapshot, used by viz)

### Routing flow (encapsulation → forwarding → delivery)

All networked packets are represented as `uav_msgs/msg/TrafficMessage`.

High-level forwarding rules implemented by endpoints:
- A node processes a bus message when:
  - it is broadcast (`dst_id == "broadcast"`), OR
  - it is addressed to it as the next hop (`next_hop_id == my_id`), OR
  - in some nodes, empty `next_hop_id` is treated specially (implementation-dependent).
- If the node is final destination (`dst_id == my_id`), it:
  - publishes on `/fanet/delivered`,
  - sends an ACK (if `requires_ack == true`).
- If the node is not the final destination:
  - **CH UAVs forward** using the routing table and loop/TTL checks.
  - Members do not forward.

#### ASCII diagram: end-to-end message flow (telemetry/control)

```
[Member UAV] --(TrafficMessage on /fanet/network_bus_raw, next_hop=CH)-->
   [Fault Injector] --(maybe drop)--> /fanet/network_bus -->
      [CH UAV] --(forward, next_hop from routing table)--> /fanet/network_bus_raw -->
         [Fault Injector] --> /fanet/network_bus -->
            ... (more CH hops) ...
               [Sink Gateway] --(publish /fanet/delivered)--> [Network Monitor]
```

#### ASCII diagram: charging decision loop

```
(UAV battery low)
   |
   v
[UAV] creates CHARGE_REQUEST (TrafficMessage, dst=ugv, next_hop=CH or route)
   |
   v
FANET routing + forwarding delivers to UGV
   |
   v
[UGV charger] enqueues request, runs policy scheduler
   |
   v
[UGV charger] sends CHARGE_DECISION (accepted / rejected / preempted)
   |
   v
FANET delivers decision to UAV  -----> [UAV charging FSM transitions]
```

#### ASCII diagram: coverage command pipeline

```
[coverage_planner_node]
   |
   | publishes /coverage_planner/task_points (TaskPointArray, transient-local)
   | injects DEPLOYMENT_CMD via /fanet/network_bus_raw (TrafficMessage)
   v
FANET delivers deployment to each node
   |
   v
[UAV/UGV/Sink] update target pose
   |
   v
[coverage_planner_node] waits for DEPLOYMENT_ACK(s), then injects MOTION_START
```

#### ASCII diagram: weather influence

```
[weather_node] publishes /environment/weather (WeatherStatus)
   |                         |
   |                         +--> [fault_injector_node] computes drop probability
   |                               and may drop /fanet/network_bus_raw packets
   |
   +--> [uav_node] adjusts energy consumption rate (battery drain)
```

## Message and Data Model

This section lists **all custom message types** in `uav_msgs` and how the simulator uses them.

### `uav_msgs/msg/TrafficMessage`

This is the core encapsulation for the FANET overlay.

Fields you must understand:
- `msg_id`: unique per message; used by monitor for causality and dedup logic.
- `src_id`, `dst_id`: end-to-end addressing identifiers (string IDs such as `ch0`, `uav1`, `ugv`, `sink_gateway`).
- `next_hop_id`: hop-by-hop forwarding target.
- `last_hop_id`: set by forwarders to mark most recent transmitter.
- `flow_type`: `0=DATA`, `1=CONTROL`.
- `control_type`: string label used as the control-plane “opcode” (e.g., `HEARTBEAT`, `CHARGE_REQUEST`, `CHARGE_DECISION`, `DEPLOYMENT_CMD`, `TASK_ASSIGN`, `CLUSTER_REASSIGN`, `NEW_DEPLOYMENT`, `DROP`, `ACK`, etc.).
- `hop_count`, `ttl`: forwarding counters; TTL is a hop limit (0 means unlimited by message definition).
- `requires_ack`: application-layer ACK request.
- `payload`: free-form string. In this repo, it is typically:
  - comma-separated values for deployments,
  - semicolon-separated lists for assignments,
  - `key=value;key=value;...` for scheduling rationales,
  - plain reasons in failure/drop events.
- `ref_msg_id`: used by ACK/DROP/DECISION to refer back to an original request.
- `last_tx_time`, `last_rx_time`: timestamps used by the monitor for delivery time reconstruction.
- `drop_reason`: when `control_type=="DROP"`, used by the monitor to attribute cause.
- `recent_hops`: bounded hop history for loop detection.

Payload packing patterns present in code:
- **Deployment payload** is a comma-separated tuple containing (at minimum) IDs and pose coordinates; consumers parse by splitting on commas.
- **Charging decision payload** includes `key=value` pairs separated by `;` including `policy`, `priority`, `rank_index`, `queue_size`, and optionally `tte_sec` / `score`. Preemption uses a `reason=PREEMPTED` marker.

### `uav_msgs/msg/UavStatus`

A periodic beacon used for:
- physical reachability checks (distance vs `comm_radius_m`),
- routing computation (routing manager),
- monitor timeseries logging,
- recovery failure detection (freshness checks, battery level).

Important fields:
- `uav_id`: string ID.
- `role`: `0=MEMBER, 1=CH, 2=BACKUP_CH` (as documented in the message).
- `pose`, `velocity`
- `battery_level` (%), `battery_capacity` (Wh-like)
- `service_radius` (CH vs member coverage radius)
- `charging_state`: `0=ACTIVE, 1=GOING_TO_UGV, 2=CHARGING, 3=RETURNING`
- `intent_to_leave`, `eta_to_leave_sec`
- `stamp`: publisher timestamp used for freshness.
- `backbone_active`: whether this node participates as an active backbone element (routing/recovery use this).

### `uav_msgs/msg/RoutingTable`

Published by the routing manager; consumers cache the mapping for their own node ID.
- `node_id`: the node these routes apply to.
- `destinations[i]` + `next_hops[i]`: routing table entries.
- `stamp`: generation time.

### `uav_msgs/msg/TaskPoint` and `uav_msgs/msg/TaskPointArray`

Used by the coverage planner (and recovery manager) to describe tasks.
- `TaskPoint.id`: task identifier
- `TaskPoint.cluster_id`: assigned cluster
- `TaskPoint.position`: 3D coordinate
- `TaskPointArray.tasks[]`: full set, published transient-local.

### `uav_msgs/msg/ClusterInfo`

Published by CH managers as a membership snapshot:
- `cluster_id`, `ch_id`
- `member_ids[]`

Recovery control can override membership via `TrafficMessage(control_type="CLUSTER_REASSIGN")`.

### Charging mirror messages
- `uav_msgs/msg/ChargeRequest`: direct ROS topic mirror published by UAVs when they request charging.
- `uav_msgs/msg/ChargeDecision`: direct ROS topic mirror published by UGV charger (marked legacy in code).

### `uav_msgs/msg/WeatherStatus`

Published by weather node:
- `regime`: `"sunny" | "windy" | "stormy"`
- `rain_intensity`, `wind_speed`, `wind_direction_deg`, `temperature_c`

### `uav_msgs/msg/FailureEvent`

Published by UAV node when battery is depleted (and possibly other failures if implemented later):
- `failure_type`: `1=BATTERY_DEAD` is used in recovery/monitor logic.
- `description`, `stamp`

### Services and Actions in `uav_msgs`
- `srv/RequestCharge.srv`: defined but **not found as an active code path** in the current nodes (charging is implemented via `TrafficMessage` control flow instead).
- `srv/SendDebugText.srv`: defined; if present in runtime, it would be used for debug messaging, but does not appear as a primary runtime mechanism in the inspected node set.
- `action/DockAndCharge.action`: defined but **not found used by current nodes**; charging uses the `TrafficMessage` control loop.

### IDs / addressing rules

The routing overlay uses **string IDs** everywhere:
- `TrafficMessage.src_id/dst_id/next_hop_id`
- `UavStatus.uav_id`
- `RoutingTable.node_id`

You must ensure consistency between:
- IDs specified in YAML (`uavs`, `sink.sink_id`, `ugv.ugv_id`, `ch_manager.*`),
- IDs used as routing destinations and next hops.

A frequent pitfall is mismatch between defaults baked into nodes (e.g., some nodes default to IDs like `uav_3`) and YAML scenario IDs (e.g., `ch0`, `uav1`). Always set IDs explicitly in YAML and pass via launch.

## Subsystem Deep Dives

### Charging Subsystem Deep Dive

This subsystem spans:
- The **UAV charging finite state machine** (in `uav_node`)
- The **UGV scheduler/dock model** (in `ugv_charger_node`)
- The **charging-message protocol** (FANET control messages) plus out-of-band mirrors

#### UAV charging state machine (states + transitions)

The state is exported via `UavStatus.charging_state`:
- `0 (ACTIVE)`: normal operation (task scanning + telemetry).
- `1 (GOING_TO_UGV)`: UAV is traveling to UGV for docking/charging.
- `2 (CHARGING)`: UAV is charging (battery increases according to configured charge power).
- `3 (RETURNING)`: UAV is returning from UGV to resume mission/reconnect to CH.

Core transitions implemented in `uav_node`:
- `ACTIVE → GOING_TO_UGV` when:
  - battery hits threshold logic / intent-to-leave activates, and
  - UAV successfully receives an accepted decision (or enters emergency return logic in certain cases).
- `GOING_TO_UGV → CHARGING` when UAV reaches docking radius and has a valid slot/permission.
- `CHARGING → RETURNING` when:
  - charge completion conditions met (battery target/time window), or
  - **preemption** decision arrives (`reason=PREEMPTED`, `target_action=STOP_CHARGING`).
- `RETURNING → ACTIVE` when it reaches its cluster operational region (and resets flags); it may also be redirected by recovery commands.

If you want to document this more precisely (to the exact variable names and timers), the definitive implementation is in `src/uav_fleet/src/uav_node.cpp`.

#### UGV slot/dock management

The UGV charger maintains:
- a waiting request queue,
- an active session list (up to `max_parallel_spots`),
- a periodic scheduler loop.

Notable aspects:
- The number of parallel charging spots can be computed dynamically from fleet composition and target utilization (a “sizing” model).
- The UGV periodically emits:
  - queue events on `/ugv/queue_events`,
  - charging snapshot JSON on `/ugv/charging_snapshot` (consumed by `planner_viz`).

#### Protocols implemented

Non-preemptive selection policies:
- `fcfs`
- `edf`
- `role_priority`
- `dynamic` (dynamic-score style)

Preemptive variants exist and are configurable through parameters (and by selecting policy strings such as `p_edf`, `p_role_priority`, `p_dynamic_score` in the provided run configs). Preemption logic includes guardrails such as minimum charge time before a victim can be preempted and a minimum priority delta.

**Where protocol selection happens**
- YAML: `ugv.charging_policy` inside `system_bringup/config/runs/*.yaml`  
- Launch: `system_bringup/launch/experiment.launch.py` passes `charging_policy` into the UGV node parameter set.
- Code: `ugv_charger_node.cpp` selects queue indices based on that string.

#### Metrics measured for charging

The **network monitor** is the authoritative logger for charging metrics. It records:
- request → decision latency,
- request → dock-start waiting time,
- charge duration (when detectable from status state transitions),
- energy recovered (battery delta),
- outcomes: accepted/rejected/dropped/timeout/preempted/battery-depleted,
- decision rationale fields (policy/priority/tte/score/rank/queue_size) parsed from decision payload,
- decision-time network context (control-plane PDR/delay/drop reasons).

#### Relevant files
- `src/uav_fleet/src/uav_node.cpp`
- `src/ugv_charger/src/ugv_charger_node.cpp`
- `src/system_bringup/config/runs/ugv_*.yaml`
- `src/network_monitor/src/network_monitor_node.cpp`

#### Key parameters (YAML/ROS params)
- UGV scheduling:
  - `ugv.charging_policy`
  - preemption controls under `ugv.*` (see preemptive run configs for the parameter names in use)
- UAV charging trigger & behavior:
  - `uav.battery_threshold`
  - `uav.charge_decision_timeout_sec`
  - `uav.charge_request_retry_sec`
- Charging power / battery model coupling:
  - `uav.battery_capacity_wh_member`, `uav.battery_capacity_wh_ch`
  - computed `ugv.charger_power_w_*` values in the launch file
- Observability:
  - `monitor.decision_timeout_sec` (charge timeout classification)

#### Key topics
- FANET control:
  - `/fanet/network_bus_raw` (`CHARGE_REQUEST`, `CHARGE_DECISION`)
  - `/fanet/network_bus`
  - `/fanet/delivered` (decisions delivered)
- Mirrors / logging aid:
  - `/uav_fleet/charge_requests`
  - `/ugv/charge_decisions`
  - `/ugv/queue_events`
  - `/ugv/charging_snapshot`

### Routing and FANET Deep Dive

Routing is centralized (control-plane) while forwarding is distributed among CH UAVs (data-plane).

#### Neighbor discovery assumptions

Nodes infer physical reachability using:
- status beacons (`/fanet/status`),
- Euclidean distance between poses,
- an effective communication radius (`comm_radius_m` from status / scenario).

A node treats neighbors as “reachable” only while their status beacons are fresh.

#### Routing table: creation and expiry

`routing_manager_node`:
- reads all statuses,
- filters “active” nodes by age (status timeout),
- builds a CH backbone graph using distance threshold and hysteresis,
- assigns endpoints to a gateway CH,
- runs Dijkstra over the CH graph,
- publishes a per-node `RoutingTable` including next hops.

Expiry occurs implicitly:
- nodes that stop publishing status are removed after the timeout window,
- routes are recomputed periodically and on routing events.

#### Forwarding rules

A typical hop:
1. Sender sets:
   - `dst_id` = final destination,
   - `next_hop_id` = immediate hop (either its CH for members, or a routing-table-derived hop for CH/sink/ugv).
2. Sender publishes the message to `/fanet/network_bus_raw`.
3. Fault injector possibly drops; if not, message appears on `/fanet/network_bus`.
4. Receiver processes only if it is the `next_hop_id` (or broadcast).
5. CH receiver decides:
   - deliver if `dst_id == self_id`,
   - else forward by recomputing `next_hop_id` for `dst_id`.

Loop prevention:
- forwarding nodes can use `recent_hops` to detect loops and drop.

TTL/hop limit:
- if TTL is non-zero and the hop limit is reached, forwarders drop rather than forward.

Deliver vs forward is explicit and observable:
- delivery is recorded by publishing to `/fanet/delivered`,
- drops are typically represented as `TrafficMessage(control_type="DROP")` with a `ref_msg_id` pointing to the original message.

#### Known pitfalls and “unclear/needs confirmation” items

- Only CH UAVs forward. If too few CHs exist or backbone alignment breaks, many destinations become unreachable.
- Some nodes use string ID defaults that may not match scenario YAML; always specify IDs in config.
- A “monitor-based” termination mode exists in launch logic, but the current launch shutdown wiring only fully terminates the system in `timeout_only` mode; if you rely on monitor-driven shutdown you may need an explicit launch event handler to propagate shutdown (documentation-level recommendation; no code changes applied here).

#### Relevant files
- `src/routing_manager/src/routing_manager_node.cpp`
- `src/uav_fleet/src/uav_node.cpp`
- `src/sink_gateway/src/sink_gateway_node.cpp`
- `src/ugv_charger/src/ugv_charger_node.cpp`
- `src/fault_injector/src/fault_injector_node.cpp`
- `src/network_monitor/src/network_monitor_node.cpp`

#### Key parameters (YAML/ROS params)
- Routing manager:
  - `network.comm_range_m`
  - `routing_manager.hysteresis_margin_m`
  - `routing_manager.recompute_period_sec`
  - `routing_manager.status_timeout_sec`
- Endpoint neighbor freshness:
  - `uav.hello_timeout_sec` (UAV side)
  - `ugv.neighbor_timeout_sec`, `sink.neighbor_timeout_sec`
- TTL defaults for injected control messages:
  - `recovery.control_ttl`, `coverage_planner.control_ttl` (see node params)

#### Key topics
- `/fanet/status`
- `/fanet/routing_table`
- `/fanet/network_bus_raw`, `/fanet/network_bus`, `/fanet/delivered`
- `/fanet/routing_event`
- `/routing_manager/alerts`

### Weather Model (Markov Regime)

The weather subsystem provides global regime-driven numeric weather values used by:
- the fault injector (packet drop probability),
- UAV nodes (battery drain multiplier / energy consumption rate),
- logging and visualization.

#### States and transitions

The weather node uses three regimes:
- `sunny`
- `windy`
- `stormy`

In `mode=markov`, it advances the regime using a transition matrix; in `mode=fixed`, it holds a constant regime. Numeric values are sampled per update tick.

#### Start state and YAML controls

The run config sets:
- `weather.mode`
- `weather.start_state`
- `weather.transition_period_sec`
- `weather.update_period_sec` (publish rate)
- `weather.seed`

#### Transition period logic

Two timers exist conceptually:
- periodic publish (update period),
- regime transition (transition period) when in markov mode.

#### How weather affects battery/network

- Network effects:
  - `fault_injector_node` computes drop probability from wind, rain, and temperature deviation and drops messages on the raw bus before they reach `/fanet/network_bus`.
- Battery effects:
  - `uav_node` consumes `/environment/weather` and adjusts battery drain.

#### Relevant files
- `src/weather_server/src/weather_node.cpp`
- `src/fault_injector/src/fault_injector_node.cpp`
- `src/uav_fleet/src/uav_node.cpp`

#### Key parameters (YAML/ROS params)
- `weather.mode` (`markov` | `fixed`)
- `weather.start_state` (`sunny` | `windy` | `stormy`)
- `weather.transition_period_sec`
- `weather.update_period_sec`
- Fault injection:
  - `fault_injector.p0`, `fault_injector.p_max`
  - `fault_injector.drop_control_multiplier`, `fault_injector.drop_data_multiplier`

#### Key topics
- `/environment/weather`
- `/fanet/network_bus_raw`, `/fanet/network_bus`

### Coverage Planning and Sink/Gateway

This subsystem includes:
- taskpoint generation/publishing,
- computing initial deployment targets,
- sending deployment control commands into the FANET,
- sending `MOTION_START` after ack barrier,
- sink reception of telemetry.

#### What the coverage planner produces

- `TaskPointArray` on `/coverage_planner/task_points` (transient-local)
- Deployment commands carried as FANET control messages (`TrafficMessage` with `control_type` set to deployment opcodes)
- Optional direct `UavDeployment` messages on `/coverage_planner/deployment` if direct deployment mode is enabled

Taskpoints can be:
- loaded from a fixed YAML file,
- generated according to a configured generation mode and seed.

#### How sink/gateway relays commands into FANET

There are two deployment pathways in the codebase:
- Coverage planner can directly inject deployment commands into `/fanet/network_bus_raw`.
- Sink gateway has logic to convert `/coverage_planner/deployment` into FANET deployments; **this is only active if `coverage_planner` is configured to publish on `/coverage_planner/deployment`**.

If you want a single authoritative command source, confirm your scenario config:
- If you keep `coverage_planner.accept_direct_deployment=false`, the sink’s deployment conversion will remain mostly dormant and deployment control will come from the coverage planner’s FANET injection.

#### How UAV telemetry returns to sink

- Members generate telemetry data packets (flow_type=0) addressed to `sink_gateway`, first-hop to their CH.
- CHs forward through the backbone using routing manager tables.
- Sink gateway publishes delivery events on `/fanet/delivered`.

Centralized vs distributed:
- Planner decisions are centralized at `coverage_planner_node`.
- Routing decisions are centralized at `routing_manager_node`.
- Execution (flight, forwarding) is distributed to the UAV nodes.

#### Relevant files
- `src/coverage_planner/src/coverage_planner_node.cpp`
- `src/coverage_planner/src/coverage_planner.cpp`
- `src/sink_gateway/src/sink_gateway_node.cpp`
- `src/uav_fleet/src/uav_node.cpp`

#### Key parameters (YAML/ROS params)
- Planner:
  - `coverage_planner.num_ch`
  - area bounds: `x_min`, `x_max`, `y_min`, `y_max`
  - taskpoints: `taskpoint_generation_mode`, `fixed_taskpoints_file`, seeds
  - control retry: `deployment_cmd_retry_sec`, `deployment_cmd_max_retries`
- Sink:
  - `sink.sink_id`
  - `sink.uplink_ch_id` (fallback first hop if routing is not ready)

#### Key topics
- `/coverage_planner/task_points`
- `/fanet/network_bus_raw` (deployment + motion start control)
- `/fanet/network_bus` (ack observation)
- `/fanet/delivered`
- `/fanet/status`

### Network Monitor and Data Logging

The network monitor node is the core experiment logger. It is intentionally “omniscient”: it subscribes to the bus and delivered streams to reconstruct causality.

#### Subscriptions (concrete)

- `/fanet/network_bus` (`uav_msgs/msg/TrafficMessage`)
- `/fanet/network_bus_raw` (`uav_msgs/msg/TrafficMessage`)
- `/fanet/delivered` (`uav_msgs/msg/TrafficMessage`)
- `/uav_fleet/charge_requests` (`uav_msgs/msg/ChargeRequest`)
- `/ugv/charge_decisions` (`uav_msgs/msg/ChargeDecision`)
- `/fanet/status` (`uav_msgs/msg/UavStatus`)
- `/fanet/routing_table` (`uav_msgs/msg/RoutingTable`)
- `/environment/weather` (`uav_msgs/msg/WeatherStatus`)
- `/ugv/queue_events` (`std_msgs/msg/String`)
- Publishes: `/network_monitor/stats` (`std_msgs/msg/String`, JSON payload)

#### What it logs and where it saves

Output root:
```
<output_dir>/<run_id>/
```

Files written (all produced by `network_monitor_node`):
- `messages.csv`
- `qos_metrics.csv`
- `charge_events.csv`
- `recovery_events.csv`
- `preemption_events.csv`
- `status_timeseries.csv`
- `charge_queue_timeseries.csv`
- `weather_timeseries.csv`
- `summary.json`

#### CSV header schemas (example header lines)

These headers are generated by the node; the exact columns are:

`messages.csv`
```
run_id,msg_id,flow_type,control_type,src_id,dst_id,creation_time_s,delivered_time_s,delivered,e2e_delay_ms,forward_count,hop_count,ttl_hops,payload_bytes,dropped,drop_reason,dropper_id,ack_time
```

`qos_metrics.csv`
```
run_id,flow_type,control_type,generated,delivered,dropped,pdr,delay_mean_ms,delay_p95_ms,jitter_ms,throughput_bps,generated_bps,qos_score
```

`charge_events.csv`
```
run_id,request_msg_id,uav_id,ugv_id,role,outcome,failure_reason,request_time,decision_time,dock_start_time,charge_end_time,decision_latency_ms,waiting_time_ms,charge_duration_ms,charge_completed,request_battery,start_battery,end_battery,energy_recovered,preempted_flag,preempt_count,decision_policy,decision_priority,decision_tte_sec,decision_score,decision_rank_index,decision_queue_size,decision_ctrl_pdr,decision_ctrl_delay_mean_ms,decision_ctrl_delay_p95_ms,decision_ctrl_drop_reasons
```

`preemption_events.csv`
```
run_id,time,victim_uav_id,winner_uav_id,victim_role,winner_role,victim_priority,winner_priority,delta_priority,victim_charge_time_s,policy
```

`status_timeseries.csv`
```
run_id,time,uav_id,role,charging_state,battery_level,backbone_active,x,y,z,energy_consumption_rate
```

`charge_queue_timeseries.csv`
```
run_id,time,queue_length,queue_length_ch,queue_length_member,queue_length_unknown,active_charging,ugv_dock_capacity,ugv_dock_utilization,mean_wait_ch_ms,mean_wait_member_ms
```

`weather_timeseries.csv`
```
run_id,time,regime,temperature_c,wind_speed,wind_direction_deg,rain_intensity
```

#### How run_id / protocol / seed are recorded

- `run_id` is recorded in every output row as a column (and as directory name).
- Charging policy attribution is done via:
  - `decision_policy` parsed from the delivered `CHARGE_DECISION` payload, and
  - the external post-processing script can map `run_id → protocol label` via a CSV.

Seed recording:
- Seeds are configured in the run YAML and passed via launch. The monitor does not automatically export the full config as metadata in current code; if you need full reproducibility metadata, add a single `metadata.json` emitter at launch time (documentation suggestion) or store the YAML alongside the run folder manually.

#### Relevant files
- `src/network_monitor/src/network_monitor_node.cpp`
- `tools/charging_protocol_compare.py`

#### Key parameters (YAML/ROS params)
- `monitor.run_id`, `monitor.output_dir`
- `monitor.csv_write_period_sec`
- `monitor.decision_timeout_sec`
- termination-related:
  - `monitor.max_runtime_sec`
  - `monitor.stop_on_backbone_loss`
  - `monitor.routing_table_empty_shutdown_sec`
- QoS scoring:
  - `monitor.qos_target_pdr`, `monitor.qos_target_delay_ms`, `monitor.qos_target_jitter_ms`
  - `monitor.qos_weight_*`

#### Key topics
- `/fanet/network_bus_raw`, `/fanet/network_bus`, `/fanet/delivered`
- `/fanet/status`, `/environment/weather`
- `/uav_fleet/charge_requests`, `/ugv/queue_events`
- `/network_monitor/stats`

### Recovery Logic and Failure Events

Recovery exists as a first-class subsystem.

What it does:
- Detects CH “death” based on stale status/heartbeat and explicit battery-dead failures.
- Triggers recovery epochs with `RECOVERY_START`/`RECOVERY_DONE` broadcasts.
- Reassigns members to CHs via `CLUSTER_REASSIGN`.
- Redistributes tasks via `TASK_ASSIGN`.
- Issues `NEW_DEPLOYMENT` to reposition CHs for coverage/reconnectivity.
- Provides fallback when no CH is alive (`MEMBER_FALLBACK`).

This is not just conceptual; it is implemented as FANET control injection with ACK retry.

#### Relevant files
- `src/recovery_manager/src/recovery_manager_node.cpp`
- `docs/recovery_cluster_rebuild_audit.md`
- `src/uav_fleet/src/uav_node.cpp`
- `src/ch_manager/src/ch_manager_node.cpp`
- `src/routing_manager/src/routing_manager_node.cpp`

#### Key parameters (YAML/ROS params)
- `recovery.comm_range_m`
- `recovery.status_timeout_sec`
- `recovery.heartbeat_timeout_sec`
- `recovery.recovery_cooldown_sec`
- `recovery.control_ttl`
- `recovery.ack_retry_period_sec`, `recovery.max_ack_retries`
- `recovery.recovery_lock_duration_sec` (lock to prevent CH manager from overwriting reassignment too quickly)

#### Key topics
- `/fanet/status`
- `/fanet/network_bus` (observes `HEARTBEAT`, `ACK`)
- `/fanet/network_bus_raw` (injects recovery controls)
- `/routing_manager/alerts`
- `/uav_fleet/failure_events`
- `/coverage_planner/task_points`
- `/ch_manager/cluster_info`

## Operations

### Running the Simulator

#### Build

From repo root (ROS 2 workspace):

```bash
colcon build --symlink-install
```

#### Source

```bash
source install/setup.bash
```

#### Launch a default scenario

The main entrypoint is:

```bash
ros2 launch system_bringup experiment.launch.py \
  config:=system_bringup/config/runs/example_run.yaml \
  run_id:=demo_run \
  output_dir:=log
```

Notes:
- `config` is the run YAML path inside the `system_bringup` package.
- `run_id` becomes the subfolder name under `output_dir`.

#### Run an experiment with a specified YAML config

Example (charging policy scenario):

```bash
ros2 launch system_bringup experiment.launch.py \
  config:=system_bringup/config/runs/ugv_edf.yaml \
  run_id:=edf_seed2 \
  output_dir:=log
```

#### Change protocol selection

Edit or choose a YAML under:
- `system_bringup/config/runs/`

Make sure it contains:
- `ugv.charging_policy: <policy_string>`

Common values present in the repo configs include:
- `fcfs`, `edf`, `role_priority`, `dynamic`
- preemptive variants: `p_edf`, `p_role_priority`, `p_dynamic_score`

#### Change runtime timeout

Use:
- `global.experiment_timeout_s`
- `global.termination_mode: timeout_only`

In practice, `timeout_only` is the only mode that cleanly shuts down the entire launch after the timeout (via a launch shutdown event). If you switch to a monitor-driven termination mode, verify shutdown propagation in your environment.

#### Change weather mode

In YAML:
- `weather.mode: fixed` and `weather.start_state: sunny|windy|stormy`
or
- `weather.mode: markov` with `weather.transition_period_sec`

#### Enable visualization

If your config/launch supports it, enable planner visualization:
- Launch argument: `enable_planner_viz:=true` (see launch file)
- If running headless, set environment:
```bash
export FLEET_VIZ_HEADLESS=1
```

### Experiment Methodology (For Reproducible Comparisons)

#### Ensure comparable runs

Recommended baseline for protocol comparisons:
- Use `termination_mode: timeout_only` with a fixed `experiment_timeout_s`.
- Fix:
  - `global.rng_seed`
  - `weather.seed`
  - taskpoint generation mode + seed (or fixed file)
  - network radius parameters
  - battery/charge parameters
- Keep identical topology (same number of UAVs, same CH/member split) across policy runs.

#### Recommended run matrix

A typical matrix for charging protocol comparison:

- **Policy** × **Weather mode** × **Seed**
  - Policy: `fcfs`, `edf`, `role_priority`, `dynamic`, plus `p_*` variants if you study preemption
  - Weather: fixed regimes (`sunny`, `windy`, `stormy`) and Markov
  - Seeds: e.g., 1–5 (or higher if you need statistically stable results)

#### Expected plots/metrics from outputs

From `<output_dir>/<run_id>/`:
- `qos_metrics.csv`: PDR and delay distributions per flow/control category
- `messages.csv`: per-message causality (generated/delivered/dropped, hop count, delay)
- `charge_events.csv`: charging performance vs policy
- `preemption_events.csv`: preemption analysis
- `status_timeseries.csv`: survival and battery trajectory
- `summary.json`: single-file aggregated outcome snapshot

Postprocess:
```bash
python3 tools/charging_protocol_compare.py \
  --log-root log \
  --output-dir analysis/charging_protocol_comparison
```

### Troubleshooting

#### Nodes fail to launch due to wrong executable/package names

Symptom:
- launch errors like “executable not found”.

Cause:
- The launch file supports an `executables:` mapping in YAML. If it’s missing/inconsistent, it may try old names.

Fix:
- Ensure `executables` block matches current package/executable names:
  - `uav_fleet/uav_node`
  - `ugv_charger/ugv_charger_node`
  - `routing_manager/routing_manager_node`
  - `fault_injector/fault_injector_node`
  - etc.

#### No traffic delivered to sink

Diagnose:
```bash
ros2 topic echo /fanet/routing_table
ros2 topic echo /routing_manager/alerts
ros2 topic echo /fanet/network_bus | grep -E "dst_id: sink_gateway|control_type: DROP"
```

Check:
- Do CHs exist and have `role=1`?
- Is `routing_manager_node` publishing non-empty routes?
- Is sink reachable according to `/routing_manager/alerts`?

#### Charging requests time out / no decisions arrive

Diagnose:
```bash
ros2 topic echo /fanet/network_bus | grep -E "CHARGE_REQUEST|CHARGE_DECISION|DROP|ACK"
ros2 topic echo /uav_fleet/charge_requests
ros2 topic echo /ugv/charging_snapshot
```

Check:
- Is the UGV reachable in routing alerts?
- Is the fault injector dropping control traffic too aggressively?
- Are UAV IDs and `ugv_id` consistent between YAML and node parameters?

#### Recovery thrashing or unexpected reassignment

Diagnose:
```bash
ros2 topic echo /fanet/network_bus | grep -E "RECOVERY_START|RECOVERY_DONE|CLUSTER_REASSIGN|TASK_ASSIGN|NEW_DEPLOYMENT"
ros2 topic echo /routing_manager/alerts
```

Check:
- timeout thresholds (`status_timeout_sec`, `heartbeat_timeout_sec`) might be too tight for your publish rates.
- CH manager publishes static membership; recovery can override membership; verify the intended control-plane precedence.

### Development Notes

#### Add a new charging protocol cleanly

Where to implement:
- `src/ugv_charger/src/ugv_charger_node.cpp`

Suggested pattern:
1. Add a new policy string constant (e.g., `my_policy`).
2. Implement a queue ranking function that returns:
   - selected index,
   - plus rationale fields (policy name, priority, rank_index, queue_size, optional score/tte).
3. Ensure the decision payload encodes the rationale as `key=value;...` so the monitor can parse it.
4. Update run configs under `system_bringup/config/runs/` with a new YAML that sets `ugv.charging_policy`.

#### Add a new monitored signal without breaking schema

Where to implement:
- `src/network_monitor/src/network_monitor_node.cpp`

Rules:
- Prefer adding **new columns** to CSV with defaults for older rows, rather than changing existing column semantics.
- Keep `run_id` as first column for joinability.
- If you add a new per-event file, also add it to `writeOutputs()` to ensure it is flushed periodically and on shutdown.

#### Clarifications recommended (docs/logging suggestions, not code changes)

If you want the README to be “self-checking”:
- Add a tiny “run metadata” emitter (JSON) at launch time that stores:
  - config path,
  - selected charging_policy,
  - seeds,
  - termination mode + timeout,
  - git commit hash.
- Document the authoritative ID map used by each scenario (sink_id, ugv_id, CH IDs, member IDs) so routing and charging do not silently fail from naming mismatches.

