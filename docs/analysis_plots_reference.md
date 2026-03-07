# Analysis Plots Reference

**Script:** `scripts/run_all_plots_test_round2.py`
**Output root:** `analysis/test_round2/figures/`

Each plot is labelled with its group number and a short code (`G1/P1`, `G7/A2-M`, …).
The suffix **-M** means *replicates merged* (averaged across runs of the same protocol).
All time axes are in **minutes** relative to experiment start (`t_rel_s / 60`).

---

## Key Data Sources

| Logical name | CSV file | Key columns |
|---|---|---|
| `network_timeseries` | `network_timeseries.csv` | `t_rel_s`, `window_pdr`, `window_delay_mean_ms` |
| `charge_events` | `charge_events.csv` | `t_rel_s`, `outcome`, `role`, `decision_latency_ms`, `effective_wait_ms` |
| `charge_queue_timeseries` | `charge_queue_timeseries.csv` | `t_rel_s`, `queue_length_ugv/ch/member`, `ugv_dock_utilization` |
| `death_events` | `death_events.csv` | `t_rel_s`, `role` |
| `packet_generated_events` | `packet_generated_events.csv` | `t_rel_s`, `control_type`, `creation_time_s` |
| `packet_delivered_events` | `packet_delivered_events.csv` | `creation_time_s`, `delivered_time_s` |
| `qos_metrics` | `qos_metrics.csv` | `generated`, `delivered`, `pdr` |
| `status_timeseries` | `status_timeseries.csv` | `t_rel_s`, `battery_level`, `node_id` |
| `weather_timeseries` | `weather_timeseries.csv` | `t_rel_s`, `regime`, `rain_intensity`, `wind_speed` |
| `charge_session_events` | `charge_session_events.csv` | `energy_charged_wh`, `battery_before/after` |

---

## Group 01 — Network & Battery Validation

### `01_network_pdr_over_time` · G1/P1
**What is drawn:** Line plot — rolling-window PDR over experiment time. One line per run, all protocols overlaid on a single axes.
**Data source:** `network_timeseries` → `window_pdr`, `t_rel_s`.
**Processing:** Filters `window_pdr >= 0`; converts seconds to minutes. Each data point is the PDR computed by the ROS network-monitor node over a sliding window.
**Read as:** Tracks whether packets are successfully delivered as the experiment progresses. Drops reveal moments of congestion or UAV death.

---

### `01_battery_ecdf` · G1/P2
**What is drawn:** ECDF (empirical CDF) of UAV battery levels — one curve per run.
**Data source:** `status_timeseries` → `battery_level`. Sink/gateway nodes are excluded.
**Processing:** All `battery_level` samples from a run are sorted; the ECDF maps each value to its cumulative fraction. No time dimension — the whole run is collapsed to a distribution.
**Read as:** A curve far to the right means UAVs maintain high battery on average; a curve shifted left indicates frequent deep discharges.

---

### `01_packet_generated_per_run` · G1/P3
**What is drawn:** Stacked bar chart — total packets generated per run, broken down by packet type (`DATA`, `CONTROL`, `Other`).
**Data source:** `packet_generated_events` → `control_type`.
**Processing:** Counts events per type per run. Rare types are collapsed into `Other` for readability.
**Read as:** Validates that all runs produced a comparable workload. Large differences between runs may indicate data-collection issues.

---

### `01_network_pdr_over_time_merged` · G1/P1-M
**What is drawn:** PDR timeseries — one line per protocol showing the **mean** across replicates.
**Data source:** `network_timeseries` → `window_pdr`, `t_rel_s`.
**Processing:** Each replicate's PDR timeseries is linearly interpolated onto a shared time grid (`bin_sec`-spaced). `np.nanmean` is taken across replicates at every grid point.
**Read as:** Smoothed protocol comparison free from single-run noise.

---

### `01_battery_ecdf_merged` · G1/P2-M
**What is drawn:** Battery ECDF — one curve per protocol, all replicates pooled.
**Data source:** `status_timeseries` → `battery_level`.
**Processing:** All battery samples across all replicates of a protocol are concatenated before computing the ECDF.
**Read as:** Representative battery distribution for each scheduling policy.

---

