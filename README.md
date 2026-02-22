# UAV–UGV Cooperative FANET Simulator (ROS 2)

This repository is a **ROS 2 simulation workspace** for **UAV–UGV cooperative disaster monitoring**, focused on studying how **network routing (FANET)** and **UGV-based charging scheduling** affect mission performance under mobility, weather, and failures. The simulator implements an explicit **application-layer FANET overlay**, where inter-agent packets (telemetry, coordination, charging requests/decisions, and recovery controls) are encapsulated as `uav_msgs/msg/TrafficMessage` and routed hop-by-hop using `next_hop_id`. The FANET is modeled via a **two-stage bus** (`/fanet/network_bus_raw → /fanet/network_bus`) with optional weather-driven impairment by the fault injector. Routing is **centralized**: `routing_manager_node` computes per-node next hops from `/fanet/status` beacons and publishes `/fanet/routing_table`. UAV behavior including **CH forwarding**, **battery + mobility**, **ACK logic**, and **charging state machine** is implemented in a single `uav_node`. The system is orchestrated by `system_bringup/launch/experiment.launch.py`, which loads a run YAML configuration (protocol/seed/runtime/weather knobs) and enforces a **timeout-based termination** policy.

## Glossary

**ACK**: Application-level acknowledgement modeled as `TrafficMessage` with `control_type="ACK"` and `ref_msg_id` referencing the original message.  
**Backbone**: The subset of nodes that forward packets. In this codebase, **only CH UAVs (role=1)** forward.  
**CH (Cluster Head)**: UAV role `role=1` that participates in backbone routing and forwards packets.  
**Member**: UAV role `role=0` that generates telemetry and requests charging but **does not forward** packets.  
**Delivered tap**: `/fanet/delivered` topic used as the “delivery event stream” for metrics/logging.  
**FANET**: Flying Ad-hoc Network; here, an application-layer routing/forwarding overlay implemented using ROS topics and `TrafficMessage` encapsulation.  
**Next hop**: `TrafficMessage.next_hop_id`, the immediate hop receiver for this hop.  
**Sink / Gateway**: `sink_gateway_node`, the end destination for most telemetry/control-plane reporting and deployment barriers.  
**TTL**: `TrafficMessage.ttl` hop limit (the `.msg` comment indicates `0` means “unlimited”; forwarding code should be checked for exact semantics).  
**UGV**: Ground vehicle implementing a multi-slot/multi-policy charging scheduler (`ugv_charger_node`).  
**FCFS / EDF / Role Priority / Dynamic Score**: Charging policies selected by string parameter `ugv.charging_policy` in run configs (see `system_bringup/config/runs/*.yaml`).  
**Preemption**: Charging policy mode where an ongoing charging session can be interrupted to serve a higher-priority UAV (preemptive run configs use `p_*` policy strings).

## Project Overview

### What the simulator is for

This project models a disaster monitoring mission where UAVs operate as **clusters** (CH + members), execute area tasks, and must deliver telemetry and control traffic through a **FANET routing overlay**, while coping with:

- **Battery drain + charging logistics** via a UGV with finite parallel charging capacity.
- **Weather regimes** that can affect **battery consumption** and **network packet drops**.
- **Network partitions and CH failures**, with explicit recovery actions injected as FANET control.

Key design principle: **all inter-agent packets (control + telemetry + charging)** are encapsulated and handled via FANET routing using `uav_msgs/msg/TrafficMessage`. In practice, some global/system signals are intentionally out-of-band (e.g., `/fanet/status` beacons and `/environment/weather`) to support routing computation and physical modeling.

### What questions it helps answer

This simulator is designed to support reproducible comparisons such as:

- Charging policy impact on waiting time, fairness, mission survivability (battery deaths), and preemption rates.
- Routing + FANET impairment impact on delivery ratio, hop counts, control-plane reliability (ACKs), and latency.
- Weather regime impact on both **packet loss** (fault injector) and **battery drain** (UAV energy model).
- Recovery behavior under CH failure, CH timeouts, and sink/UGV unreachability alerts.

### Relevant files

- `src/system_bringup/launch/experiment.launch.py`
- `src/system_bringup/config/runs/*.yaml`
- `src/uav_fleet/src/uav_node.cpp`
- `src/routing_manager/src/routing_manager_node.cpp`
- `src/fault_injector/src/fault_injector_node.cpp`
- `src/weather_server/src/weather_node.cpp`
- `src/sink_gateway/src/sink_gateway_node.cpp`
- `src/coverage_planner/src/coverage_planner_node.cpp`
- `src/recovery_manager/src/recovery_manager_node.cpp`

### Key parameters (YAML / ROS params)

These are *scenario-level* knobs surfaced in run YAML and passed through the bringup launch:

- Termination: `global.termination_mode`, `global.experiment_timeout_s`
- IDs: `sink.sink_id`, `ugv.ugv_id`, UAV IDs under `uavs.*`, CH IDs under `clusters.*`
- Network: `network.comm_radius_m` (also passed into routing manager and UAV reachability checks)
- Charging policy: `ugv.charging_policy` (plus preemption knobs under `ugv.*`)
- Weather: `weather.mode`, `weather.start_state`, `weather.macrostate_period_sec` (plus transition matrix if enabled)

### Key topics

- FANET bus: `/fanet/network_bus_raw`, `/fanet/network_bus`, `/fanet/delivered`
- Routing control-plane: `/fanet/status`, `/fanet/routing_table`, `/fanet/routing_event`, `/routing_manager/alerts`
- Coverage planning: `/coverage_planner/task_points`, `/coverage_planner/deployment`
- Weather: `/environment/weather`
- Failure events: `/uav_fleet/failure_events`

## Repository Layout

This repository is a ROS 2 workspace. Authoritative source is under `src/`. The repository also contains `build/` and `install/` directories, which are build artifacts and should not be used as the source of truth when documenting behavior.

### Package tree

```
src/
  ch_manager/
  coverage_planner/
  fault_injector/
  network_monitor/
  planner_viz/
  recovery_manager/
  routing_manager/
  sink_gateway/
  system_bringup/
  uav_fleet/
  uav_msgs/
  ugv_charger/
  user_devices_sim/
  weather_server/
tools/
docs/
```

### Package details

#### `uav_msgs`

