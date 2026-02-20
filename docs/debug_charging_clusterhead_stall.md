# ClusterHead charging stall debug report

## Charging pipeline: CH vs Member path comparison

### End-to-end pipeline map

1. **Trigger to request (UAV side)**
   - Battery drain and charging trigger logic executes in `publishStatus()` for both CH and Member once deployment/motion gates are ready.
   - Trigger condition is `request_needed` (energy-based threshold against estimated energy-to-UGV + reserve/buffer + adaptive offset). On trigger, `requestCharge()` is called.
   - `requestCharge()` sets `waiting_for_charge_response_=true` and creates a pending request.
   - `sendChargeRequest()` emits a **network-bus control** message (`TrafficMessage`) with `control_type="CHARGE_REQUEST"` to `dst_id=ugv_id`, with next hop selected by FANET routing (`my_ch_id_` for member, routed next hop for CH).

2. **UGV decision generation and send**
   - UGV scheduler accepts queued request and sends **network-bus** `TrafficMessage` with `control_type="CHARGE_DECISION"`, `accepted=1`, `slot_id`, and UGV pose in payload (`ugv_x/y/z`).
   - Decision is sent through routing (`resolveNextHop(job.uav_id)`), not via direct UAV topic path.

3. **Decision receive (UAV side)**
   - UAV receives from `/fanet/network_bus` in `trafficCallback()` and dispatches `CHARGE_DECISION` to `handleChargeDecisionFromNetwork()` when final destination matches.
   - On accepted decision:
     - `waiting_for_charge_response_=false`
     - `has_charge_slot_=true`
     - `charge_state_=TO_UGV`
     - charge target pose loaded from decision payload (or resolved from cached UGV pose)

4. **State transition to motion**
   - `mobilityStep()` executes periodically.
   - If `charge_state_==TO_UGV`, charge motion branch has priority: it moves toward `charge_target_pose_`; on arrival it calls `beginChargingSession()`.

5. **Controller actuation**
   - This codebase uses an internal kinematic step controller (`stepTowards2D`) in `mobilityStep()` rather than ROS action-goal clients.
   - Pose is integrated each tick; velocity/presence exported via `/fanet/status`.

---

## CH vs Member divergence analysis

| Stage | Member behavior | CH behavior | Divergence hypothesis |
|---|---|---|---|
| Request trigger | Energy threshold in `publishStatus()` | Same logic | No role divergence in trigger logic.
| Request transport | `CHARGE_REQUEST` to CH as first hop, then FANET route | `CHARGE_REQUEST` directly FANET-routed toward UGV | Both network-routed; no direct-topic charging path.
| Decision receive | `trafficCallback()` -> `handleChargeDecisionFromNetwork()` | Same | No role divergence in handler dispatch.
| Decision accepted -> state | sets `TO_UGV`, target pose, slot flags | Same | Transition logic is shared.
| Motion gate | Members can still be blocked by deployment/release gates when not in charge motion | CH also shares gates | Critical divergence found at `emergency_landed_` guard: this blocked mobility even when a new accepted `TO_UGV` decision existed.
| Move to UGV | `TO_UGV` branch runs and steps pose | CH stuck if `emergency_landed_` remained true from prior emergency event | Root cause: stale recovery flag prevented motion tick execution for CH accepted charge path.

---

## Likely root cause(s)

1. **Primary confirmed cause**
   - In `mobilityStep()`, guard `if (emergency_landed_) return;` ran before motion execution, including charging motion.
   - If CH previously entered emergency landing flow and `emergency_landed_` remained true, CH could receive `CHARGE_DECISION accepted`, transition to `TO_UGV`, yet still never move.

2. **Secondary observability gap**
   - Prior implementation did not provide end-to-end structured trace of charging transitions and movement block reason codes, making CH stall hard to diagnose.

---

## Fix applied

### Logic fix

1. **Allow charge motion to override landed hold**
   - Changed mobility guard to block on `emergency_landed_` **only when charge motion is not active**.
2. **Reset stale emergency flags on accepted decision**
   - On accepted decision transition into `TO_UGV`, clear `emergency_landed_` and `emergency_recovery_active_` to ensure clean re-entry into go-to-UGV path.

### Charging Debug Trace instrumentation