### `01_pdr_cdf_merged` · G1/P3-M
**What is drawn:** CDF of `window_pdr` values ≥ 0.5, one curve per protocol, all replicates pooled.
**Data source:** `network_timeseries` → `window_pdr`.
**Processing:** Values below the `pdr_min` threshold (default 0.5) are excluded to focus on the high-PDR tail. ECDF is computed over the pooled samples.
**Read as:** How reliably a protocol keeps PDR above the acceptable floor.

---

### `01_packet_generated_merged` · G1/P4-M
**What is drawn:** Two side-by-side stacked bar panels — (left) absolute mean total packets per protocol with std error bars; (right) normalised ratio per packet type.
**Data source:** `packet_generated_events` → `control_type`.
**Processing:** Packet counts are computed per replicate, then mean and std are computed across replicates. Ratios are computed from the mean counts.
**Read as:** Whether protocols generate similar traffic volumes, and whether the breakdown by packet type is consistent.

---

### `01_mean_pdr_merged` · G1/P5-M
**What is drawn:** Bar chart — overall mean PDR per protocol, error bars = std across replicates.
**Data source:** `qos_metrics` → `generated`, `delivered`.
**Processing:** Per replicate: `PDR = sum(delivered) / sum(generated)`. Mean and std are then computed across replicates.
**Read as:** Single-number network reliability summary per scheduling policy.

---

## Group 02 — Per-Protocol Charging Statistics

### `02_charge_success_rate` · G2/P1
**What is drawn:** Bar chart — charge success rate per run.
**Data source:** `charge_events` → `outcome`; or `summary.json` fallback.
**Processing:** `success_rate = count(outcome == "STARTED") / total_requests`.
**Read as:** How often a UAV that requests charging actually gets a dock slot.

---

### `02_decision_latency` · G2/P2
**What is drawn:** Boxplot — time between a charge request and the scheduler's decision, per protocol.
**Data source:** `charge_events` → `decision_latency_ms`. Values of −1 (missing) are removed.
**Processing:** Pools raw latency values per protocol across all runs.
**Read as:** Scheduling responsiveness. High latency means UAVs wait longer for an answer even before queuing.

---

### `02_effective_wait` · G2/P3
**What is drawn:** Boxplot — effective waiting time from request to dock arrival, per protocol.
**Data source:** `charge_events` → `effective_wait_ms`. Values of −1 removed.
**Processing:** Pools values per protocol.
**Read as:** Total delay experienced by the UAV. Combines decision latency and transit time.

---

### `02_energy_recovered` · G2/P4
**What is drawn:** Boxplot — energy recovered per completed charge session, per protocol.
**Data source:** `charge_session_events` (primary) → `energy_charged_wh`; or computed from `battery_before/after`; fallback `charge_events` → `energy_recovered_pct`.
**Processing:** Energy values are pooled per protocol.
**Read as:** How much energy is returned per charging visit. Protocols that allow longer sessions or fewer interruptions recover more.

---

### `02_*_merged` · G2/P1–P4-M
Same as the non-merged variants but with **replicates pooled** into one distribution per protocol (box plots) or averaged with error bars (bar charts). Provides more statistically robust comparisons.

---

## Group 03 — Cross-Protocol Charging Dynamics

### `03_charge_queue_length` · G3/P1
**What is drawn:** Line plot — total charge queue length over time (all protocols overlaid).
**Data source:** `charge_queue_timeseries` → `queue_length_ugv`, `queue_length_ch`, `queue_length_member`, `t_rel_s`.
**Processing:** Sums available queue columns per timestep.
**Read as:** How many UAVs are waiting to charge at any moment. Sustained large queues signal capacity bottlenecks.

---

### `03_dock_utilization` · G3/P2
**What is drawn:** Line plot — UGV dock utilisation over time (0 = empty, 1 = fully occupied).
**Data source:** `charge_queue_timeseries` → `ugv_dock_utilization`, `t_rel_s`.
**Processing:** Filters `ugv_dock_utilization >= 0`.
**Read as:** How intensively the charging infrastructure is used. High utilisation is efficient but leaves no slack for surges.

---