Purpose: project-wide messages/services/actions.  
Main artifacts (declared in `src/uav_msgs/CMakeLists.txt`):

- `msg/TrafficMessage.msg`
- `msg/UavStatus.msg`
- `msg/Heartbeat.msg`
- `msg/ClusterInfo.msg`
- `msg/TaskPoint.msg`, `msg/TaskPointArray.msg`
- `msg/ChargeRequest.msg`, `msg/ChargeDecision.msg`
- `msg/WeatherStatus.msg`
- `msg/FailureEvent.msg`
- `msg/UavDeployment.msg`
- `msg/RoutingTable.msg`
- `srv/RequestCharge.srv`, `srv/SendDebugText.srv`
- `action/DockAndCharge.action`

Main risk to avoid: **assuming services/actions are used at runtime**. Most charging/control flow in the current nodes is implemented via `TrafficMessage`. Verify usage via node code before relying on services/actions.

Key files:

- `src/uav_msgs/msg/TrafficMessage.msg`
- `src/uav_msgs/msg/UavStatus.msg`
- `src/uav_msgs/CMakeLists.txt`

#### `uav_fleet`

Purpose: UAV behavioral model (both CH and member) including mobility, battery model, telemetry generation, FANET send/receive, ACK, buffering, charging FSM, and recovery command handling.  
Main node/executable:

- `uav_node` (`src/uav_fleet/src/uav_node.cpp`)

Key files:

- `src/uav_fleet/src/uav_node.cpp`

#### `routing_manager`

Purpose: centralized routing computation. Builds CH backbone graph from `/fanet/status`, assigns endpoints to gateway CH, runs shortest-path routing over CH graph, publishes per-node `/fanet/routing_table`, emits reachability alerts.  
Main node/executable:

- `routing_manager_node` (`src/routing_manager/src/routing_manager_node.cpp`)

Key files:

- `src/routing_manager/src/routing_manager_node.cpp`

#### `fault_injector`

Purpose: weather-driven impairment of FANET traffic, transforming raw bus packets into processed bus packets with drop probability dependent on `/environment/weather`.  
Main node/executable:

- `fault_injector_node` (`src/fault_injector/src/fault_injector_node.cpp`)

Key files:

- `src/fault_injector/src/fault_injector_node.cpp`

#### `weather_server`

Purpose: global weather generator. Supports Markov regime transitions or fixed regime mode.  
Main node/executable:

- `weather_node` (`src/weather_server/src/weather_node.cpp`)

Key files:

- `src/weather_server/src/weather_node.cpp`

#### `sink_gateway`

Purpose: sink endpoint for traffic. Receives FANET traffic whose final destination matches the sink ID, publishes delivery events on `/fanet/delivered`, sends ACKs when required. Also handles initial deployment barrier logic and CH status summaries.  
Main node/executable:

- `sink_gateway_node` (`src/sink_gateway/src/sink_gateway_node.cpp`)

Key files:

- `src/sink_gateway/src/sink_gateway_node.cpp`

#### `coverage_planner`

Purpose: deployment planner and taskpoint generator. Publishes taskpoints, computes CH/member initial placements, places sink and UGV, and injects deployment/motion-start control into FANET. Also observes ACK-style traffic on the bus for barriers.  
Main node/executable:

- `coverage_planner_node` (`src/coverage_planner/src/coverage_planner_node.cpp`)

Key files:

- `src/coverage_planner/src/coverage_planner_node.cpp`
- `src/coverage_planner/src/coverage_planner.cpp`, `src/coverage_planner/src/coverage_planner.hpp` (layout logic)

#### `ugv_charger`

Purpose: UGV charging dock and scheduler. Receives charging requests via FANET control messages and returns decisions via FANET. Policy selection is configured by `ugv.charging_policy` in run YAML.  
Main node/executable:

- `ugv_charger_node` (`src/ugv_charger/src/ugv_charger_node.cpp`)

Key files:

- `src/ugv_charger/src/ugv_charger_node.cpp` (must be treated as authoritative for exact policy semantics)

#### `network_monitor`

Purpose: experiment logger and metric aggregator. Subscribes to bus + delivery + status + charging mirrors and writes per-run logs to `output_dir/run_id`.  
Main node/executable:

- `network_monitor_node` (`src/network_monitor/src/network_monitor_node.cpp`)

Key files:

- `src/network_monitor/src/network_monitor_node.cpp`
- `tools/charging_protocol_compare.py` (postprocessing expects specific log files/columns)

#### `ch_manager`

Purpose: cluster membership publisher and membership updater on recovery. Publishes `ClusterInfo` periodically, listens for `CLUSTER_REASSIGN` control on FANET to update the membership list.  
Main node/executable:

- `ch_manager_node` (`src/ch_manager/src/ch_manager_node.cpp`)

Key files:

- `src/ch_manager/src/ch_manager_node.cpp`

#### `recovery_manager`

Purpose: recovery watchdog and centralized recovery coordinator. Detects CH failure/timeouts and sink/UGV reachability alerts and injects recovery control messages (`RECOVERY_*`, `CLUSTER_REASSIGN`, `TASK_ASSIGN`, `NEW_DEPLOYMENT`, `MEMBER_FALLBACK`) with ACK retry.  
Main node/executable:

- `recovery_manager_node` (`src/recovery_manager/src/recovery_manager_node.cpp`)

Key files:

- `src/recovery_manager/src/recovery_manager_node.cpp`
- `docs/recovery_cluster_rebuild_audit.md` (deep audit and known gaps)

#### `user_devices_sim` （Old logic not used）

Purpose: simulated user device traffic injection. This package is typically enabled/disabled via run YAML.  
Main node/executable:

- `user_device_node` (`src/user_devices_sim/src/user_device_node.cpp`)

Key files:

- `src/user_devices_sim/src/user_device_node.cpp`

#### `planner_viz`

Purpose: optional visualization (Python). Intended for development/debug; may be disabled in headless runs.  
Main node/executable:

- `fleet_viz_node` (Python entrypoint, check `src/planner_viz/`)

Key files:

- `src/planner_viz/planner_viz/fleet_viz_node.py`

#### `system_bringup`

Purpose: system orchestration: launch file + run configs + taskpoint files.  
Main launch entrypoint:

- `src/system_bringup/launch/experiment.launch.py`

Run configs:

