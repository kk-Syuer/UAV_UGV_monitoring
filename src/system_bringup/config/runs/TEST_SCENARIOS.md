# Preemptive Charging Protocol Test Scenarios

## Build & Setup

```bash
cd /home/user/UAV_UGV_monitoring
colcon build --symlink-install
source install/setup.bash
```

---

## Test 1: Non-Preemptive EDF (Baseline -- no preemption occurs)

```bash
ros2 launch system_bringup experiment.launch.py run:=ugv_edf
```

### Monitor live charging decisions:
```bash
ros2 topic echo /fanet/network_bus_raw | grep -E "CHARGE|PREEMPT"
```

### Expected logs:
- UGV logs show `UGV: assigned dock to <uav>` with `policy='edf'`
- **No** `PREEMPT_STOP_CHARGING` messages appear
- charge_events.csv: all rows have `preempted_flag=false`, `preempt_count=0`
- No `preemption_events.csv` file is created (empty)

---

## Test 2: Preemptive EDF (P-EDF) -- mid-charge preemption

```bash
ros2 launch system_bringup experiment.launch.py run:=ugv_p_edf
```

### Monitor:
```bash
ros2 topic echo /fanet/network_bus_raw | grep -E "CHARGE|PREEMPT"
```

### Expected logs (UGV):
```
UGV: PREEMPT_STOP_CHARGING slot victim=<UAV_A> winner=<UAV_B> delta_priority=<X.XXX> victim_charge_time_s=<Y.Y> victim_role=<R1> winner_role=<R2> victim_priority=<P1> winner_priority=<P2>
UGV: sending PREEMPT_STOP_CHARGING msg_id=ugv_1_preempt_<UAV_A>_<timestamp> dst=<UAV_A> via=<next_hop> backoff=<Z.Z>s
UGV: assigned dock to <UAV_B> (role=<R2>, batt=<B>%) with policy='p_edf'
```

### Expected logs (UAV_A -- victim):
```
UAV <UAV_A>: received PREEMPT_STOP_CHARGING, stopping charge, leaving dock. backoff=<Z.Z>s
UAV <UAV_A>: preempted at battery=<E> Wh (<P>%), returning to (<X>, <Y>).
UAV <UAV_A>: preemption backoff until t+<Z.Z>s
```

### Output files:
- `charge_events.csv`: at least one row with `preempted_flag=true`
- `preemption_events.csv`: at least one row with victim/winner details
- `summary.json`: `"preempted"` count > 0

---

## Test 3: Preemptive Role Priority (P-RolePriority) -- CH preempts member

```bash
ros2 launch system_bringup experiment.launch.py run:=ugv_p_role_priority
```

### Expected behavior:
- When a CH (role=1) requests charging while a member (role=0) is mid-charge, CH preempts.
- When a member requests while CH is charging, member does NOT preempt CH (guarded).
- Logs show `victim_role=0 winner_role=1` in preemption events.

### Expected logs:
```
UGV: PREEMPT_STOP_CHARGING slot victim=<member> winner=<CH> delta_priority=... victim_role=0 winner_role=1
```

---

## Test 4: Preemptive Dynamic Score (P-DynamicScore)

```bash
ros2 launch system_bringup experiment.launch.py run:=ugv_p_dynamic_score
```

### Expected behavior:
- UAV with higher dynamic score (role weight + battery urgency + wait time) preempts lower-scored occupant.
- Preemption only occurs when score gap >= `delta_priority_min` (0.25).
- Minimum charge time enforced (60s default).

---

## Test 5: Guardrails Prevent Thrashing

Test with aggressive drain rates to trigger more preemption attempts:

```bash
# Modify ugv_p_edf.yaml temporarily:
#   drain_rate_member: 0.148148  (4x speed)
#   preemption_min_charge_time_s: 60
#   preemption_cooldown_s: 120
#   preemption_max_per_uav: 3
ros2 launch system_bringup experiment.launch.py run:=ugv_p_edf
```

### Expected:
- After a UAV is preempted, it is NOT preempted again within 120s (cooldown).
- After 3 preemptions, that UAV is never preempted again (max cap).
- Preempted UAV waits `backoff_base_s + jitter` before re-requesting (unless emergency).

---

## Test 6: All Decisions Traverse FANET

```bash
ros2 topic echo /fanet/network_bus_raw --field control_type | grep -c "CHARGE_DECISION"
```

### Expected:
- ALL charge decisions (GRANT, REJECT, PREEMPT_STOP_CHARGING) appear on the FANET bus.
- No direct ROS topic shortcuts for preemption messages.

---

## Verification Checklist

| Criterion | How to Verify |
|-----------|---------------|
| P-EDF selectable | `charging_policy: p_edf` in YAML, UGV log shows `policy='p_edf'` |
| P-RolePriority selectable | `charging_policy: p_role_priority` in YAML |
| P-DynamicScore selectable | `charging_policy: p_dynamic_score` in YAML |
| Mid-charge interrupt works | UAV stops charging before full (victim log) |
| Guardrails prevent thrash | Cooldown/max-cap logs in UGV debug output |
| FANET-routed decisions | All CHARGE_DECISION msgs on `/fanet/network_bus_raw` |
| Monitor records preemption | `preemption_events.csv` and `charge_events.csv` columns |
| Unified schema | `preempted_flag`, `preempt_count` columns in charge_events.csv |
| Non-preemptive unchanged | Running `ugv_edf` produces no preemption events |
