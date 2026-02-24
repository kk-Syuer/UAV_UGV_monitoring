# Data Contract

Generated from discovered policy/run folders under input root.

| File | Present in runs | Required columns found | Inferred semantics |
|---|---:|---|---|
| `messages.csv` | 0 | (none found) | Per-message outcomes; delivered flag and optional type labels + delays. |
| `charge_queue_timeseries.csv` | 0 | (none found) | Queue-length timeseries for charge scheduling over simulation time. |
| `death_events.csv` | 0 | (none found) | Failure/death event log with UAV identity and event time. |
| `charge_events.csv` | 0 | (none found) | Charging sessions with request/dock/end timing and battery/energy fields. |