- `src/system_bringup/config/runs/*.yaml`
- Example: `src/system_bringup/config/runs/example_run.yaml`
- Charging policy sweeps: `ugv_fcfs.yaml`, `ugv_edf.yaml`, `ugv_role_priority.yaml`, `ugv_dynamic.yaml`
- Preemptive configs: `ugv_p_edf.yaml`, `ugv_p_role_priority.yaml`, `ugv_p_dynamic_score.yaml`

Taskpoints:

- `src/system_bringup/config/taskpoints/fixed_taskpoints.yaml`

## System Architecture

### Node graph by role

The system can be understood as a set of roles attached to specific ROS nodes:

**UAV member (`uav_node`, role=0)**  
Runs task/mobility and telemetry generation; sends traffic with first hop to its CH; requests charging; processes control messages but does not forward.

**UAV CH (`uav_node`, role=1)**  
Runs all member behavior plus: forwards multi-hop FANET traffic (CH backbone); publishes CH-specific status messages and can act as gateway for members.

**UGV charger (`ugv_charger_node`)**  
Receives charge requests, maintains queue/dock model, sends charge decisions, publishes UGV status beacons for routing.

**Sink/gateway (`sink_gateway_node`)**  
Final destination for traffic; publishes delivery markers; sends ACKs; injects deployments and motion-start barriers.

**Routing manager (`routing_manager_node`)**  
Centralized routing computation and reachability alerts.

**FANET impairment (`fault_injector_node`)**  
Optional packet drop stage between raw bus and processed bus driven by weather.

**Weather (`weather_node`)**  
Publishes global weather status costed by drift and Markov regime transitions.

**Coverage planner (`coverage_planner_node`)**  
Creates taskpoints, computes initial deployment, injects deployment and motion start control into the FANET.

**Recovery manager (`recovery_manager_node`)**  
Watchdog on CH timeouts/failures/unreachability; injects recovery control messages.

**Cluster membership (`ch_manager_node`, typically one per cluster)**  
Publishes `ClusterInfo` and updates membership when recovery reassigns members.

**Monitor (`network_monitor_node`)**  
Subscribes and writes logs. Treated as “omniscient observer”.

### Key topics and message types

#### FANET bus topics

- `/fanet/network_bus_raw` — `uav_msgs/msg/TrafficMessage`  
  Source bus: nodes (UAVs, sink, recovery, planner, UGV) publish newly created and forwarded messages here.

- `/fanet/network_bus` — `uav_msgs/msg/TrafficMessage`  
  Processed bus: produced by fault injector (or direct equivalent if fault injector disabled). Nodes that *receive/forward* traffic subscribe here.

- `/fanet/delivered` — `uav_msgs/msg/TrafficMessage`  
  Delivery tap: published by final destinations to expose deliveries for metrics. In current code, `uav_node` and `sink_gateway_node` explicitly publish delivered markers for both final-destination packets and some special cases.

#### Status / routing control-plane topics

These are out-of-band supports for routing and reachability modeling:

- `/fanet/status` — `uav_msgs/msg/UavStatus`  
  Periodic state beacons for routing computation, neighbor discovery assumptions, and monitor/recovery.

- `/fanet/routing_table` — `uav_msgs/msg/RoutingTable`  
  Routing snapshots published by routing manager. Each node consumes only the entries where `RoutingTable.node_id == my_id`.

- `/fanet/routing_event` — `std_msgs/msg/String`  
  Event-driven recompute triggers for routing manager (e.g., “no route” notifications).

- `/routing_manager/alerts` — `std_msgs/msg/String`  
  Reachability alerts (`SINK_REACHABLE/UNREACHABLE`, `UGV_REACHABLE/UNREACHABLE`) used by recovery manager.

#### Charging control + mirrors

Charging is primarily encoded as FANET control (TrafficMessage):

- `TrafficMessage(flow_type=1, control_type="CHARGE_REQUEST")`
- `TrafficMessage(flow_type=1, control_type="CHARGE_DECISION")`

In addition, UAV nodes publish an out-of-band mirror topic:

- `/uav_fleet/charge_requests` — `uav_msgs/msg/ChargeRequest`

UGV charger may publish additional mirrors depending on its implementation; treat `src/ugv_charger/src/ugv_charger_node.cpp` as authoritative for exact topic names.

#### Coverage topics

- `/coverage_planner/task_points` — `uav_msgs/msg/TaskPointArray` (transient-local QoS)
- `/coverage_planner/deployment` — `uav_msgs/msg/UavDeployment`  
  Note: UAV nodes default to **not** accepting direct deployments; they expect deployment via FANET control messages unless `accept_direct_deployment` parameters are enabled.

#### Weather topics

- `/environment/weather` — `uav_msgs/msg/WeatherStatus`

### Routing flow

Routing is **control-plane centralized** but **data-plane forwarding distributed**:

- `routing_manager_node` builds per-node next-hop tables and publishes `/fanet/routing_table`.
- Members route by always sending to their CH (`my_ch_id`), while CHs use the routing table to forward.
- Forwarding uses `TrafficMessage.next_hop_id` to decide who should process each hop.

A `TrafficMessage` hop is modeled as:

1. Sender selects `next_hop_id` for the destination.
2. Sender publishes to `/fanet/network_bus_raw`.
3. Fault injector forwards or drops the message onto `/fanet/network_bus`.
4. Receiver checks whether it is the hop destination (`next_hop_id == my_id`) or whether the message is broadcast (`dst_id == "broadcast"`). If not, it ignores it.
5. If receiver is final destination (`dst_id == my_id`), it publishes the message to `/fanet/delivered`, and optionally emits an ACK.
6. Otherwise, if receiver is a CH, it forwards by re-writing `next_hop_id` to the next hop toward the final destination.

### ASCII diagrams

#### End-to-end message flow: UAV → FANET → routing → sink/UGV

```
[UAV Member]
  |  create TrafficMessage (dst=sink_gateway, next_hop=my_ch_id)
  v
/fanet/network_bus_raw
  |
  | (optional impairment)
/fault_injector_node
  v
/fanet/network_bus
  |
  v
[UAV CH]  (receives because next_hop_id == CH)
  |  if dst != CH: forward -> new next_hop_id from /fanet/routing_table
  v
/fanet/network_bus_raw  (forwarded hop)
  |
  v
  ... (more CH hops) ...
  |
  v
[Sink Gateway or UGV]
  | publish to /fanet/delivered
  | send ACK if requires_ack
  v
[Monitor observes delivery + bus for metrics]
```

