# ClusterHead charging stall debug report

## Scope and symptom

Observed runtime symptom: Cluster Head (CH) UAV can trigger and send `CHARGE_REQUEST`, UGV emits accepted decision, but CH remains at original pose and does not enter `TO_UGV` movement. Members do not show this stall in the same scenario.

---

## Charging pipeline: CH vs Member path comparison

### 1) Trigger conditions (`publishStatus` -> `requestCharge`)

Both roles share the same trigger function (`publishStatus`) and threshold logic:

- Compute estimated energy-to-UGV using current UGV pose if available.
- Compute emergency and request thresholds.
- If battery is below emergency threshold (or member cannot reach CH while request is needed), invoke emergency return (`startEmergencyReturnToUgv`).
- Else request charging via `requestCharge` when below request threshold and no pending request.

Key notes:

- CH uses CH drain/capacity parameters; member uses member values.
- Member has CH reachability guard for normal request send (`my_ch_id_` must be present in neighbors).
- `intent_to_leave` also reflects waiting/slot/to_ugv states.

### 2) Request path (`requestCharge` -> `sendChargeRequest`)

Shared behavior:

- Set `waiting_for_charge_response_ = true`.
- Publish `uav_msgs/ChargeRequest` to `/uav_fleet/charge_requests` for monitoring.
- Send control `TrafficMessage` with `control_type=CHARGE_REQUEST` over `/fanet/network_bus_raw`.

Routing difference:

- Member sends first hop to CH.
- CH sends toward UGV using routing table resolution.

### 3) Decision receive handling

Before fix, actionable decision handling in UAV node was only via routed `TrafficMessage` with `control_type=CHARGE_DECISION` (`handleChargeDecisionFromNetwork`).

After fix, decision intake is unified via two sources:

- routed network control message (`NETWORK_ROUTED`), and
- direct UGV decision topic `/ugv/charge_decisions` (`DIRECT_TOPIC`).

Both sources now call one function (`applyAcceptedChargeDecision`) that applies the exact same state transition and goal setup logic.

### 4) Transition + goal setup

Accepted decision path now performs:

- clear waiting flag
- store slot id
- set `has_charge_slot_ = true`
- set `charge_state_ = TO_UGV`
- set `charge_departure_pose_`
- resolve and set `charge_target_pose_` + validity flag

This was already present for routed decision path, but direct topic path previously had no subscriber/action in `uav_node.cpp`.

### 5) Motion/controller actuation

`mobilityStep()` executes at `mobility_dt_sec` and:

- if `charge_state_ == TO_UGV`, CH/member both prioritize charging movement to `charge_target_pose_`.
- if goal unresolved, function retries pose resolution and holds.
- once arrived, starts charging session.

So CH motion itself is implemented; stalling occurs if accepted decision never transitions CH into `TO_UGV` (or no goal).

---

## Observed code path

1. CH requests charge successfully.
2. UGV accepts and publishes decision.
3. CH movement stall appears when decision is visible on direct `/ugv/charge_decisions` but CH does not receive routed control equivalent (or receives too late/dropped).
4. Without decision handling, CH remains in non-`TO_UGV` state, so no charging motion branch runs.

---

## Where CH vs Member diverges

| Stage | Member behavior | CH behavior | Divergence hypothesis |
|---|---|---|---|
| Request trigger | same threshold pipeline, member CH-reachability check | same threshold pipeline (no CH-reachability check) | Not root cause; requests are observed from CH |
| Decision receive | can receive routed CHARGE_DECISION and transition | may rely on routed path only; if routed decision not delivered, no state transition | **Primary divergence** observed in failing runs |
| Accepted transition | sets TO_UGV and goal when decision consumed | same logic existed but only for routed message | Missing direct-topic decision integration |
| Movement loop | TO_UGV branch active once state set | same | Movement code works if state/goal are set |

---

## Likely root cause(s)

1. **Missing action path for direct `/ugv/charge_decisions` in UAV node**:
   - `charge_decision_sub_` was declared but not created.
   - CH could request and UGV could accept, yet CH might never consume acceptance unless routed control message arrives.

2. Secondary runtime blocker visibility gap:
   - Existing logs were not structured enough to quickly prove whether stall was due to missing decision intake, missing goal, or movement guard.

---

## Fix applied and why

### A) Unified decision application path

Added subscriber to `/ugv/charge_decisions` and routed both direct-topic and routed-network decision flows into a single function:

- `applyAcceptedChargeDecision(source, accepted, slot_id, payload_has_pose, payload_pose)`

Why: prevents behavior drift between two intake channels and ensures accepted decision always updates charging state/goal identically.

### B) Structured Charging Debug Trace layer

Added parameter-gated trace logging (`charging_debug_trace`, default false) with grep-friendly single-line `key=value` output.

Implemented:

- `chargeTraceSnapshot(event, reason)`
- `logChargeTrace(event, reason, throttle, throttle_ms)`
- `movementBlockReason()` standardized reason codes

Instrumented events:

- `TRIGGER_REQUEST`
- `SEND_REQUEST`
- `RECV_DECISION`
- `ACCEPT_DECISION`
- `STATE_TRANSITION`
- `SET_GOAL`
- `MOVE_TICK` (throttled)
- `MOVE_BLOCKED` (with reason)

Why: lets operators follow exact sequence from request to actuation and identify first failing stage immediately.

### C) Blocked reason codification

`movementBlockReason()` returns reason codes from the requested set, including:

- `RECOVERY_ACTIVE`
- `DEPLOYMENT_FREEZE`
- `HOLD_POSITION`
- `NO_GOAL_SET`
- `CONTROLLER_DISABLED`
- `WAITING_FOR_SOMETHING`
- `TASK_LOCK`
- `UNKNOWN`

(plus charge trace emission on blocked returns in `mobilityStep`).

---

## How to reproduce and verify

### Build
```bash
cd ~/ros2_ws
colcon build --symlink-install
source install/setup.bash
```

### Run the scenario
```bash
ros2 launch system_bringup bringup.launch.py run:=<YOUR_RUN_NAME>
```

### Observe decision + motion
```bash
ros2 topic echo /ugv/charge_decisions
ros2 topic echo /fanet/network_bus_raw
ros2 topic echo /fanet/network_bus
ros2 topic echo /fanet/status
```

### Watch CH pose and goal
```bash
ros2 topic list | grep -E "pose|odom|tf|goal|cmd|twist"
ros2 topic echo /<CH_NS>/odom
ros2 topic echo /<CH_NS>/goal_pose
ros2 topic echo /<CH_NS>/cmd_vel
```

### Grep logs (runtime)
```bash
ros2 run rqt_console rqt_console
```

Or:
```bash
ros2 launch system_bringup bringup.launch.py run:=<YOUR_RUN_NAME> | grep "CHARGE_TRACE"
```

### Expected verification sequence

For CH, after accepted decision, you should observe:

1. `event=ACCEPT_DECISION` / `event=STATE_TRANSITION` with `charge_state=TO_UGV`
2. then either:
   - `event=SET_GOAL` followed by `event=MOVE_TICK` with decreasing `dist_to_goal`, or
   - `event=MOVE_BLOCKED reason=<CODE>` identifying exact blocker.

---

## Charging Debug Mode parameter

- Parameter: `charging_debug_trace` (bool)
- Default: `false`
- Behavior:
  - `false`: existing minimal logs
  - `true`: full `CHARGE_TRACE` events (movement tick throttled to ~1Hz)

Example:
```bash
ros2 param set /<UAV_NODE_NAME> charging_debug_trace true
```
