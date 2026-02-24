# UGV Charger Locker Usage Inference (Test Round 1)

## Question
Can we infer how different charging protocols affect:
1. UGV charger locker usage, and
2. total energy charged over time?

## Short answer
Yes. The existing `charge_queue_timeseries.csv`, `charge_events.csv`, and `summary.json` logs are enough to estimate locker utilization, queue pressure, charging throughput, and how those vary by protocol.

## Data used
- Per-run queue/locker telemetry from `experiment_data_collection/test_round1/ugv_*/charge_queue_timeseries.csv`.
- Per-charge-event outcomes and recovered energy from `experiment_data_collection/test_round1/ugv_*/charge_events.csv`.
- Run-level charging and fleet summaries from `experiment_data_collection/test_round1/ugv_*/summary.json`.

## Derived protocol comparison

| Protocol | Mean locker utilization | Mean active sessions | Peak queue length | Started charge events | Completed charge events | Total recovered energy (Wh) | Mean energy per event (Wh) | Run duration (h) | Energy rate (Wh/h) |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| ugv_dynamic | 0.720 | 2.161 | 3 | 62 | 60 | 4247.79 | 70.80 | 2.78 | 1529.2 |
| ugv_edf | 0.745 | 2.235 | 3 | 67 | 65 | 4568.06 | 70.28 | 2.78 | 1644.5 |
| ugv_fcfs | 0.740 | 2.221 | 3 | 72 | 70 | 4837.84 | 69.11 | 2.78 | 1741.6 |
| ugv_p_dynamic_score | 0.696 | 2.088 | 3 | 68 | 66 | 4384.08 | 66.43 | 2.78 | 1578.3 |
| ugv_p_edf | 0.715 | 2.146 | 3 | 70 | 68 | 4502.47 | 66.21 | 2.78 | 1620.9 |
| ugv_p_role_priority | 0.741 | 2.224 | 3 | 78 | 75 | 5127.12 | 68.36 | 5.56 | 922.9 |
| ugv_role_priority | 0.710 | 2.131 | 3 | 66 | 63 | 4408.59 | 69.98 | 2.78 | 1587.1 |

## Energy-over-time shape (first half vs second half of each run)

| Protocol | First-half recovered energy (Wh) | Second-half recovered energy (Wh) |
|---|---:|---:|
| ugv_dynamic | 3514.0 | 733.8 |
| ugv_edf | 4229.8 | 338.3 |
| ugv_fcfs | 2610.1 | 2227.8 |
| ugv_p_dynamic_score | 2070.3 | 2313.8 |
| ugv_p_edf | 3655.1 | 847.4 |
| ugv_p_role_priority | 5127.1 | 0.0 |
| ugv_role_priority | 1262.5 | 3146.1 |

## Interpretation
1. **Protocol choice does measurably affect total energy delivered.** In this dataset, total recovered energy varies from ~4248 Wh to ~5127 Wh across protocols.
2. **Locker occupancy is high in all runs** (mean utilization ~0.70–0.75, peak utilization 1.0), indicating a heavily loaded shared charger.
3. **Queue pressure saturates similarly** (peak queue length 3 for all listed runs), so differences are likely from scheduling decisions and event timing, not from a fundamentally different demand envelope.
4. **Energy timing differs by policy.** Some protocols front-load energy (e.g., EDF variants), while others deliver more in the second half (e.g., `ugv_role_priority`, `ugv_p_dynamic_score`).
5. **Caution:** `ugv_p_role_priority` appears to run for about twice the duration of the others, so raw total energy is not directly apples-to-apples; use Wh/h or normalize by equal time windows.

## Practical next step
If you want stronger causal claims, run each protocol with:
- same fixed simulation horizon,
- multiple seeds (>=10), and
- confidence intervals on Wh/h, completed charges, queue wait, and UAV survival.


## Reproducible plotting script
Use `tools/plot_ugv_locker_usage_comparison.py` to regenerate the table inputs and figures.

### Command to run
```bash
python tools/plot_ugv_locker_usage_comparison.py \
  --data_root experiment_data_collection/test_round1 \
  --out_dir analysis/test_round1/ugv_locker_usage
```

### If matplotlib is missing
```bash
python -m pip install matplotlib
```

### Outputs
- `analysis/test_round1/ugv_locker_usage/ugv_locker_usage_summary.csv`
- `analysis/test_round1/ugv_locker_usage/ugv_locker_usage_by_protocol.png`
- `analysis/test_round1/ugv_locker_usage/ugv_energy_recovered_by_protocol.png`
- `analysis/test_round1/ugv_locker_usage/ugv_energy_time_split_by_protocol.png`
