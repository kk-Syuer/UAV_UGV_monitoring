# Charging Queue Dynamics Verification

## Scope and implementation location

- Queue implementation: `UgvChargerNode` in `src/ugv_charger/src/ugv_charger_node.cpp`.
- Storage structure: `std::deque<QueueEntry> queue_`.
- Scheduling loop: `schedulerLoop()` (runs every 500 ms via timer).
- Next-UAV selector: `chooseNextIndex(const rclcpp::Time & now)`.
- Supported policy enum values: `FCFS`, `ROLE_PRIORITY`, `EDF`, `DYNAMIC_SCORE`.

## Key evidence patterns (static vs dynamic)

### Static signs found

1. Queue entries store snapshot fields captured at enqueue/update time:
   - `QueueEntry.role`, `QueueEntry.battery_level`, `QueueEntry.request_time`, `QueueEntry.request_msg_id`.
2. Enqueue/update path writes `role`, `battery_level`, and `request_time` from *current* status into the queue entry.
3. There is no queue-wide `sort()`/`rebuild heap`/`priority key update` after status updates.
4. Status callbacks update `uav_status_` only; they do not synchronize queued entry battery/role unless a new/retried `CHARGE_REQUEST` arrives.

### Dynamic signs found

1. Scheduler decision is recomputed each scheduling decision (every 500 ms) by scanning `queue_` in `chooseNextIndex(now)`.
2. `DYNAMIC_SCORE` recalculates wait-time term from `now - request_time` at each decision.
3. `EDF` recomputes TTE formula each decision, but using queue snapshot battery (`QueueEntry.battery_level`) rather than latest `uav_status_`.

## Per-policy assessment

### 1) FCFS (`fcfs`)

- **Is queue dynamic?** No (selection is static by insertion order).
- **When computed?** Order established at enqueue; selection is always index 0.
- **Priority inputs:** Arrival order only.
- **Do inputs change over time?** N/A for order.
- **Re-sort/re-heapify on change?** No.
- **Consequence:** Lower responsiveness to urgency; a critically low-battery UAV can wait behind older requests.
- **Where to modify for dynamic behavior:**
  - `chooseNextIndex()` branch `Policy::FCFS` to use time-varying metrics.
  - Optionally add periodic reorder in `schedulerLoop()` before selecting.

### 2) Role priority (`role_priority`)

- **Is queue dynamic?** Partially dynamic at decision time, but based on potentially stale role snapshot in `queue_`.
- **When computed?** Recomputed each decision by scanning for first `role == 1` in current queue order.
- **Priority inputs:** `QueueEntry.role` + fallback FCFS position.
- **Do inputs change over time?** UAV role could change via status, but queued role is only refreshed when that UAV sends another `CHARGE_REQUEST` (duplicate update path).
- **Re-sort/re-heapify on change?** No full reorder; linear scan each decision.
- **Consequence:** If role changes without re-request, ordering may be inconsistent with current fleet role state.
- **Where to modify for full dynamic behavior:**
  - In `chooseNextIndex()`, read current role from `uav_status_` by `uav_id` instead of `QueueEntry.role`.
  - In `statusCallback()`, update matching queued entries (or recompute on-the-fly from status map).

### 3) EDF (`edf`)

- **Is queue dynamic?** Decision is recomputed each selection, but **effective priority is mostly static** because it uses enqueue-time battery snapshots.
- **When computed?** Recomputed each scheduling decision by scanning all queue entries.
- **Priority inputs:** `QueueEntry.battery_level` and role-specific drain params (`drain_percent_ch_`, `drain_percent_member_`).
- **Do inputs change over time?** Real battery changes over time, but queued battery is stale unless request is refreshed.
- **Re-sort/re-heapify on change?** No; no automatic key update on status callback.
- **Consequence:** EDF can become inaccurate over time (deadline ordering may not reflect actual remaining energy).
- **Where to modify for full dynamic EDF:**
  - `chooseNextIndex()` EDF branch: replace `q.battery_level` with latest `uav_status_[q.uav_id].battery_level` when available.
  - Optionally maintain/update queue entry battery in `statusCallback()` for queued UAVs.

### 4) Dynamic score (`dynamic` / `DYNAMIC_SCORE` enum)

- **Is queue dynamic?** Mixed:
  - **Dynamic component:** waiting-time term (`w_wait * wait_sec`) is truly time-varying and recomputed every decision.
  - **Static/stale component:** battery and role terms come from `QueueEntry` snapshot unless CHARGE_REQUEST refreshes entry.
- **When computed?** Recomputed each scheduling decision.
- **Priority inputs:** role, battery deficit `(100 - battery)`, and waiting time.
- **Do inputs change over time?** waiting time increases continuously; actual battery/role can change but are not auto-refreshed from status map.
- **Re-sort/re-heapify on change?** No persistent reorder; decision-time scan only.
- **Consequence:** ranking evolves due to wait time but can be wrong on role/battery if status drifts from queued snapshot.
- **Where to modify for full dynamic behavior:**
  - `computeDynamicScore()` and/or `chooseNextIndex()` to pull freshest role+battery from `uav_status_`.
  - Optional: sync `queue_` entries in `statusCallback()` for queued UAVs.

## Direct answer to the goal question

The charging waiting list is **not purely static and not fully dynamic**:

- It is **decision-time recomputed** (dynamic selection pass every scheduler tick).
- But many policies rely on **snapshot attributes stored in queue entries**, so they are **partially static** unless UAVs resend `CHARGE_REQUEST` and refresh their entry.

Most accurate statement: **hybrid/partially dynamic queueing** (dynamic index selection over a mostly static per-entry data snapshot).
