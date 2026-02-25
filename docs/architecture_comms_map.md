# ROS 2 Communication Architecture Map

This document is a static-analysis communication map for the ROS 2 workspace under `src/`, including package executables, launch wiring (`system_bringup/launch/experiment.launch.py`), and run-config switches (`system_bringup/config/runs/*.yaml`).

## Generated artifacts

- Interface extraction JSON: `artifacts/ros_interfaces.json`
- Graphviz DOT graph: `artifacts/ros_graph.dot`
- Graph renders: `artifacts/ros_graph.svg` and `artifacts/ros_graph.png` (generated only if `dot` is available in environment)

> In this environment, Graphviz `dot` was not installed, so only DOT was produced.

## Node inventory

| Node (runtime pattern) | Package | Executable | Key params / IDs (observed) | Timers / periodic behavior |
|---|---|---|---|---|
| `network_monitor_<run_id>` | `network_monitor` | `network_monitor_node` | `run_id`, `output_dir`, `ugv_dock_capacity` | CSV flush + charge timeout + status/weather/queue/network timeseries + stats publish timers |
| `weather_<run_id>` (optional) | `weather_server` | `weather_node` | `mode`, `start_state`, `seed`, weather coefficients | periodic weather publish + macrostate transition timer |
| `fault_injector_<run_id>` (optional) | `fault_injector` | `fault_injector_node` | `drop_mode`, `p0`, `aw`, `ar`, `at`, `p_max`, `drop_control_multiplier`, `drop_data_multiplier` | passive forwarder/dropper (no periodic publishers) |
| `sink_gateway_<run_id>` | `sink_gateway` | `sink_gateway_node` | `sink_id`, `ugv_id`, `uplink_ch_id`, `comm_radius_m` | status, control, CH-timeout, deployment resend, motion-start resend timers |
| `ugv_charger_<run_id>` | `ugv_charger` | `ugv_charger_node` | `ugv_id`, `charging_policy`, `max_parallel_spots`, charger/battery params | scheduler + status + ack retry + charging snapshot + neighbor timeout (+ optional mobility) |
| `user_device_<run_id>` (optional) | `user_devices_sim` | `user_device_node` | `user_id`, `cluster_id`, `ch_id` | periodic traffic generation timer |
| `<uav_id>_<run_id>` (many instances) | `uav_fleet` | `uav_node` | `uav_id`, `role`, `my_ch_id`, `ugv_id` + battery and telemetry params | status/heartbeat/traffic/neighbor/buffer retry + CH status + charge request + ACK retry (+ optional mobility) |
| `routing_manager_<run_id>` | `routing_manager` | `routing_manager_node` | `sink_id`, `ugv_id`, `comm_range_m`, `recompute_period_sec`, `status_timeout_sec` | periodic route recompute timer |
| `recovery_manager_<run_id>` (optional) | `recovery_manager` | `recovery_manager_node` | `sink_id`, `ugv_id`, `heartbeat_timeout_sec`, `recovery_cooldown_sec` | watchdog/retry style timers for recovery injection |
| `coverage_planner_<run_id>` (optional) | `coverage_planner` | `coverage_planner_node` | `uav_ids`, `num_ch`, `ugv_id`, `taskpoint_generation_mode`, bounds, fixed_taskpoints settings | deployment/ack retry timers |
| `ch_manager_<cluster_id>` (optional, one per cluster) | `ch_manager` | `ch_manager_node` | `cluster_id`, `ch_id`, `member_ids` | periodic cluster info publish timer |
| `fleet_viz_<run_id>` (optional) | `planner_viz` | `fleet_viz` | visualization tuning parameters | subscriber-driven UI updates |
| (not launched by experiment) `planner_viz_node` | `planner_viz` | `planner_viz_node` | map bounds/service radius params | subscriber-driven UI updates |

### Launch/runtime naming & executable mapping notes

- `experiment.launch.py` composes most package/executable pairs via `executables.*` YAML overrides; defaults point to package-local executables (e.g., `routing_manager_node`, `uav_node`, etc.).
- Optional subsystems are gated by run config flags: `weather.enable`, `fault_injector.enable`, `traffic.user_device_enable`, `recovery_manager.enable`, `coverage_planner.enable`, `ch_manager.enable`, `planner_viz.enable`.
- UAV and CH manager are multi-instance nodes with generated names based on IDs from the run YAML.

## Topic inventory