### `03_charge_outcome_breakdown` · G3/P3
**What is drawn:** Stacked bar chart — count of charge outcomes per protocol per run.
**Data source:** `charge_events` → `outcome`.
**Processing:** Value counts of `STARTED`, `REJECTED`, `DROPPED`, `TIMEOUT`, `PREEMPTED`, `ENERGY_DEPLETED`. `PREEMPTED` is suppressed for non-preemptive protocols (those without `_p_` in the protocol name).
**Read as:** What happens to charge requests. A large `DROPPED` or `TIMEOUT` fraction indicates the scheduler is losing too many requests.

---

### `03_dead_uav_cumulative` · G3/P4
**What is drawn:** Step plot — cumulative UAV deaths over time.
**Data source:** `death_events` → `t_rel_s`; fallback `charge_queue_timeseries` → `dead_event_count`.
**Processing:** Each death event increments a counter; the step function is plotted vs. time.
**Read as:** The total UAV attrition rate. A steep slope indicates that the charging policy is failing to keep enough UAVs alive.

---

### `03_cumulative_energy_charged` · G3/P5
**What is drawn:** Step plot — total energy delivered to UAVs over time, per run.
**Data source:** `charge_session_events` → `energy_charged_wh`; fallback `charge_events` → `energy_recovered_pct` scaled by battery capacity.
**Processing:** Events are sorted by time and energy is accumulated.
**Read as:** Charging throughput. A steeper slope means the system is delivering more energy per unit time.

---

### `03_*_merged` · G3-M variants
Replicates are merged onto a common time grid and mean (and std where applicable) is computed. Outcome breakdown sums counts across all replicates before plotting.

---

## Group 04 — Policy Radar

### `04_policy_radar` · G4/P1
### `04_policy_radar_merged` · G4/P1-M
**What is drawn:** Radar / spider chart — five normalised KPIs plotted as a polygon per protocol.
**Data source:** `summary.json` (pre-computed KPIs), or falls back to computing from CSVs:

| Axis | Metric | Direction |
|---|---|---|
| PDR | Mean packet delivery ratio | Higher = better |
| Success Rate | Fraction of charge requests that started | Higher = better |
| Low Latency | 1 − normalised decision latency | Higher = better |
| Energy Recovery | Mean energy recovered per session | Higher = better |
| Survival | Fraction of UAVs alive at end | Higher = better |

**Processing:** Each KPI is normalised to [0, 1] across all protocols. For the merged version, KPIs are averaged across replicates before normalisation.
**Read as:** A polygon that covers more area is better overall. Reveals trade-offs between policies (e.g., low latency vs. high energy recovery).

---

## Group 05 — Weather Interaction

### `05_pdr_vs_weather_regime` · G5/P1
**What is drawn:** Boxplot — window PDR grouped by weather regime (Clear / Cloudy / Rain / Storm, etc.).
**Data source:** `network_timeseries` → `window_pdr`, `t_rel_s`; joined with `weather_timeseries` → `regime`, `t_rel_s` via a nearest-time merge.
**Processing:** Each PDR sample is labelled with the weather regime active at that moment. One box per regime.
**Read as:** Whether adverse weather degrades network performance.

---

### `05_pdr_vs_weather_regime_merged` · G5/P1-M
**What is drawn:** Grouped bar chart — mean PDR per (protocol, weather regime), replicates pooled.
**Data source:** Same as above.
**Processing:** PDR values are grouped by `(protocol, regime)` across all replicates; mean is computed.
**Read as:** Side-by-side comparison of how each scheduling policy handles different weather conditions.

---

### `05_weather_timeseries` · G5/P2
**What is drawn:** Multi-panel line plot — rain intensity, wind speed, and temperature over time for one representative run.
**Data source:** `weather_timeseries` → `rain_intensity`, `wind_speed`, `temperature_c`, `t_rel_s`.
**Processing:** One run is selected automatically. Available weather columns are stacked as separate subplot panels.
**Read as:** Environmental context for interpreting network and charging anomalies.

---

### `05_delay_vs_weather_regime` · G5/P3
**What is drawn:** Boxplot — window mean E2E delay grouped by weather regime, all protocols overlaid.
**Data source:** `network_timeseries` → `window_delay_mean_ms`; joined with `weather_timeseries` → `regime`.
**Processing:** Same nearest-time join as G5/P1. The delay column is computed by the ROS node from `rclcpp::Time` objects and is not affected by CSV timestamp precision.
**Read as:** Whether rain or wind increases message latency.

