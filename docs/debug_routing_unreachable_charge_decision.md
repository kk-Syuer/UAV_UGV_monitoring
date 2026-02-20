# Debug report: `UNREACHABLE_CHARGE_DECISION_NEXT_HOP` for CHARGE_DECISION

## Where the UNREACHABLE log comes from

The observed payload entries like:

```text
...,UNREACHABLE_CHARGE_DECISION_NEXT_HOP
```

are produced by **UGV charger node** (`ugv_charger_node.cpp`) in:

- `sendDecisionControlMessage(...)` when sending routed `CHARGE_DECISION`
- `ensureReachableOrDrop(...)` when next hop is empty/unreachable
- `publishDrop(...)` which emits a `DROP` control message containing `<msg_id>,<drop_reason>`

So this is **not** a direct-topic failure; it is a FANET routing reachability failure on `/fanet/network_bus_raw`.

---

## Root-cause pattern found

The failure occurs when UGV chooses a decision next hop from routing table (`resolveNextHop(dst)`), but that next hop cannot be validated as reachable by `neighborReachable(next_hop)`.

`neighborReachable` requires all of:

1. next hop exists in `uav_status_`
2. status not stale (`neighbor_timeout_sec`)
3. geometric range check passes (`dist <= min(ugv_comm_radius, neighbor_comm_radius)`)
4. neighbor not currently charging and not leaving soon

If any check fails, `ensureReachableOrDrop(...)` drops with `UNREACHABLE_CHARGE_DECISION_NEXT_HOP`.

A frequent practical cause is **ID inconsistency** (e.g., route references `ch0` while statuses/routing peers use `uav_3`), which makes next hop effectively missing/unreachable.

---

## Instrumentation/fixes added

### 1) Structured routing trace at failure point

Added `ROUTE_TRACE event=UNREACHABLE` with fields:

- `control_type`
- `src_id`, `dst_id`, `current_node_id`
- `candidate_next_hop`
- `routing_table_size`, `neighbor_table_size`
- `dst_last_seen_age_ms`, `next_hop_last_seen_age_ms`
- `reason_code`

Reason codes now include:

- `DST_UNKNOWN`
- `NO_ROUTE`
- `NEXT_HOP_EXPIRED`
- `GRAPH_DISCONNECTED`
- `ID_NOT_FOUND`
- `NEXT_HOP_ID_NOT_FOUND`

### 2) ID consistency checks

Added warnings when:

- routing-table next-hop IDs are absent from status-known neighbors (`ROUTING_TABLE_INCONSISTENT`)
- destination UAV is unknown in status map (`DST_UNKNOWN`)
- destination exists but route is absent (`GRAPH_DISCONNECTED`)

### 3) Neighbor lifecycle logs

Added optional neighbor trace logs:

- `ROUTE_TRACE event=NEIGHBOR_ADDED`
- `ROUTE_TRACE event=NEIGHBOR_UPDATED`
- `ROUTE_TRACE event=NEIGHBOR_EXPIRED`

### 4) Control-drop policy logs

Added optional logs when `CHARGE_REQUEST` is filtered by policy:

- `reason_code=BATTERY_DEAD`
- `reason_code=NO_STATUS`
- `reason_code=BATTERY_ABOVE_GATE`

---

## Runtime knobs

Enable high-detail routing traces:

```bash
ros2 param set /ugv_charger routing_debug_trace true
ros2 param set /ugv_charger neighbor_debug_trace true
```

---


## Transport path compliance

Charging request/decision control path remains FANET-routed (`/fanet/network_bus_raw` -> `/fanet/network_bus`) using `TrafficMessage` (`CHARGE_REQUEST`, `CHARGE_DECISION`).
Any mirrored `/ugv/charge_decisions` publication is compatibility telemetry only and is not used as the control delivery path.

---

## Reproduction & verification commands

```bash
# Build
cd ~/ros2_ws
colcon build --symlink-install
source install/setup.bash

# Bringup
ros2 launch system_bringup bringup.launch.py run:=<YOUR_RUN_NAME>

# Inspect routed control traffic
ros2 topic echo /fanet/network_bus_raw
ros2 topic echo /fanet/network_bus

# Focus on charge decisions and drops
ros2 topic echo /fanet/network_bus --once
ros2 topic echo /fanet/network_bus | grep -E "CHARGE_DECISION|UNREACHABLE_CHARGE_DECISION_NEXT_HOP|ROUTE_TRACE"

# Verify node IDs present in graph
ros2 node list | grep -i -E "ch|ugv|uav"

# Verify status stream includes the expected next-hop IDs
ros2 topic echo /fanet/status

# Optional GUI logs
ros2 run rqt_console rqt_console
```

---

## Evidence to expect after patch

When decision fails to route, you should now see explicit context such as:

```text
ROUTE_TRACE event=UNREACHABLE control_type=CHARGE_DECISION src_id=ugv_1 dst_id=uav_3 current_node_id=ugv_1 candidate_next_hop=ch0 routing_table_size=... neighbor_table_size=... dst_last_seen_age_ms=... next_hop_last_seen_age_ms=-1 reason_code=ID_NOT_FOUND
```

This pinpoints whether the issue is:

- missing destination identity
- missing route
- stale next hop
- route/status ID mismatch