#### Charging decision loop

```
[UAV] battery drops -> decides to request charge
  | create CHARGE_REQUEST TrafficMessage (dst=ugv_id)
  v
FANET routing delivers to UGV
  |
  v
[UGV charger] enqueues request -> runs policy -> produces decision
  | create CHARGE_DECISION TrafficMessage (dst=requester_uav_id)
  v
FANET routing delivers to UAV
  |
  v
[UAV] charging FSM transitions
  - ACTIVE -> GOING_TO_UGV -> CHARGING -> RETURNING
  - possible STOP_CHARGING on preemption decision
```

#### Coverage command pipeline

```
[coverage_planner_node]
  | publish TaskPointArray (transient local)
/coverage_planner/task_points
  |
  | compute deployment poses
  | inject DEPLOYMENT_CMD control to /fanet/network_bus_raw
  v
FANET delivers deployment cmd to UAV/UGV/sink
  |
  v
UAVs ACK deployment to sink (DEPLOYMENT_ACK), sink may gate mobility
  |
  v
[sink_gateway_node] (and/or planner) inject MOTION_START
  |
  v
UAVs start mobility toward deployment targets and then task execution
```

#### Weather regime influence on battery/network

```
[weather_node] publishes /environment/weather (WeatherStatus)
  |                          |
  |                          +--> [fault_injector_node] computes drop probability
  |                               and may drop packets between raw->processed bus
  |
  +--> [uav_node] uses wind/rain/temp to modify energy consumption rate
       (battery drain changes -> earlier/later charging triggers)
```

## Message & Data Model

This section lists **all custom message types in `uav_msgs`** and documents how they are used in the simulator.

### `uav_msgs/msg/TrafficMessage`

Core FANET encapsulation. Key fields:

- `msg_id` (string): unique; used for dedup, ACK, and logging.  
- `src_id`, `dst_id` (string): end-to-end identifiers. `dst_id="broadcast"` is treated as broadcast in UAV code.  
- `next_hop_id` (string): hop recipient. Receiver-side code generally processes a message only when `next_hop_id == my_id` (broadcast is special).  
- `last_hop_id` (string): can be used to mark previous forwarder.  
- `flow_type` (uint8): `0=DATA`, `1=CONTROL`.  
- `control_type` (string): control opcode (examples implemented in UAV/sink/recovery code include `HEARTBEAT`, `ACK`, `DEPLOYMENT`, `DEPLOYMENT_CMD`, `MOTION_START`, `STATUS_CH`, `CHARGE_REQUEST`, `CHARGE_DECISION`, `RECOVERY_START`, `RECOVERY_DONE`, `CLUSTER_REASSIGN`, `TASK_ASSIGN`, `NEW_DEPLOYMENT`, `MEMBER_FALLBACK`, `FAILURE_EVENT`, `DROP`).  
- `hop_count`, `ttl` (uint32): hop accounting and hop limit.  
- `requires_ack` (bool): when true, receiver attempts to send back `ACK` referencing `msg_id`.  
- `payload` (string): packed control payload; see “payload packing” below.  
- `creation_time` (time): used for ordering, logging, and latency estimation.  
- `ref_msg_id` (string): used by ACK, DROP, and other derived messages to point back to the original.

Payload packing conventions in current code:

- **Deployment payload** (used by `DEPLOYMENT` and `DEPLOYMENT_CMD`): comma-separated tuple  
  `role,cluster_id,ch_id,x,y,z,next_hop_to_sink,next_hop_to_ugv`  
  (with `-` used as placeholder when next hop strings are empty).  
- **Task assignment payload** (used by recovery `TASK_ASSIGN`): semicolon-separated list of `x,y,z` tuples.  
- **Member fallback payload** (`MEMBER_FALLBACK`): `TARGET_LABEL,x,y,z` where `TARGET_LABEL` is `"UGV"` or `"SINK"`.  
- **Cluster reassign payload** (`CLUSTER_REASSIGN`): new CH ID as a string.
- **CH status payload** (`STATUS_CH`): comma-separated `x,y,battery,state` parsed at sink.

If you introduce a new control opcode, prefer a payload encoding that can be parsed deterministically (CSV tuples or `key=value;key=value` pairs), and document it with the opcode.

### `uav_msgs/msg/UavStatus`

Periodic beacon used for:

- reachability (distance vs `comm_radius_m`),
- routing computation (`routing_manager_node`),
- recovery detection (freshness and battery level).

Important fields:

- `uav_id` (string): the *network identity*; **this is what routing keys on**.  
- `role` (uint8): `0=MEMBER`, `1=CH`, `2=BACKUP_CH` (see `.msg` comments).  
- `pose` (`geometry_msgs/Pose`): used for distance computations.  
- Battery: `battery_level` (%) and `battery_capacity` (energy scale).  
- Movement/charge: `charging_state` enum (0..3), `intent_to_leave`, `eta_to_leave_sec`.  
- Network: `comm_radius_m`, plus optional fields like `traffic_load` and `packet_loss_estimate`.  
- `stamp`: freshness time used by routing/recovery.

### `uav_msgs/msg/RoutingTable`

Published by `routing_manager_node` as per-node tables:

- `node_id`: the node for which this table is intended.
- `destinations[]` and `next_hops[]`: parallel arrays; empty `next_hops[i]` means unreachable.
- `stamp`: generation time.

### `uav_msgs/msg/TaskPoint` and `uav_msgs/msg/TaskPointArray`

From coverage planner:

- `TaskPoint.id`
- `TaskPoint.cluster_id` (string)
- `TaskPoint.position` (Point)
- `TaskPointArray.tasks[]`

The taskpoint topic is transient-local so late-joining nodes (e.g., visualizer) can obtain the latest snapshot.

### `uav_msgs/msg/ClusterInfo`

From `ch_manager_node`:

- `cluster_id`, `ch_id`, `member_ids[]`

Members also accept membership changes via recovery `CLUSTER_REASSIGN` FANET control.

### `uav_msgs/msg/ChargeRequest` and `uav_msgs/msg/ChargeDecision`