| Topic | Type | Publishers | Subscribers | Notes |
|---|---|---|---|---|
| `/fanet/network_bus_raw` | `uav_msgs::msg::TrafficMessage` | `uav_fleet/uav_node`, `sink_gateway/sink_gateway_node`, `ugv_charger/ugv_charger_node`, `coverage_planner/coverage_planner_node`, `user_devices_sim/user_device_node`, `recovery_manager/recovery_manager_node` | `fault_injector/fault_injector_node`, `network_monitor/network_monitor_node`, `planner_viz/fleet_viz` | Data-plane ingress bus (pre-fault injection) |
| `/fanet/network_bus` | `uav_msgs::msg::TrafficMessage` | `fault_injector/fault_injector_node` | `uav_fleet/uav_node`, `sink_gateway/sink_gateway_node`, `ugv_charger/ugv_charger_node`, `coverage_planner/coverage_planner_node`, `user_devices_sim/user_device_node`, `recovery_manager/recovery_manager_node`, `ch_manager/ch_manager_node`, `network_monitor/network_monitor_node`, `planner_viz/fleet_viz` | Data-plane bus after optional injector |
| `/fanet/delivered` | `uav_msgs::msg::TrafficMessage` | `fault_injector/fault_injector_node`, `uav_fleet/uav_node`, `sink_gateway/sink_gateway_node`, `ugv_charger/ugv_charger_node`, `user_devices_sim/user_device_node` | `network_monitor/network_monitor_node`, `planner_viz/fleet_viz` | Delivery tap |
| `/fanet/delivered_raw` | `uav_msgs::msg::TrafficMessage` | `user_devices_sim/user_device_node` | `fault_injector/fault_injector_node` | Raw delivered side-channel into injector |
| `/fanet/status` | `uav_msgs::msg::UavStatus` | `uav_fleet/uav_node`, `sink_gateway/sink_gateway_node`, `ugv_charger/ugv_charger_node` | `routing_manager/routing_manager_node`, `recovery_manager/recovery_manager_node`, `coverage_planner/coverage_planner_node`, `network_monitor/network_monitor_node`, `planner_viz/fleet_viz`, `uav_fleet/uav_node`, `ugv_charger/ugv_charger_node` | Routing control-plane input + fleet beacons |
| `/fanet/routing_table` | `uav_msgs::msg::RoutingTable` | `routing_manager/routing_manager_node` | `uav_fleet/uav_node`, `sink_gateway/sink_gateway_node`, `ugv_charger/ugv_charger_node`, `network_monitor/network_monitor_node` | Routing manager output |
| `/routing_manager/alerts` | `std_msgs::msg::String` | `routing_manager/routing_manager_node` | `recovery_manager/recovery_manager_node`, `planner_viz/fleet_viz` | Alert feed into recovery |
| `/fanet/routing_event` | `std_msgs::msg::String` | `uav_fleet/uav_node`, `sink_gateway/sink_gateway_node`, `ugv_charger/ugv_charger_node` | `routing_manager/routing_manager_node` | Routing-event feedback/control |
| `/uav_fleet/charge_requests` | `uav_msgs::msg::ChargeRequest` | `uav_fleet/uav_node` | `network_monitor/network_monitor_node` | Explicit metrics tap; charging control also on TrafficMessage |
| `/ugv/charge_decisions` | `uav_msgs::msg::ChargeDecision` | `ugv_charger/ugv_charger_node` | `network_monitor/network_monitor_node` | Explicit metrics tap; charging control also on TrafficMessage |
| `/uav_fleet/failure_events` | `uav_msgs::msg::FailureEvent` | `uav_fleet/uav_node` | `recovery_manager/recovery_manager_node`, `ch_manager/ch_manager_node`, `ugv_charger/ugv_charger_node`, `planner_viz/fleet_viz` | Failure detection/control trigger |
| `/coverage_planner/deployment` | `uav_msgs::msg::UavDeployment` | `coverage_planner/coverage_planner_node` | `uav_fleet/uav_node`, `sink_gateway/sink_gateway_node`, `ugv_charger/ugv_charger_node`, `planner_viz/fleet_viz` | Deployment commands |
| `/coverage_planner/task_points` | `uav_msgs::msg::TaskPointArray` | `coverage_planner/coverage_planner_node` | `uav_fleet/uav_node`, `planner_viz/fleet_viz` | Coverage targets |
| `/ch_manager/cluster_info` | `uav_msgs::msg::ClusterInfo` | `ch_manager/ch_manager_node` | `uav_fleet/uav_node`, `planner_viz/fleet_viz` | Cluster membership/state publication |
| `/environment/weather` | `uav_msgs::msg::WeatherStatus` (plus alias `WeatherStatus` in injector code) | `weather_server/weather_node` | `fault_injector/fault_injector_node`, `uav_fleet/uav_node`, `network_monitor/network_monitor_node`, `planner_viz/fleet_viz` | Weather feeds drop model + UAV energy behavior |
| `/ugv/queue_events` | `std_msgs::msg::String` | `ugv_charger/ugv_charger_node` | `network_monitor/network_monitor_node` | Charging queue telemetry |
| `/ugv/charging_snapshot` | `std_msgs::msg::String` | `ugv_charger/ugv_charger_node` | `network_monitor/network_monitor_node`, `planner_viz/fleet_viz` | Charging-state snapshot |
| `/network_monitor/stats` | `std_msgs::msg::String` | `network_monitor/network_monitor_node` | `planner_viz/fleet_viz` | Dashboard stats feed |