---

### `05_delay_vs_weather_regime_merged` · G5/P3-M
**What is drawn:** Grouped bar chart — mean E2E delay per (protocol, weather regime), replicates pooled.
**Data source:** Same as above.
**Processing:** Delay values grouped by `(protocol, regime)`, mean computed across replicates.
**Read as:** Cross-protocol comparison of delay sensitivity to weather.

---

## Group 06 — Network QoS & End-to-End Delay

### `06_qos_heatmap` · G6/P1
### `06_qos_heatmap_merged` · G6/P1-M
**What is drawn:** Heatmap — PDR per message category (DATA / CONTROL) × protocol. Cells are annotated with the numeric PDR value.
**Data source (primary):** Join of `packet_generated_events` and `packet_delivered_events` on `creation_time_s`. This covers all packet types.
**Data source (fallback):** `qos_metrics` when the join is unavailable.
**Processing:** `PDR = delivered / generated` per `(protocol, category)`. Colour scale: RdYlGn from 0 to 1. In the merged version, values are averaged across replicates.
**Read as:** Whether DATA packets or CONTROL packets have worse delivery. Helps isolate whether network problems affect mission-critical traffic disproportionately.

---

### `06_e2e_delay` · G6/P2
### `06_e2e_delay_merged` · G6/P2-M
**What is drawn:** Boxplot — distribution of E2E delay values per protocol.
**Data source (priority order):**
1. `network_timeseries` → `window_delay_mean_ms` (one value per 10-second window; authoritative ROS computation)
2. Join of `packet_generated_events` and `packet_delivered_events` → `delivered_time_s − creation_time_s`
3. `messages.csv` → `e2e_delay_ms`
**Processing:** Values are pooled per protocol. For merged, replicates are combined.
**Read as:** Typical and tail latency per protocol. Long tails can interfere with time-critical coordination messages.

---

### `06_mean_e2e_delay_merged` · G6/P3-M
**What is drawn:** Bar chart — mean E2E delay per protocol, error bars = std across replicates.
**Data source:** Same priority as G6/P2.
**Processing:** Mean delay is computed per replicate; mean ± std is aggregated across replicates.
**Read as:** Concise latency ranking across protocols.

---

### `06_corr_heatmap_pdr` · G6/Corr-PDR
**What is drawn:** Symmetric Pearson correlation heatmap — 7 per-run scalar KPIs including **mean PDR**.
**Data source:** Built from multiple CSVs per run:

| KPI | Source |
|---|---|
| Mean PDR | `qos_metrics` |
| Mean E2E delay | `packet_events` or `messages.csv` |
| Total deaths | `death_events` |
| Mean queue length | `charge_queue_timeseries` |
| Decision latency | `charge_events` |
| Charge success rate | `charge_events` |
| Energy recovered | `charge_session_events` |

**Processing:** Each metric is reduced to a single scalar per run. `np.corrcoef` is applied to the (n_runs × 7) matrix. Each cell is annotated with Pearson r.
**Read as:** Which pairs of KPIs move together across runs. A strong negative correlation between PDR and deaths confirms network failure drives UAV mortality.

---

### `06_corr_heatmap_e2e` · G6/Corr-E2E
Same structure as `06_corr_heatmap_pdr` but **mean E2E delay** replaces mean PDR as the primary network metric.

---

### `06_pdr_vs_e2e_scatter` · G6/Scatter
**What is drawn:** Scatter plot — mean PDR (Y) vs mean E2E delay in ms (X), one point per run coloured by protocol. A least-squares regression line is drawn across all protocols. Pearson r and p-value are annotated in the top-right corner.
**Data source:** Same scalar KPI dataframe used by the correlation heatmaps.
**Processing:** `scipy.stats.linregress` for regression; `scipy.stats.pearsonr` for annotation.
**Read as:** Whether high delay is associated with low PDR across runs and protocols.

---

## Group 07 — Causal Analysis

### Dip Threshold (used by all G7 PDR+event plots)
PDR dips are detected by `_pdr_dip_intervals`. The threshold is the **10th percentile (p10) of all non-negative `window_pdr` values** in the curve being analysed. Any contiguous block where PDR falls below p10 is shaded in gold as a "dip interval". On merged plots the p10 is computed on the mean curve, not per-replicate.