These are used as out-of-band mirrors and (depending on UGV implementation) may also be published for logging/visualization. The authoritative charging control protocol is still `TrafficMessage` (`CHARGE_REQUEST`, `CHARGE_DECISION`).

### `uav_msgs/msg/WeatherStatus`

From weather node:

- `regime` (string): **five regimes are implemented**: `sunny`, `cloudy`, `windy`, `rainy`, `stormy`.  
- Numeric fields: `temperature_c`, `wind_speed`, `rain_intensity`, `wind_direction_deg`.

### `uav_msgs/msg/FailureEvent`

Emitted by UAVs when a failure occurs (battery death is explicitly used by recovery):

- `uav_id`, `role`, `failure_type`, `description`, `stamp`

### `uav_msgs/msg/UavDeployment`

Used as a planner-to-sink/viz artifact; deployment control is usually sent via FANET `TrafficMessage`:

- `uav_id`, `role`, `cluster_id`, `ch_id`, `target_pose`
- `next_hop_to_sink`, `next_hop_to_ugv` (note: UAV node comments indicate centralized routing ignores these hints)

### IDs and addressing rules

There are **three kinds of identifiers** you must keep consistent:

1. **Network IDs** (strings): `UavStatus.uav_id` and `TrafficMessage.src_id/dst_id/next_hop_id`. This is what routing uses.
2. **ROS node graph names**: launch file often appends `_{run_id}` to avoid collisions. This does *not* affect routing unless the node’s `uav_id` parameter changes.
3. **Cluster IDs**: e.g., `cluster_1`, `cluster_2`.

Practical rule: treat the run YAML as the single source of truth for ID strings, and ensure the same strings are used consistently in all node parameters.

## Subsystem Deep Dives

### Charging Subsystem Deep Dive

#### What exists in code today

Charging spans:

- UAV charging state machine and request/decision parsing in `src/uav_fleet/src/uav_node.cpp`
- UGV scheduling and dock management in `src/ugv_charger/src/ugv_charger_node.cpp`
- Charging messages in `uav_msgs` (`ChargeRequest`, `ChargeDecision`, and FANET `TrafficMessage` control opcodes)

#### UAV charging state machine

UAV charging state is exported via `UavStatus.charging_state`:

- `0 ACTIVE`
- `1 GOING_TO_UGV`
- `2 CHARGING`
- `3 RETURNING`

Core transition drivers (must be verified directly in `uav_node.cpp` for exact thresholds/guards):

- ACTIVE → GOING_TO_UGV: when battery threshold logic triggers and UAV receives an accepted `CHARGE_DECISION`.
- GOING_TO_UGV → CHARGING: when UAV reaches UGV docking region and is allowed to start charging (slot assignment comes from decision payload).
- CHARGING → RETURNING: when charging ends (target reached or time limit) or when preemptive stop is commanded.
- RETURNING → ACTIVE: when UAV returns to the operational region and resets mission state.

Preemption is modeled by a decision message that indicates a stop action (e.g., `reason=PREEMPTED`, `target_action=STOP_CHARGING`), forcing CHARGING → RETURNING.

#### UGV slot/dock management

UGV charger maintains:

- A queue of incoming requests.
- A set of concurrently charging UAVs, bounded by `ugv.max_parallel_spots` (or similarly named parameter).

Exact queue data structures and scheduling frequency are defined in `ugv_charger_node.cpp` and must be used as truth when extending policy logic.

#### Protocols and where selection happens

Run configurations select the policy by YAML:

- `ugv.charging_policy: fcfs|edf|role_priority|dynamic|...`
- preemptive scenarios use `p_*` strings (e.g., `p_edf`, `p_role_priority`, `p_dynamic_score`) in the shipped run YAMLs.

**Where protocol selection happens:**

- YAML: `src/system_bringup/config/runs/*.yaml`
- Launch: `src/system_bringup/launch/experiment.launch.py` passes `charging_policy` into UGV node parameters.
- UGV code: `src/ugv_charger/src/ugv_charger_node.cpp` should contain the switch/dispatch for the policy string.

#### Charging metrics

Charging metrics are intended to be logged by `network_monitor_node` and postprocessed by `tools/charging_protocol_compare.py`. Treat the monitor node implementation as authoritative for exact event definitions.

#### Relevant files

- `src/uav_fleet/src/uav_node.cpp`
- `src/ugv_charger/src/ugv_charger_node.cpp`
- `src/system_bringup/config/runs/ugv_*.yaml`
- `tools/charging_protocol_compare.py`

#### Key parameters (YAML / ROS params)

- Policy selection: `ugv.charging_policy`
- Dock capacity: `ugv.max_parallel_spots`
- Preemption knobs: under `ugv.*` in `ugv_p_*.yaml` configs
- UAV charging trigger: `uav.battery_threshold`, plus retry/timeout knobs (see `example_run.yaml` and `uav_node.cpp`)

#### Key topics

- FANET: `/fanet/network_bus_raw`, `/fanet/network_bus`, `/fanet/delivered` (for decisions/ACK)
- Mirror/orchestration: `/uav_fleet/charge_requests`
- UGV publishes its status on `/fanet/status` (required for routing and planner discovery)

### Routing & FANET Deep Dive

#### Neighbor discovery assumptions

Reachability is modeled via periodic `/fanet/status` (`UavStatus`) beacons:

- Nodes keep a neighbor table of recently seen statuses.
- A neighbor is considered reachable if the beacon is fresh (timeout-based) and within range (`comm_radius_m` distance check).

Routing manager separately filters nodes by status freshness (`status_timeout_sec`).

#### Routing table creation and expiry

`routing_manager_node`:

- Reads all `/fanet/status` beacons.
- Treats `role==1` nodes as backbone CHs; all others are endpoints.
- Builds a CH-only graph with a distance threshold `comm_range_m`.
- Applies hysteresis (`hysteresis_margin_m`) to reduce link flapping.
- Assigns each endpoint to a gateway CH (nearest CH within range).
- Runs Dijkstra shortest paths across the CH graph.
- Publishes `RoutingTable` for each node to `/fanet/routing_table`.

Routes expire implicitly when nodes stop publishing status or move out of range; recomputation is periodic (`recompute_period_sec`) and also triggered by `/fanet/routing_event`.

#### Forwarding rules

Forwarding semantics are implemented primarily in UAV code:

- **Members do not forward**.
- **CHs forward**:
  - deliver if `dst_id == my_id`
  - otherwise, look up routing table for `dst_id` and rewrite `next_hop_id`
  - enforce loop prevention using hop history (`recent_hops`) and TTL/hop count.

ACK strategy:

- If a message has `requires_ack=true`, the final destination attempts to send an `ACK` control message back to `src_id`, using the routing table to find a next hop.

Unclear/needs confirmation:

- Some control-plane “observer nodes” (planner/recovery/monitor) subscribe to `/fanet/network_bus` and may read messages not addressed to them. This is intentional for logging/coordination but is not equivalent to FANET-delivered reception.

#### Relevant files

- `src/routing_manager/src/routing_manager_node.cpp`
- `src/uav_fleet/src/uav_node.cpp`
- `src/fault_injector/src/fault_injector_node.cpp`

#### Key parameters (YAML / ROS params)

- Routing manager:
  - `routing_manager.comm_range_m`
  - `routing_manager.hysteresis_margin_m`
  - `routing_manager.recompute_period_sec`
  - `routing_manager.status_timeout_sec`
  - `routing_manager.ch_move_threshold_m`
  - `routing_manager.sink_id`, `routing_manager.ugv_id`
- UAV neighbor retention:
  - `uav.hello_timeout_sec` and related neighbor timers in `uav_node.cpp`
- Fault injection:
  - `fault_injector.enabled`, `fault_injector.p0`, `fault_injector.p_max`, multipliers

#### Key topics

- `/fanet/status`
- `/fanet/routing_table`
- `/fanet/network_bus_raw`, `/fanet/network_bus`, `/fanet/delivered`
- `/fanet/routing_event`
- `/routing_manager/alerts`

### Weather Model

#### Weather states

`weather_node` implements **five regimes**:

- `sunny`, `cloudy`, `windy`, `rainy`, `stormy`

The regime is stored as a string in `WeatherStatus.regime`.

#### Markov transition model and controls

Weather node parameters (authoritative in `src/weather_server/src/weather_node.cpp`):

- `mode`: `"markov"` or `"fixed"`
- `start_state`: one of the five regimes
- Publish/update frequency: `update_period_sec` (alias: `publish_period_sec`)
- Regime transition timer: `macrostate_period_sec` (alias: `transition_period_sec`)
- Seeding: `seed` (`-1` uses random_device)
- Transition matrix: enabled with `transition_matrix_from_yaml=true`, then each entry is read from parameters like:  
  `transition_matrix.sunny.cloudy`, `transition_matrix.rainy.stormy`, etc. (rows are normalized automatically)

#### How weather affects battery and network

Implemented effects:

- **Network**: fault injector reads `/environment/weather` and computes drop probability from wind/rain/temp deviation; it may drop traffic between raw and processed bus.
- **Battery**: UAV nodes consume `/environment/weather` and adjust their energy consumption rate based on wind, rain, and temperature factors.

#### Relevant files

- `src/weather_server/src/weather_node.cpp`
- `src/fault_injector/src/fault_injector_node.cpp`
- `src/uav_fleet/src/uav_node.cpp`

#### Key parameters (YAML / ROS params)

- Weather:
  - `weather.mode`, `weather.start_state`
  - `weather.publish_period_sec` (alias), `weather.macrostate_period_sec`
  - optional transition matrix keys
- Fault injection:
  - `fault_injector.p0`, `fault_injector.aw/ar/at`, `fault_injector.p_max`
  - `fault_injector.drop_control_multiplier`, `fault_injector.drop_data_multiplier`

#### Key topics

- `/environment/weather`
- `/fanet/network_bus_raw`, `/fanet/network_bus`

### Coverage Planning & Sink/Gateway

#### What coverage planner produces

`coverage_planner_node` produces:

- `/coverage_planner/task_points` (`TaskPointArray`, transient-local)
- FANET deployment control messages injected into `/fanet/network_bus_raw`:
  - `control_type="DEPLOYMENT_CMD"` for deployments
  - `control_type="MOTION_START"` for motion barrier

It can also publish `/coverage_planner/deployment` (`UavDeployment`) if `accept_direct_deployment=true`, but both planner and UAV default to `accept_direct_deployment=false`, meaning deployments are expected via FANET control.

Taskpoint generation modes (`taskpoint_generation_mode`):

- `random`
- `fixed_file` (loads YAML list under `taskpoints:`; default points to `system_bringup/config/taskpoints/fixed_taskpoints.yaml`)
- `fixed_dispersed` (deterministic grid-like dispersion controlled by `fixed_taskpoints_count` and `fixed_taskpoints_seed`)
- Or override directly via `task_points` parameter list of `id:x:y` strings

#### How sink/gateway relays commands into FANET

`sink_gateway_node` subscribes to `/coverage_planner/deployment` and *re-encodes* those deployments into FANET control messages published to `/fanet/network_bus_raw`:

- `control_type="DEPLOYMENT"`
- requires ACK and maintains a resend barrier until all expected UAVs ACK (either via `DEPLOYMENT_ACK` or generic `ACK` referencing `DEP_*` msg IDs)
- after barrier, sends `MOTION_START` (requires ACK and can be resent)

Important behavior to confirm in your scenario:

- If `coverage_planner` does not publish `/coverage_planner/deployment` (because `accept_direct_deployment=false`), then sink’s deployment conversion path is mostly dormant, and deployment commands primarily originate from the planner’s own FANET injection (`DEPLOYMENT_CMD`).

This dual-source behavior is **present in code** and can be confusing. If you need a single authoritative initializer, document which one you rely on for your experiments and disable the other via configuration if possible (currently a doc-level recommendation; no refactor performed).

#### How UAV telemetry returns to sink

- Members generate telemetry (DATA `TrafficMessage`) destined for `sink_gateway` and first-hop it to their CH.
- CHs forward along backbone using routing manager tables.
- Sink gateway publishes a delivery tap to `/fanet/delivered`.

Centralized vs distributed:

- Centralized: coverage planning and routing.
- Distributed: mobility, forwarding, charging state machines, ACK behavior.

#### Relevant files

- `src/coverage_planner/src/coverage_planner_node.cpp`
- `src/sink_gateway/src/sink_gateway_node.cpp`
- `src/uav_fleet/src/uav_node.cpp`

#### Key parameters (YAML / ROS params)