## Service inventory

| Service name | Type | Servers | Clients | Notes |
|---|---|---|---|---|
| `/uav_fleet/" + uav_id_ + "/send_debug_text` | `uav_msgs::srv::SendDebugText` | `uav_fleet/uav_node` | *(none detected)* | Dynamic per-UAV service name pattern; static scanner captured expression literal |

## Action inventory

No action servers or action clients were detected in node source code (action definitions exist in `uav_msgs`, but runtime usage was not found in current executables).

## Key pipelines

### A) FANET data plane

1. Multiple producers publish `uav_msgs/msg/TrafficMessage` onto `/fanet/network_bus_raw`.
2. `fault_injector_node` subscribes `/fanet/network_bus_raw` and republishes (with optional drop/corruption behavior) to `/fanet/network_bus`.
3. Consumer nodes subscribe `/fanet/network_bus` for application-layer FANET forwarding.
4. Delivered traffic appears on `/fanet/delivered` (published by both endpoint logic and injector depending on mode), with `network_monitor`/viz observing it.

### B) Routing control plane

1. UAVs, UGV charger, and sink publish `/fanet/status` beacons.
2. `routing_manager_node` consumes `/fanet/status` (+ `/fanet/routing_event`) and periodically computes routes.
3. It publishes `/fanet/routing_table` and `/routing_manager/alerts`.
4. Routes feed UAV/sink/UGV behavior; alerts feed `recovery_manager_node`.

### C) Charging control path

- Primary control packets (e.g., `CHARGE_REQUEST`, `CHARGE_DECISION`) are carried in `TrafficMessage` over FANET bus topics.
- Additional explicit observability topics exist: `/uav_fleet/charge_requests` and `/ugv/charge_decisions` (consumed by `network_monitor`).

### D) Weather influence path

- `weather_node` publishes `/environment/weather`.
- `fault_injector_node` consumes weather to modulate drop probability.
- `uav_node` consumes weather for onboard battery/energy behavior (and monitor/viz also consume).

### E) Coverage path

- `coverage_planner_node` publishes `/coverage_planner/task_points` and `/coverage_planner/deployment`.
- UAVs consume both; sink/UGV consume deployment updates.
- Planner deployment control retries are timer-driven.

### F) Recovery path

- Inputs: `/routing_manager/alerts`, `/fanet/status`, `/uav_fleet/failure_events`, and heartbeat-like traffic from `/fanet/network_bus`.
- `recovery_manager_node` emits recovery control injections to `/fanet/network_bus_raw` as `TrafficMessage`.

## Graph structure

- Graph file: `artifacts/ros_graph.dot`.
- It uses publisher → topic → subscriber directionality.
- Nodes are boxed and grouped by subsystem/package clusters.
- Topics are ellipse nodes with type labels.
- `/fanet/network_bus_raw`, `/fanet/network_bus`, and `/fanet/delivered` edges are emphasized with bold styling.
- Layout tuning uses orthogonal splines, topic backbone rank constraints, and edge concentration to produce cleaner circuit-like routing with fewer overlaps.

## Runtime validation commands (recommended)

```bash
colcon build --symlink-install
source install/setup.bash
ros2 launch system_bringup experiment.launch.py config:=system_bringup/config/runs/example_run.yaml run_id:=mapcheck output_dir:=log
```

In another terminal (after sourcing):

```bash
ros2 node list
ros2 topic list
ros2 service list
ros2 action list
ros2 node info /<node_name>
ros2 topic info <topic> -v
```

### Runtime naming pattern note

Most launched nodes include a run suffix: `<base_name>_<run_id>`. Multi-instance node names include IDs (e.g., `<uav_id>_<run_id>`, `ch_manager_<cluster_id>`).