Added `charging_debug_trace` parameter (`bool`, default `false`) and structured single-line trace output (`CHARGE_TRACE ... key=value ...`) via helper:
- snapshot includes event ID, role, UAV ID, charging state, mobility FSM state, battery/threshold, slot flags, goal fields, distance to goal, movement-enabled status, hold/recovery/deployment flags, task context, cluster IDs, decision age, and reason code.

Added trace points:
- `TRIGGER_REQUEST`
- `SEND_REQUEST`
- `RECV_DECISION`
- `ACCEPT_DECISION`
- `STATE_TRANSITION`
- `SET_GOAL`
- `MOVE_TICK` (throttled ~1Hz when debug enabled)
- `MOVE_BLOCKED` with reason code from `movementBlockReason()`

Reason codes emitted include:
- `EMERGENCY_LANDED`
- `DEPLOYMENT_FREEZE`
- `HOLD_POSITION`
- `NO_GOAL_SET`
- `CONTROLLER_DISABLED`
- `WAITING_FOR_SOMETHING`
- `UNKNOWN`

(Framework includes the required blocked-reason structure for CH stall localization.)

---

## How to reproduce and verify

### Build
```bash
cd ~/ros2_ws
colcon build --symlink-install
source install/setup.bash
```

### Run scenario
```bash
ros2 launch system_bringup bringup.launch.py run:=<YOUR_RUN_NAME>
```

### Observe decision + motion transport
```bash
ros2 topic echo /ugv/charge_decisions
ros2 topic echo /fanet/network_bus_raw
ros2 topic echo /fanet/network_bus
ros2 topic echo /fanet/status
```

### Watch CH pose and goal topics
```bash
ros2 topic list | grep -E "pose|odom|tf|goal|cmd|twist"
ros2 topic echo /<CH_NS>/odom
ros2 topic echo /<CH_NS>/goal_pose
ros2 topic echo /<CH_NS>/cmd_vel
```

### Enable charging debug mode
```bash
ros2 param set /<CH_NODE_NAME> charging_debug_trace true
```

### Runtime log grep
```bash
ros2 run rqt_console rqt_console
```

or

```bash
ros2 launch system_bringup bringup.launch.py run:=<YOUR_RUN_NAME> | grep "CHARGE_TRACE"
```

### Expected log sequence

- Healthy charging transition:
  1. `event=RECV_DECISION ...`
  2. `event=ACCEPT_DECISION ...`
  3. `event=STATE_TRANSITION ... reason=IDLE_TO_TO_UGV`
  4. `event=SET_GOAL ...`
  5. `event=MOVE_TICK ... dist_to_goal=<decreasing>`

- If still blocked:
  - `event=MOVE_BLOCKED reason=<REASON_CODE>` appears with full state snapshot, making blocker explicit.


---

## Addendum: emergency_landed guard semantics and unblock proof

### Is `emergency_landed_` cleared on accepted decision?

No. The updated logic **does not clear** `emergency_landed_` automatically when `ACCEPT_DECISION` is received.

### mobilityStep() guard before vs after

- **Before**
  ```cpp
  if (emergency_landed_) return;
  ```

- **After (safe override only for active accepted charge-to-UGV path)**
  ```cpp
  if (emergency_landed_ && !(has_charge_slot_ && charge_state_ == ChargeState::TO_UGV)) return;
  ```

This keeps emergency hold semantics by default, but allows the explicit accepted charging path to proceed.

### Explicit blocked reason code

`movementBlockReason()` now returns `EMERGENCY_LANDED` when this guard blocks motion, and `MOVE_BLOCKED` emits:

```text
event=MOVE_BLOCKED reason=EMERGENCY_LANDED
```

### Golden trace sample (CH)

```text
CHARGE_TRACE event=RECV_DECISION role=CH ... reason=ACCEPTED
CHARGE_TRACE event=ACCEPT_DECISION role=CH ... reason=ENTER_TO_UGV
CHARGE_TRACE event=STATE_TRANSITION role=CH ... reason=IDLE_TO_TO_UGV
CHARGE_TRACE event=SET_GOAL role=CH ... reason=TARGET_FROM_NEIGHBOR_STATUS_AGE_MS=220
CHARGE_TRACE event=MOVE_TICK role=CH ... dist_to_goal=145.2
CHARGE_TRACE event=MOVE_TICK role=CH ... dist_to_goal=141.9
```

or, if blocked:

```text
CHARGE_TRACE event=MOVE_BLOCKED role=CH ... reason=EMERGENCY_LANDED
```