- Planner:
  - `coverage_planner.uav_ids`, `coverage_planner.num_ch`
  - `coverage_planner.taskpoint_generation_mode`, `coverage_planner.fixed_taskpoints_file`
  - `coverage_planner.rng_seed`, `coverage_planner.fixed_taskpoints_seed`
- Sink gateway:
  - `sink.sink_id`, `sink.uplink_ch_id`
  - resend knobs: `deployment_resend_period_sec`, `deployment_max_resends`, `motion_start_resend_period_sec`, `motion_start_max_resends`

#### Key topics

- `/coverage_planner/task_points`
- `/coverage_planner/deployment`
- `/fanet/network_bus_raw`, `/fanet/network_bus`, `/fanet/delivered`
- `/fanet/status`

### Network Monitor & Data Logging

This section must be treated as **code-defined** by `src/network_monitor/src/network_monitor_node.cpp`. The repository also includes `tools/charging_protocol_compare.py`, which expects specific output files/columns; use that tool as a practical check that monitor outputs match your analysis pipeline.

What is definite from bringup + tools:

- Output directory is structured by `output_dir/run_id` (see bringup launch + postprocessing script expectation).
- Run configs pass `run_id` and `output_dir` into the monitor node.

Unclear/needs confirmation (must verify in `network_monitor_node.cpp`):

- Exact subscribed topics list.
- Exact CSV filenames and schema headers.
- How seeds/protocol metadata are stored (some pipelines rely on `run_id` conventions rather than explicit metadata).

If you need to “lock down” reproducibility without refactoring code, add a doc-required step: copy the run YAML into `output_dir/run_id/` at launch time (manual) or implement a minimal metadata emission patch later.

#### Relevant files

- `src/network_monitor/src/network_monitor_node.cpp`
- `tools/charging_protocol_compare.py`

#### Key parameters (YAML / ROS params)

- `monitor.run_id`, `monitor.output_dir`
- Data flush periods and timeouts under `monitor.*` in run configs
- Termination-related parameters (if monitor supports shutdown triggers): look for `monitor.*shutdown*` params in `network_monitor_node.cpp`

#### Key topics

- Must be confirmed in `network_monitor_node.cpp`. At minimum, for sane metrics it should observe:
  - `/fanet/network_bus_raw`, `/fanet/network_bus`, `/fanet/delivered`
  - `/fanet/status`, `/fanet/routing_table`
  - charging mirrors (at least `/uav_fleet/charge_requests`)

### Recovery Logic & Failure Events

Recovery is implemented as a centralized controller (`recovery_manager_node`) that injects FANET control messages.

#### What triggers recovery

- CH status timeout (`/fanet/status` freshness)
- CH heartbeat timeout (`HEARTBEAT` control observed on `/fanet/network_bus`)
- Explicit `FailureEvent` (`failure_type==BATTERY_DEAD`)
- Sink/UGV unreachable alerts from routing manager (`/routing_manager/alerts`)

#### Recovery actions injected as FANET control

- `RECOVERY_START` and `RECOVERY_DONE` broadcasts (`dst_id="broadcast"`)
- `CLUSTER_REASSIGN` to members (first hop set to CH)
- `TASK_ASSIGN` to members (first hop to CH if known)
- `NEW_DEPLOYMENT` to CHs (direct next hop)
- `MEMBER_FALLBACK` to members when no CHs remain (payload is target pose)

ACK retry:

- Recovery manager maintains a `pending_acks_` map and retransmits until acked or a retry limit is reached.
- To ensure ACKs are routable, recovery manager uses `control_src_id` parameter (defaulting to sink ID), so ACKs target a routable identity. Recovery manager observes ACKs on the bus to clear pending retransmits.

Known gap (document-level, not a code change):

- Recovery logic currently treats “alive” as `battery_level > 0` + fresh beacons. It does not interpret `charging_state` or `intent_to_leave` as temporary unavailability. This can cause “available backbone” vs “alive” ambiguity. The repo includes an explicit audit with suggested improvements in `docs/recovery_cluster_rebuild_audit.md`.

#### Relevant files

- `src/recovery_manager/src/recovery_manager_node.cpp`
- `docs/recovery_cluster_rebuild_audit.md`
- `src/uav_fleet/src/uav_node.cpp`
- `src/ch_manager/src/ch_manager_node.cpp`
- `src/routing_manager/src/routing_manager_node.cpp`

#### Key parameters (YAML / ROS params)

- `recovery.comm_range_m`
- `recovery.status_timeout_sec`
- `recovery.heartbeat_timeout_sec`
- `recovery.recovery_cooldown_sec`
- `recovery.control_ttl`
- `recovery.ack_retry_period_sec`, `recovery.max_ack_retries`
- `recovery.recovery_lock_duration_sec`

#### Key topics

- `/fanet/status`
- `/fanet/network_bus` (observes `HEARTBEAT` and `ACK`)
- `/fanet/network_bus_raw` (injects recovery controls)
- `/routing_manager/alerts`
- `/uav_fleet/failure_events`
- `/coverage_planner/task_points`
- `/ch_manager/cluster_info`

## Running the Simulator

### Build

From repository root:

```bash
colcon build --symlink-install
```

### Source

```bash
source install/setup.bash
```

### Launch a default scenario

Main entrypoint:

```bash
ros2 launch system_bringup experiment.launch.py \
  config:=system_bringup/config/runs/example_run.yaml \
  run_id:=demo_run \
  output_dir:=log
```

Notes:

- `config` is a path within the `system_bringup` package.
- `run_id` is used for naming nodes (as suffix) and for output folder naming.
- Termination is enforced from the launch file by timeout when `termination_mode` is `timeout_only`.

### Run an experiment with a specific run YAML

Example: EDF policy run

```bash
ros2 launch system_bringup experiment.launch.py \
  config:=system_bringup/config/runs/ugv_edf.yaml \
  run_id:=edf_seed0 \
  output_dir:=log
```

### Change protocol selection

Choose or edit a run YAML under:

- `src/system_bringup/config/runs/`

Set:

- `ugv.charging_policy: fcfs|edf|role_priority|dynamic|p_edf|p_role_priority|p_dynamic_score`

### Change runtime timeout

Edit run YAML:

- `global.termination_mode: timeout_only`
- `global.experiment_timeout_s: <seconds>`