### Event Marker Types
| Marker | Colour | Event |
|---|---|---|
| Solid vertical line | Dark red | CH (cluster-head) UAV death |
| Dashed vertical line | Red | Any UAV death |
| Dash-dot vertical line | Steel blue | CH charging session started |
| Tick rug (binned) | Same colours | Binned variant: mean events per replicate per time bin |

---

### `07_pdr_events_{protocol}` · G7/A1
**What is drawn:** PDR timeseries for all replicates of one protocol on a single axes. Vertical event markers and gold dip-shading are overlaid.
**Data source:** `network_timeseries` → `window_pdr`; `death_events` + `charge_events` → event times.
**Processing:** Dip detection runs per replicate; the first replicate's threshold is displayed. Events from all replicates are pooled as individual vlines (cleaned: negatives, NaN, and out-of-range times removed).
**Read as:** Whether UAV deaths and charging events cluster inside PDR dips.

---

### `07_pdr_events_all_protocols` · G7/A1-panel
Same data and processing as `07_pdr_events_{protocol}` but all protocols are stacked as subplots sharing a common time axis. Useful for side-by-side comparison of when events happen relative to PDR under different scheduling policies.

---

### `07_pdr_events_merged_{protocol}` · G7/A1-M
**What is drawn:** Mean PDR timeseries (replicates merged) with event markers.
**Data source:** `network_timeseries` (replicates), `death_events`, `charge_events`.
**Processing:** Replicates interpolated onto a common grid; `np.nanmean` applied. Events pooled and **binned**: the full-height bar at each bin centre represents the *mean number of events per replicate per bin*. Tallest bar = 35 % of plot height.
**Read as:** Clean view of the average PDR trajectory with event density per time window.

---

### `07_pdr_events_merged_rawevents_{protocol}` · G7/A1-M (raw)
Identical to `07_pdr_events_merged_{protocol}` but events are drawn as individual full-height vlines (one line per event from all replicates pooled) rather than binned bars. Useful for locating precise event timing but can be visually dense.

---

### `07_pdr_events_merged_variance_{protocol}` · G7/A1-MV
**What is drawn:** Mean PDR line + **±1σ shaded band** + event markers (binned by default).
**Data source / processing:** Same as `07_pdr_events_merged_{protocol}` plus `np.nanstd` computed at each grid point.
**Read as:** Identifies whether PDR drops are consistent across replicates (narrow band) or highly variable (wide band).

---

### `07_pdr_events_merged_variance_rawevents_{protocol}` · G7/A1-MV (raw)
Same as `07_pdr_events_merged_variance_{protocol}` but with raw vlines instead of binned bars.

---

### `07_pdr_events_merged_all_protocols` · G7/A1-MP
All-protocols stacked panel. Mean PDR per protocol with binned event markers. Shares x-axis. Equivalent to running `07_pdr_events_merged_{protocol}` for every protocol and stacking the axes.

---

### `07_pdr_events_merged_rawevents_all_protocols` · G7/A1-MP (raw)
Same as above but using raw vlines.

---

### `07_pdr_events_merged_variance_all` · G7/A1-MVA
All-protocols stacked panel with **±1σ variance band** per protocol and binned event markers.

---

### `07_pdr_events_merged_variance_rawevents_all` · G7/A1-MVA (raw)
Same as above but with raw vlines.

---