The launch file implements timeout shutdown via a ROS 2 launch `Shutdown` event. If you rely on other termination modes, confirm that shutdown is propagated to the full launch graph.

### Change weather mode

In run YAML:

- Fixed mode:
  - `weather.mode: fixed`
  - `weather.start_state: sunny|cloudy|windy|rainy|stormy`
- Markov mode:
  - `weather.mode: markov`
  - `weather.macrostate_period_sec: <seconds>`
  - optional `weather.transition_matrix_from_yaml: true` plus transition matrix entries.

### Enable/disable fault injection

In run YAML (exact key depends on how bringup maps parameters):

- `fault_injector.enabled: true|false`

If you disable fault injection entirely at launch level (bringup may provide a `use_fault_injector` override), confirm that `/fanet/network_bus_raw` is still bridged to `/fanet/network_bus` or that nodes subscribe consistently to the bus you are using.

### Practical debugging commands

List nodes:

```bash
ros2 node list
```

Inspect bus traffic (control opcodes):

```bash
ros2 topic echo /fanet/network_bus | grep -E "control_type:|dst_id:|next_hop_id:"
```

Inspect routing tables:

```bash
ros2 topic echo /fanet/routing_table
ros2 topic echo /routing_manager/alerts
```

Inspect status stream:

```bash
ros2 topic echo /fanet/status
```

Inspect weather:

```bash
ros2 topic echo /environment/weather
```

Inspect charging request mirror:

```bash
ros2 topic echo /uav_fleet/charge_requests
```

## Experiment Methodology

### Ensuring comparable runs

To make results comparable across charging protocols and network conditions:

- Use **timeout-only termination** with identical `experiment_timeout_s`.
- Fix all seeds:
  - global RNG seed (passed via bringup)
  - weather `seed`
  - taskpoint seeds (or use `fixed_file`)
- Keep topology constant:
  - same number of CHs (clusters),
  - same UAV IDs and initial CH/member split,
  - same bounds and comm radius.

A practical workflow:

1. Pick a “baseline” YAML (e.g., `ugv_fcfs.yaml`).
2. Clone it per policy: `ugv_edf.yaml`, `ugv_role_priority.yaml`, etc.
3. Change **only** `ugv.charging_policy` between clones.

### Recommended run matrix

Recommended for protocol evaluation:

- Charging policy × Weather mode × Seed  
  Example:
  - Policy: `fcfs`, `edf`, `role_priority`, `dynamic`, plus `p_*` variants
  - Weather: `fixed` (sunny/cloudy/windy/rainy/stormy) + `markov`
  - Seeds: at least 5 (more if variance is high)

### Expected artifacts and plots

Your analysis pipeline should produce:

- Charging performance:
  - Waiting time distributions per role and per policy
  - Preemption rates and victim/winner statistics (for `p_*`)
  - Battery-dead counts / time-to-first-failure
- Network metrics:
  - Delivery ratio (PDR) per `{flow_type, control_type}`
  - Delay and jitter distributions
  - Drop reasons frequency (weather drop vs route drop vs TTL, etc.)

Use the provided tool (if it matches your monitor outputs):

```bash
python3 tools/charging_protocol_compare.py --help
```

If the tool fails due to missing columns/files, reconcile the monitor schema with the tool expectations (doc-first approach: document the monitor schema as the contract).

## Troubleshooting

### “Executable not found” during launch

Cause: bringup launch supports an `executables:` mapping in your run YAML; missing/incorrect entries can cause the launch to try legacy package/executable names.

Check your run YAML contains correct `executables` mapping, as in `example_run.yaml`.

### No traffic reaches sink

Symptoms:

- `/fanet/delivered` mostly empty for `dst_id == sink_gateway`.

Diagnose:

```bash
ros2 topic echo /routing_manager/alerts
ros2 topic echo /fanet/routing_table
ros2 topic echo /fanet/network_bus | grep -E "dst_id: sink_gateway|control_type: DROP"
```

Common causes:

- No CHs are active (`role=1` missing).
- Routing manager is not running or not publishing routes.
- ID mismatch: sink is named differently in YAML vs node parameters.

### Charging requests time out / decisions missing

Diagnose:

```bash
ros2 topic echo /fanet/network_bus | grep -E "CHARGE_REQUEST|CHARGE_DECISION|DROP|ACK"
ros2 topic echo /uav_fleet/charge_requests
ros2 topic echo /routing_manager/alerts
```

Common causes:

- UGV unreachable (see routing alerts).
- Fault injector dropping too aggressively (reduce `fault_injector.p0` or multipliers).
- ID mismatch on `ugv_id`.

### Recovery triggers unexpectedly or thrashes

Diagnose:

```bash
ros2 topic echo /fanet/network_bus | grep -E "RECOVERY_START|RECOVERY_DONE|CLUSTER_REASSIGN|TASK_ASSIGN|NEW_DEPLOYMENT|MEMBER_FALLBACK"
ros2 topic echo /routing_manager/alerts
```

Common causes:

- Timeouts too tight relative to publish rates.
- Routing reachability alerts flapping due to movement and recompute cycles.
- Control-plane conflicts between static cluster info and recovery reassign commands (see `docs/recovery_cluster_rebuild_audit.md` for analysis).

## Development Notes

### Adding a new charging protocol cleanly

Where to implement:

- `src/ugv_charger/src/ugv_charger_node.cpp`

Recommended contract:

- Policy is selected by a **string** parameter (run YAML sets `ugv.charging_policy`).
- The decision should be sent back as FANET control `CHARGE_DECISION`.
- If you embed rationale in `TrafficMessage.payload`, prefer a stable format such as:
  - `key=value;key=value;...`
so monitors/tools can parse it without ambiguity.

Also update:

- Add a new run YAML under `src/system_bringup/config/runs/` that sets `ugv.charging_policy` to your new string.
- Document the new policy string and its semantics in README (this section).

### Adding a new monitored signal without breaking schema

Where to implement:

- `src/network_monitor/src/network_monitor_node.cpp`

Schema rules for stable analysis:

- Never rename existing columns without providing backward compatibility.
- Prefer adding new columns with empty/default values for older events.
- Keep `run_id` (and ideally `msg_id`) as join keys.
- If you add a new CSV, document:
  - filename,
  - header line,
  - per-row semantics,
  - which topic(s) populate it.