### `07_window_pdr_dist_{protocol}` · G7/A3
**What is drawn:** Two-panel figure per protocol.
- **Left panel:** Histogram (density=True) of all `window_pdr` values pooled across replicates, overlaid with a Gaussian KDE (Scott's bandwidth). Mean, std, and n annotated. Dip threshold (p10) marked as a dashed vertical line.
- **Right panel:** Box-and-whisker per replicate so run-to-run variance is visible alongside the pooled distribution.
**Data source:** `network_timeseries` → `window_pdr`.
**Processing:** Values with `window_pdr < 0` excluded. KDE from `scipy.stats.gaussian_kde`.
**Read as:** Full distribution shape of network quality. If the distribution is bimodal (cluster near 0 and near 1), the protocol experiences prolonged outages rather than gradual degradation.

---

### `07_dock_util_with_timeouts` · G7/A2
**What is drawn:** Per-protocol subplots — dock utilisation timeseries for each replicate plus orange TIMEOUT event markers.
**Data source:** `charge_queue_timeseries` → `ugv_dock_utilization`; `charge_events` → TIMEOUT event times.
**Processing:** Each replicate plotted as a separate line. TIMEOUT markers are individual vlines from all replicates pooled.
**Read as:** Whether TIMEOUT events happen when the dock is fully occupied (utilisation = 1) or during slack periods, indicating scheduling inefficiency.

---

### `07_dock_util_with_timeouts_merged` · G7/A2-M
**What is drawn:** Mean dock utilisation ±1σ per protocol (replicates merged) with pooled TIMEOUT events shown as an orange rug at y ≈ 0.
**Data source:** Same as above.
**Processing:** `_merged_ts` bins and averages dock utilisation. TIMEOUT times are pooled from all replicates and drawn as tick marks proportional to count.
**Read as:** Cleaner view of the average dock pressure and where timeouts cluster.

---

### `07_lagged_corr_{protocol}` · G7/B
**What is drawn:** 2×2 grid of lagged Pearson correlation plots per protocol. Four signal pairs:

| X (leading) | Y (lagging) |
|---|---|
| PDR | TIMEOUT count per bin |
| PDR | Death count per bin |
| Dock utilisation | TIMEOUT count per bin |
| E2E delay | TIMEOUT count per bin |

**Data source:** `network_timeseries` (PDR, delay); `charge_queue_timeseries` (dock util); `charge_events` (TIMEOUT times); `death_events`.
**Processing:**
1. All timeseries are binned into `bin_sec`-second bins (default 60 s).
2. Lag range: −10 to +10 bins (± 10 minutes at default bin size).
3. `_lagged_pearson(x, y, max_lag)` shifts `y` by each lag and computes `np.corrcoef`.
4. The lag with maximum |r| is marked in red and its coordinates annotated.
5. Results saved to `summary_tables/07_lagged_corr_summary.csv`.

**Read as:** Positive lag on the X→Y axis means Y responds *after* X. Example: if PDR→TIMEOUT peaks at lag +2 min, PDR drops precede a surge in timeouts by ~2 minutes.

---

### `07_death_slope_vs_pdr_dip` · G7/C
**What is drawn:** Multi-panel boxplot per protocol — rolling death-rate slope during PDR-dip intervals vs. normal intervals.
**Data source:** `network_timeseries` → `window_pdr`, `t_rel_s`; `death_events` → event times.
**Processing:**
1. Death events are binned into `bin_sec` bins → `death_rate_per_bin`.
2. Rolling slope computed via `np.gradient` over a 3-bin window.
3. Each bin is labelled **dip** or **normal** using `_pdr_dip_intervals` (threshold = p10).
4. Slope values in dip and normal bins are collected and compared as boxplots.

**Read as:** If the dip box has a significantly higher slope than the normal box, it means death rates are *accelerating* during PDR drops — evidence that network failure causally drives fleet attrition.

---

## Output File Naming Convention

| Pattern | Meaning |
|---|---|
| `0X_name.png` | Per-run figure, all protocols overlaid |
| `0X_name_merged.png` | Replicates merged (mean across runs of same protocol) |
| `07_..._merged_{protocol}.png` | One figure per protocol, replicates merged |
| `07_..._rawevents_...` | Same figure but with raw individual vlines instead of binned bars |
| `07_..._variance_...` | Includes ±1σ shaded band across replicates |
| `07_..._all` | All protocols stacked in sub-panels |

---

## Event Cleaning & Binning Notes

All event times (`t_rel_s`) are validated by `_clean_event_times` before use:
- Removes `NaN` and negative timestamps (sensor artefacts).
- Removes events beyond the run's maximum `t_rel_s` (stale log entries).

In **merged** plots, events from N replicates are pooled after cleaning. The **binned** display mode counts events per `bin_sec` bin and normalises by N replicates, drawing a proportional bar (max height = 35 % of plot height). The **raw** mode draws one vertical line per event and can appear dense when N > 2.
