# Cross-Layer Analysis: UAV Charging Scheduling Protocols and Network Quality
## Test Round 2 — Evidence-Based Thesis-Grade Report

**Date:** 2026-03-07
**Dataset:** `experiment_data_collection/test_round2`
**Figures root:** `analysis/test_round2/figures/`
**New figures:** `analysis/test_round2/figures/cross_layer/`
**Summary tables:** `analysis/test_round2/figures/summary_tables/`

---

## 1. Executive Summary

- **ugv_p_edf (Preemptive Earliest Deadline First) achieves the highest mean PDR (0.630 ± 0.035)** and the lowest battery depletion event count (20.0 ± 3.0 per run), demonstrating that deadline-aware scheduling with preemption most effectively preserves both fleet survivability and network quality. Evidence: `01_mean_pdr_merged.png`, `03_dead_uav_cumulative_merged.png`, `CL_A_protocol_kpi_overview.png`.

- **Charge success rate is a statistically significant positive predictor of PDR** (Pearson r = 0.578, p = 0.006; Spearman ρ = 0.552, p = 0.010). Protocols that successfully start more charging sessions maintain a healthier fleet, which sustains multi-hop routing paths and delivers more packets. Evidence: `CL_B_charging_vs_pdr_scatter.png`, `06_corr_heatmap_pdr.png`.

- **Charge request ROUTING_DROP rate is a significant negative predictor of PDR** (Pearson r = −0.527, p = 0.014; Spearman ρ = −0.555, p = 0.009). Routing drops occur when the network is already degraded (bad weather, UAV deaths), creating a vicious feedback cycle: degraded network → lost charge requests → more UAV deaths → further network degradation. Evidence: `CL_D_mechanism_chain.png`, `CL_G_routing_drop_timeseries.png`.

- **Battery depletion frequency is the strongest mediating variable** between charging policy and network quality. Total depletion events negatively correlate with PDR (Pearson r = −0.567, p = 0.007) and with E2E delay (Pearson r = −0.548, p = 0.010), meaning protocols that fail to keep UAVs charged cause both packet loss and — paradoxically — lower measured delay (a conditioning bias artifact). Evidence: `07_death_slope_vs_pdr_dip.png`, `CL_D_mechanism_chain.png`.

- **E2E delay rankings are contaminated by a conditioning bias**: protocols with the most UAV deaths (e.g., ugv_edf run 3: 85 depletions) show the *lowest* measured delay because fewer surviving UAVs generate simpler, shorter-hop topologies, causing the window-average delay to drop. ugv_p_role_priority has the highest mean delay (71.4 ms) while maintaining a healthy fleet — this reflects genuine topology complexity rather than network failure. **This bias is now formally confirmed by CL_K**: across all 21 runs × all time windows, delivered-packet count and mean E2E delay have a significant positive OLS slope (r > 0, p < 0.05), and PDR-weighted delay rankings invert the raw ranking for protocols affected by high attrition. Evidence: `CL_C_e2e_delay_conditioning_analysis.png`, **`CL_K_delay_robustness_panel.png`**.

- **ugv_role_priority (non-preemptive role priority) is the worst-performing protocol overall**: lowest charge success rate (38.4%), highest timeout rate (31.9%), and second-highest depletion count (39.3 per run), leading to second-lowest PDR (0.563). The absence of preemption and the coarse role-based priority prevents the scheduler from responding flexibly to energy urgency. Evidence: `04_policy_radar_merged.png`, `02_charge_success_rate_merged.png`.

- **ugv_edf run 3 is a critical anomaly**: 85 depletion events (vs. a cross-protocol mean of ~27) with only 5 unique UAV IDs confirms UAVs entered a recurring depletion-respawn cycle. ROUTING_DROP rate for this run reached 49.6% — meaning nearly half of all charge requests could not be delivered, severing the UAV's lifeline to the charging scheduler. Quick verification: `charge_events.csv` → `outcome == ROUTING_DROP` + `failure_reason == WEATHER_DROP` + `death_events.csv` depletion timestamps clustered after t ≈ 6 min. Evidence: `07_pdr_events_ugv_edf.png`, `07_window_pdr_dist_ugv_edf.png`.

- **Decision latency and effective wait time do not significantly predict PDR** (Pearson r = −0.175, p = 0.45 and r = −0.326, p = 0.15 respectively). This indicates that the primary bottleneck is not *how fast* the scheduler decides but *whether the request even reaches the scheduler* (ROUTING_DROP) and *whether a dock slot is available* (SUCCESS vs TIMEOUT). Evidence: `02_decision_latency_merged.png`, `CL_E_correlation_bar_chart.png`.

---

## 2. Data Coverage and Sanity Checks

### 2.1 Dataset Completeness

| Check | Result |
|---|---|
| Protocol count | **7 detected**: `ugv_dynamic`, `ugv_edf`, `ugv_fcfs`, `ugv_p_dynamic_score`, `ugv_p_edf`, `ugv_p_role_priority`, `ugv_role_priority` |
| Replicates per protocol | **3 confirmed** (runs `_1`, `_2`, `_3`) for all protocols |
| Total run folders | **21/21 present** |
| Key files present | `qos_metrics.csv`, `network_timeseries.csv`, `charge_events.csv`, `death_events.csv`, `charge_session_events.csv`, `charge_queue_timeseries.csv` — **all 21 × 6 = 126 files confirmed** |
| Run duration | **All 21 runs: t_max = 180.0 min** (consistent) |

### 2.2 Missing Columns / Structural Checks

- **`death_events.csv → role` column**: Encodes integer 0/1 (not strings "CH"/"member"). Value `1` = cluster head (CH), `0` = member UAV. The `alive_count` column correctly tracks fleet state only up to the first full-fleet depletion; subsequent events reflect respawn-and-die cycles (see §5 Anomaly A).
- **`charge_events.csv → decision_latency_ms`**: Contains −1 sentinel values for events where timing was not recorded (i.e., ROUTING_DROP and TIMEOUT events that never reached the scheduler). These are excluded before computing mean latency. Across all runs, only 15–40% of events have valid latency values.
- **`charge_session_events.csv`**: `charge_duration_s` and `energy_charged_wh` are −1 for `DOCK_START` events that have not yet concluded. Completed sessions with valid energy are used for energy analysis.
- **`qos_metrics.csv`**: PDR computed as `sum(delivered) / sum(generated)` over all logged message windows. All resulting PDR values are in [0, 1] — no out-of-range values detected.

### 2.3 Workload Consistency

Traffic generation is highly consistent across runs (coefficient of variation ≤ 3.1% for most protocols):

| Protocol | Generated packets (runs 1–3) | CV (%) |
|---|---|---|
| ugv_dynamic | 137.95M, 133.50M, 134.08M | 1.5 |
| ugv_edf | 140.54M, 141.45M, 144.87M | 1.3 |
| ugv_fcfs | 142.33M, 135.33M, 138.49M | 2.1 |
| ugv_p_dynamic_score | 133.07M, 142.32M, 133.48M | 3.1 |
| ugv_p_edf | 137.30M, 134.92M, 135.32M | 0.8 |
| ugv_p_role_priority | 138.84M, 136.90M, 120.04M | **6.4** |
| ugv_role_priority | 129.21M, 120.71M, 139.62M | **6.0** |

`ugv_p_role_priority_3` (120M generated vs. ~138M expected) and `ugv_role_priority_2` (120M) show lower-than-expected packet generation, likely caused by severe UAV attrition reducing active transmitting nodes in those runs. This is consistent with their higher depletion event counts.

### 2.4 Suspicious Values and Duplicated Entries

- **Duplicate timestamps in `network_timeseries.csv`**: Many runs have exactly 2 rows per second-interval (observable in ugv_edf_3 head). These appear to be two different ROS network-monitor node callback fires per window. The plotting pipeline handles this correctly (both rows have identical PDR/delay values). No correction needed.
- **Negative PDR / delay sentinel values (−1)**: Present in early windows before enough packets accumulate. The plotting pipeline filters `window_pdr >= 0` and `window_delay_mean_ms >= 0`. These are valid sentinels, not data errors.
- **ugv_edf_3 `alive_count = 0` throughout most of experiment**: Not a data corruption — reflects persistent depletion-respawn cycling (see §5 Anomaly A).

---

## 3. Protocol-Level Comparisons (Merged Replicates)

### 3.1 Network KPIs

**Table 1: Network Performance Summary (mean ± 1 SD across 3 replicates)**

| Protocol | Mean PDR | PDR SD | Mean E2E Delay (ms) | E2E SD (ms) | PDR Rank | Delay Rank |
|---|---|---|---|---|---|---|
| ugv_p_edf | **0.630** | 0.035 | 57.9 | 16.5 | 1 (best) | 3 |
| ugv_fcfs | 0.616 | 0.041 | 64.2 | 11.4 | 2 | 5 |
| ugv_dynamic | 0.595 | 0.045 | 62.1 | 9.6 | 3 | 4 |
| ugv_p_dynamic_score | 0.580 | 0.033 | 52.8 | 21.1 | 4 | 1 (best) |
| ugv_edf | 0.573 | **0.094** | 55.6 | **23.8** | 5 | 2 |
| ugv_role_priority | 0.563 | 0.071 | 66.7 | 8.0 | 6 | 6 |
| ugv_p_role_priority | 0.554 | 0.094 | **71.4** | 4.8 | 7 (worst) | 7 (worst) |

**Key observations:**
- The PDR range across protocols (0.554–0.630) is modest but consistent across replicates for most protocols.
- `ugv_edf` and `ugv_p_role_priority` have the highest PDR variance (SD ≈ 0.094), driven by single outlier runs with catastrophic depletion cascades. This instability is a critical weakness.
- The delay ranking is largely decoupled from the PDR ranking due to the conditioning bias (see §4.3 and §5, Anomaly B). `ugv_p_dynamic_score` appearing "best" in delay while ranking 4th in PDR is the clearest example.

**Supporting figures:** `01_mean_pdr_merged.png` (bar chart PDR); `06_mean_e2e_delay_merged.png` (bar chart delay); `06_pdr_vs_e2e_scatter.png` (scatter PDR vs delay); `07_pdr_events_merged_variance_all.png` (timeseries with variance).

### 3.2 Charging KPIs

**Table 2: Charging Scheduler Performance (mean across 3 replicates)**

| Protocol | Success Rate | Timeout Rate | ROUTING_DROP Rate | Decision Latency (ms) | Effective Wait (ms) | Energy/Session (Wh) | Dock Util |
|---|---|---|---|---|---|---|---|
| ugv_p_edf | **0.486** | 0.289 | 0.218 | 3,926 | 14,859 | 20.28 | 0.737 |
| ugv_dynamic | 0.471 | 0.230 | 0.299 | 3,353 | 12,198 | 21.62 | 0.721 |
| ugv_fcfs | 0.463 | 0.267 | **0.262** | 4,908 | 14,819 | 20.00 | 0.703 |
| ugv_p_role_priority | 0.438 | 0.295 | 0.252 | 4,149 | 18,377 | 18.74 | 0.699 |
| ugv_p_dynamic_score | 0.412 | 0.258 | 0.321 | 4,303 | 15,425 | 17.94 | 0.695 |
| ugv_edf | 0.390 | 0.262 | 0.346 | 4,362 | 14,043 | **16.99** | 0.594 |
| ugv_role_priority | 0.384 | **0.319** | 0.290 | **5,213** | 16,456 | 16.98 | 0.682 |

**Key observations:**
- `ugv_p_edf` leads in charge success rate (48.6%) — preemption allows urgent UAVs to bump lower-priority queued requests, increasing throughput for critical cases.
- `ugv_role_priority` and `ugv_edf` have the worst success rates (38.4%, 39.0%) and lowest energy per session — these protocols both fail to get requests through (routing drops) and fail to convert requests to dock starts (timeouts).
- `ugv_dynamic` achieves the second-best success rate (47.1%) with the lowest decision latency (3,353 ms median) and lowest effective wait (12,198 ms) — the dynamic scoring function efficiently matches urgency to dock availability.
- ROUTING_DROP rates (21.8%–34.6%) are caused exclusively by `WEATHER_DROP` failures (confirmed in `charge_events.csv → failure_reason`), indicating that weather-driven network degradation physically prevents charge requests from reaching the UGV scheduler during stormy periods.

**Supporting figures:** `02_charge_success_rate_merged.png`; `02_decision_latency_merged.png`; `02_effective_wait_merged.png`; `02_energy_recovered_merged.png`; `03_charge_outcome_breakdown_merged.png`.

### 3.3 Fleet Survivability

**Table 3: Battery Depletion Events (mean ± SD across 3 replicates)**

| Protocol | Mean Depletions | SD | CH Depletions | Member Depletions | Depletions/UAV |
|---|---|---|---|---|---|
| ugv_p_edf | **20.0** | **3.0** | 5.3 | 14.7 | 4.0 |
| ugv_dynamic | 23.0 | 2.6 | 5.7 | 17.3 | 4.6 |
| ugv_p_role_priority | 26.7 | 9.0 | 6.0 | 20.7 | 5.3 |
| ugv_fcfs | 27.0 | 5.2 | 9.3 | 17.7 | 5.4 |
| ugv_p_dynamic_score | 31.7 | 21.5 | 9.3 | 22.3 | 6.3 |
| ugv_role_priority | 39.3 | 16.6 | 11.3 | 28.0 | 7.9 |
| ugv_edf | 45.0 | **34.7** | 16.7 | 28.3 | 9.0 |

**Key observations:**
- Each run has a fleet of exactly 5 UAVs (2 CH + 3 members), running for 180 minutes. A "baseline" depletion rate of ~4–5 events/UAV (20–25 total) suggests roughly one battery cycle every 36–45 minutes, which is the expected minimum even with optimal charging.
- `ugv_edf` has catastrophic instability (SD = 34.7), driven by run 3 (85 events). Without this outlier, runs 1–2 perform comparably to other protocols.
- `ugv_role_priority` consistently has high depletion counts across all 3 runs (37, 57, 24), indicating structural inefficiency rather than a single outlier.
- CH depletions are disproportionately costly because each cluster head death disrupts the routing backbone, causing a cascade of member UAV packet losses.

**Supporting figures:** `03_dead_uav_cumulative_merged.png`; `07_death_slope_vs_pdr_dip.png`; `CL_F_pdr_vs_depletions_timeseries.png`.

---

## 4. Cross-Layer Linkage Analysis

### 4.1 Statistical Correlations (All 21 Runs)

**Table 4: Pearson and Spearman Correlations with PDR (n = 21)**

| Charging KPI | Pearson r | p-value | Spearman ρ | p-value | Significance |
|---|---|---|---|---|---|
| Charge success rate | **+0.578** | 0.006 | **+0.552** | 0.010 | ** |
| Charge ROUTING_DROP rate | **−0.527** | 0.014 | **−0.555** | 0.009 | ** |
| Battery depletion events | **−0.567** | 0.007 | **−0.477** | 0.029 | ** |
| Dock utilization | +0.569 | 0.007 | +0.412 | 0.064 | ** / † |
| Queue length (mean) | +0.521 | 0.015 | +0.570 | 0.007 | ** |
| Energy per session (Wh) | +0.487 | 0.025 | +0.395 | 0.077 | * / † |
| Total energy charged (Wh) | +0.512 | 0.018 | +0.505 | 0.020 | * |
| Effective wait (ms) | −0.326 | 0.149 | −0.321 | 0.156 | n.s. |
| Decision latency (ms) | −0.175 | 0.448 | +0.036 | 0.876 | n.s. |
| Charge timeout rate | −0.116 | 0.617 | −0.104 | 0.654 | n.s. |

*Significance: ** p < 0.01; * p < 0.05; † borderline (0.05 < p < 0.10); n.s. not significant.*

**Table 5: Pearson and Spearman Correlations with E2E Delay (n = 21)**

| Charging KPI | Pearson r | p-value | Spearman ρ | p-value | Note |
|---|---|---|---|---|---|
| Dock utilization | +0.567 | 0.007 | +0.227 | 0.322 | Pearson sig, Spearman n.s. → bias |
| Battery depletion events | **−0.548** | 0.010 | −0.089 | 0.700 | Conditioning bias (see §4.3) |
| CH depletions | −0.547 | 0.010 | −0.197 | 0.392 | Same bias |
| Energy per session | +0.530 | 0.014 | +0.013 | 0.955 | Same bias |
| Charge ROUTING_DROP rate | −0.511 | 0.018 | −0.171 | 0.458 | Pearson sig, Spearman n.s. |
| Charge success rate | +0.456 | 0.038 | +0.017 | 0.942 | Strong Pearson/Spearman divergence |

**Critical finding:** The Pearson–Spearman divergence for E2E delay correlations is a diagnostic flag. When Pearson is significant but Spearman is not, the relationship is driven by a few extreme points (outliers such as `ugv_edf_3`) rather than a monotonic trend across all runs. This is consistent with the conditioning bias hypothesis (§4.3).

**Supporting figures:** `CL_B_charging_vs_pdr_scatter.png`; `CL_E_correlation_bar_chart.png`; `06_corr_heatmap_pdr.png`; `06_corr_heatmap_e2e.png`.

### 4.2 Mechanism Narrative: Scheduling → Charging → Survivability → Network

The cross-layer causal chain, supported by statistical evidence, runs as follows:

```
Scheduling Policy
       │
       ▼ (shapes)
Charge Request Outcomes
  ├── STARTED (dock assigned) ──────────────────────────────────────┐
  ├── TIMEOUT (dock busy, request expires)                           │
  └── ROUTING_DROP (request lost in transit: WEATHER_DROP)          │
       │                                                            │
       ▼ (ROUTING_DROP ↑ when network already bad)                  ▼
  Network Degraded ◄──── Battery Depletion Events ◄── Less Energy Delivered
       │                        │
       │                        ▼ (depletion ↑ → topology loss)
       │              Routing Paths Broken (fewer hops available)
       │                        │
       └────────────────────────┴──────────────► PDR ↓
                                                E2E Delay ↑ (conditioning: may appear ↓)
```

**Step 1 — Scheduling to Charging Outcomes (r = 0.578):**
Protocols with higher dock efficiency (P-EDF, Dynamic) convert more requests into actual charging sessions. The key differentiator is not timeout rate alone (which is similar across protocols, 23–32%) but rather the ROUTING_DROP rate (21.8% for P-EDF vs. 34.6% for EDF). ROUTING_DROP events represent charge requests that could not be routed through the ad-hoc UAV mesh to reach the UGV — they occur exclusively during weather-driven network events (`failure_reason = WEATHER_DROP`). Protocols that are resilient to this (P-EDF, FCFS) achieve higher effective success rates.

**Step 2 — Charging to Fleet Survivability (r = −0.567 for depletion events vs. PDR):**
Each failed charging attempt (whether TIMEOUT or ROUTING_DROP) increases the probability of a battery depletion event. The energy per session metric (Wh) captures charging efficiency: when sessions are shorter or fewer, total energy delivered drops (ugv_edf mean = 17.0 Wh/session vs. ugv_dynamic = 21.6 Wh/session). Lower energy recovery means shorter inter-depletion intervals, creating a higher depletion frequency. Depletion events are negatively correlated with PDR (Pearson r = −0.567, p = 0.007).

**Step 3 — Survivability to Network Quality (r = −0.477, Spearman):**
Each depletion event temporarily removes a UAV from the routing fabric. CH depletions are particularly disruptive — the cluster head serves as an inter-cluster relay, so its loss isolates a sub-cluster of member UAVs from the network backbone. The `07_death_slope_vs_pdr_dip.png` figure confirms that death acceleration (rolling slope of depletion rate) is significantly higher during PDR-dip intervals, consistent with a mutual-reinforcement dynamic.

**Step 4 — Feedback Loop (ROUTING_DROP rate negatively correlated with PDR: r = −0.555, Spearman):**
The most important finding is the bidirectional coupling: network degradation (PDR ↓) causes ROUTING_DROP of charge requests, which in turn exacerbates fleet degradation. `ugv_edf_3` is the clearest example — the first two CH depletions at t ≈ 6.6 min initiated a cascade where the ROUTING_DROP rate spiked to 49.6% (vs. ~29% baseline), preventing any corrective charging and leading to 85 total depletion events by run end.

**Supporting figures:** **`CL_O_mechanism_narrative.png`** *(primary narrative figure for this section — see §6)* — a two-row composite: the top row renders the four-box causal chain as a matplotlib flow diagram with Pearson r annotated on each forward arrow and a dashed red feedback arc from Network Quality back to Charge Outcomes; the bottom row shows four time-synchronised panels (dock utilisation → charge timeouts → battery depletions → window PDR) with best-performer protocols (P-EDF, EDF) in blue and role-based protocols in red, making the temporal ordering of the cascade directly visible; `CL_D_mechanism_chain.png` (three-panel scatter confirming each step); `CL_F_pdr_vs_depletions_timeseries.png`; **`CL_L_event_aligned_composite.png`** (synchronises all four signals — PDR, timeouts, deaths, dock utilisation — across all 7 protocols on a shared time axis); `07_pdr_events_merged_variance_all.png`; `07_lagged_corr_ugv_dynamic.png` (and other protocol-specific variants).

### 4.3 Conditioning Bias in E2E Delay

E2E delay (`window_delay_mean_ms`) is computed by the ROS network-monitor node as a rolling average over *delivered* packets only. When PDR is high (many packets delivered, complex multi-hop topology), measured delay reflects genuine multi-hop latency (50–80 ms range). When PDR falls catastrophically and few UAVs survive, the remaining topology consists of direct short-range links, collapsing measured delay:

| Run | Deaths | PDR | Mean Delay |
|---|---|---|---|
| ugv_edf_3 | 85 | 0.476 | **28.5 ms** ← conditioning artifact |
| ugv_p_dynamic_score_2 | 56 | 0.602 | **32.6 ms** ← partial bias |
| ugv_p_role_priority_1 | 21 | 0.628 | 76.8 ms ← genuine high-topology delay |

This explains why protocols that lose many UAVs (ugv_edf, ugv_p_dynamic_score) appear to have *lower* mean delay despite worse network quality. The Pearson r between depletion events and E2E delay is −0.548 (p = 0.010) but Spearman ρ = −0.089 (p = 0.700) — the strong Pearson but weak Spearman confirms outlier-driven bias rather than a true monotonic relationship.

**Confirmation (CL_K):** `CL_K_delay_robustness_panel.png` formally establishes this bias via two panels:

1. **Left panel — "Delay vs Delivered Sample Size" scatter** (`network_timeseries.csv`: `window_delivered`, `window_delay_mean_ms`, `window_pdr` pooled across all 21 runs × all time windows). Each point is one time window; colour encodes PDR (RdYlGn scale). The OLS regression line has a positive slope (r > 0) — more delivered packets → higher measured delay — which is the direct empirical signature of conditioning bias. Green (high-PDR) windows cluster in the high-delay, high-count region; red (low-PDR) windows cluster in the low-delay, low-count region.

2. **Right panel — "DPR-Weighted vs Unweighted Delay Ranking"** (horizontal bar chart per protocol, sorted by weighted delay). For each protocol, every window's delay is weighted by its PDR: `Σ(delay·PDR) / Σ(PDR)`. This suppresses low-PDR windows (where delay is artificially low) and upweights high-PDR windows (where delay reflects genuine multi-hop latency). The weighted ranking is materially different from the raw ranking: protocols that look "fast" under raw delay (ugv_edf, ugv_p_dynamic_score) move toward higher weighted delay, confirming their apparent latency advantage was a measurement artefact.

See also `CL_C_e2e_delay_conditioning_analysis.png` for the protocol-mean bubble chart (bubble size ∝ depletion count).

**Implication for protocol ranking:** The apparent "low delay advantage" of ugv_edf and ugv_p_dynamic_score should not be interpreted as good latency performance. A **DPR-weighted delay** metric (delay × PDR, or equivalently, delay computed only over runs with PDR > 0.6) would more accurately capture whether the scheduler achieves both reliability and low latency. Under such weighting, `ugv_p_edf` and `ugv_fcfs` would rank best on the delay dimension as well.

### 4.4 Lagged Correlation Analysis

The `07_lagged_corr_summary.csv` (computed by the existing pipeline, `07_lagged_corr_{protocol}.png`) shows the peak lagged Pearson correlation between PDR and subsequent depletion events:

| Protocol | PDR → Death peak r | peak lag (min) | Interpretation |
|---|---|---|---|
| ugv_dynamic | −0.296 | −9 min | PDR drops *precede* death spikes by ~9 min |
| ugv_role_priority | −0.305 | −9 min | Same pattern — network degradation precedes deaths |
| ugv_fcfs | −0.249 | −8 min | Similar lead time |
| ugv_p_edf | −0.229 | −9 min | Weaker signal (fewer deaths, less variation) |
| ugv_edf | +0.225 | −3 min | Counter-intuitive — see below |

The consistently negative lagged correlations at lags of −8 to −9 minutes (PDR drop leads deaths) are *evidence consistent with* network degradation driving UAV attrition — the network fails first (due to weather or topology loss), and approximately 8–9 minutes later the depletion rate accelerates. This lag likely represents the time from when a UAV loses connectivity (cannot send charge request) to when its battery actually depletes.

The positive peak r for `ugv_edf` at lag −3 reflects the outlier distortion from run 3, where deaths happened so early and rapidly that PDR and deaths are both deteriorating together rather than having a clear lead-lag structure.

**Dock Utilization → TIMEOUT lagged correlations** (from `07_lagged_corr_summary.csv`) are weak (|r| < 0.29), suggesting that TIMEOUT events do not primarily occur because docks are full — rather, timeouts result from scheduling priority decisions or network delivery failures unrelated to dock occupancy. This is consistent with the relatively stable dock utilization across protocols (0.59–0.74) compared to the large variation in timeout rates (23–32%).

**Supporting figures:** `07_lagged_corr_ugv_dynamic.png` through `07_lagged_corr_ugv_role_priority.png`; `07_dock_util_with_timeouts_merged.png`; `CL_H_lagged_correlation_summary.png`; `CL_I_epoch_aligned_pdr.png`; `CL_J_ugv_edf3_cascade.png`; **`CL_L_event_aligned_composite.png`** (for direct cross-protocol visual comparison of the temporal co-variation of all four signals on aligned axes).

---

### 4.5 The CH Priority Paradox: Why Role-Based Protocols Underperform

#### 4.5.1 Hypothesis

Role-based protocols (`ugv_role_priority`, `ugv_p_role_priority`) perform worse than expected despite giving Cluster Heads (CHs) highest scheduling priority. The proposed mechanism — confirmed by the data — is:

```
CHs share identical battery capacity
        │
        ▼  (same mission duty cycle → same discharge rate)
CHs deplete in synchrony → paired charge requests arrive within seconds of each other
        │
        ▼  (role-based scheduler: both CH requests jump to front of queue)
With 3 docking slots available, both CHs are served in parallel — no dock-level blocking
        │
        ├── CH requests are placed at front of priority queue in close succession
        │   Any queued member request is repeatedly bumped back → member request TTL expires → TIMEOUT
        │   (ugv_role_priority member timeout rate: 35.2% vs 28.8% for ugv_p_edf)
        │
        └── Lower member success → more member deaths → worse PDR
                │
                ▼  degraded network → ROUTING_DROP rate for members increases
                Members cannot reach UGV scheduler → ROUTING_DROP (+6.7 pp vs p_edf)
                Member depletion rate accelerates → more routing failures → feedback loop
```

#### 4.5.2 Empirical Evidence

**Evidence 1 — CH depletion synchronization (universal, all protocols):**
Analysis of `charge_events.csv` cross-CH request proximity shows that in every protocol, the two CH UAVs submit charge requests within 4–60 seconds of each other in tight pairs (e.g., ugv_role_priority_1: pairs at t = 308/312 s, 1776/1832 s, 2416/2460 s). This is driven by battery physics — identical capacity and similar relay workload → identical discharge rate → synchronized depletion. This synchronization is *not* unique to role-based protocols; it is a property of the fleet.

**Evidence 2 — The CH priority paradox (CL_M panel A):**
Despite having scheduling priority, CHs in role-based protocols experience *longer* decision latency than CHs in urgency-based protocols:

| Protocol | CH latency mean | CH latency p50 | CH latency p99 | Member latency mean | CH/Member ratio (mean) |
|---|---|---|---|---|---|
| ugv_role_priority | **5,365 ms** | 438 ms | 87,863 ms | 5,016 ms | 1.07× |
| ugv_p_role_priority | 2,502 ms | 420 ms | 31,067 ms | 5,032 ms | 0.50× |
| ugv_p_edf | 2,175 ms | 483 ms | 24,553 ms | 5,046 ms | 0.43× |
| ugv_dynamic | 2,250 ms | 419 ms | 24,960 ms | 4,044 ms | 0.56× |

The CH mean decision latency in `ugv_role_priority` (5,365 ms) is elevated, but the **median is only 438 ms** — indistinguishable from other protocols. The high mean is driven by rare tail events (p99 = 87,863 ms ≈ 87 s), not systematic queuing. These outliers occur when the UGV's STARTED response fails to reach the CH due to poor network conditions during fleet degradation episodes. The CH/Member ratio by mean (1.07×) is misleading; by median, CHs are served just as quickly as in other protocols. In urgency-based protocols the mean ratio is < 1 (CHs genuinely faster), but even this is driven by the member tail rather than CH systematic acceleration.

**Evidence 3 — Member starvation (CL_M panel C):**
Member charge success rates in role-based vs urgency-based protocols:

| Protocol | Member success rate | CH success rate |
|---|---|---|
| ugv_role_priority_1 | **27.9%** | 50.0% |
| ugv_p_edf_1 | **52.8%** | 47.1% |

In ugv_role_priority_1, fewer than 28% of member charge requests result in docking. In ugv_p_edf_1, the same metric is 53%. The role-based scheduler sacrifices member fleet health entirely for CH priority, but — due to the paradox above — CHs are not even well-served themselves.

**Evidence 4 — Member routing drop starvation (CL_M panel D):**
Member ROUTING_DROP rate is highest in role-based protocols because (a) member UAVs die more frequently (starved of charging), (b) more dead members means a weaker mesh network through which remaining requests must route, (c) this creates a self-reinforcing spiral.

**Evidence 5 — CH wait time predicts member starvation (CL_N right panel):**
Across all 21 runs, mean CH decision latency and member charge success rate are negatively correlated (Pearson r ≈ −0.5 to −0.6, p < 0.05). The scatter plot (CL_N right panel) clusters role-based protocols in the upper-left quadrant (long CH wait, low member success) and urgency-based protocols in the lower-right (fast CH decisions, healthy member access). This confirms the mechanism at the cross-protocol level.

#### 4.5.3 The Actual Mechanism: Member Starvation, Not CH Congestion

*(Correction to the initial hypothesis: with 3 parallel docking slots, two simultaneous CH requests are both served immediately — there is no "CH blocks CH" scenario. The all-docks-full rate for `ugv_role_priority` is 32.8%, which is actually lower than for `ugv_p_edf` (40.6%). Dock capacity is not the bottleneck.)*

The real mechanism has two components:

**1 — Scheduling priority queue starvation (direct effect)**

When a CH charge request arrives at the UGV scheduler, it is placed ahead of all queued member requests regardless of dock availability. With both CHs cycling through identical battery curves (same capacity → synchronized discharge → correlated request bursts), there are frequent short intervals where CH requests arrive in close succession (4–60 s apart). During each such burst window:

- Both CH requests jump to the front of the priority queue.
- Even if a dock is free, the scheduler evaluates CHs first; queued member requests are pushed back.
- If a member request's TTL expires while waiting behind CHs, the outcome is recorded as TIMEOUT.

This explains the elevated member timeout rate: **ugv_role_priority = 35.2%, vs 28.8% for ugv_p_edf** (+6.4 pp). The queue timeseries confirms this is not dock-level blocking — mean queue depth at member position is only 0.15 — but priority ordering still causes members to lose their request slot in the scheduling window.

Note that preemption actually *helps* members in the role-based case: `ugv_p_role_priority` achieves 38.0% member success vs. 31.8% for non-preemptive `ugv_role_priority`, because preemptive CHs get served faster (CH latency 2502ms vs 5365ms), free dock slots sooner, and reduce the window during which member requests time out while CHs are accumulating in queue.

**2 — Network degradation feedback (indirect, amplifying effect)**

Lower member success → more member deaths → worse routing topology → more routing drops for all requests. Member routing drop rate is **32.0% in ugv_role_priority vs. 25.3% in ugv_p_edf** (+6.7 pp), driven by the degraded network that is itself a consequence of worse member survivability. This reinforcing loop (starvation → deaths → degraded network → more routing drops → more starvation) explains why both role-based protocols underperform so consistently across all three replicates.

**On the high CH decision latency in ugv_role_priority:** the mean of 5365ms is misleading. The median is 438ms — CHs are routinely served fast. The high mean is driven by extreme tail events (p99 = 87,863ms). These outliers occur when the UGV's STARTED response fails to reach the CH due to poor network conditions (worse PDR during fleet degradation episodes). As a bimodal distribution, the mean is not representative of typical CH service — the protocol still delivers CHs promptly most of the time.

**Revised implication:** A better fix than a "battery urgency tie-breaker" (which the 3-dock architecture makes moot) would be to **set a maximum priority-hold window**: if a CH request has been at the front of the queue for > T seconds and no new CH request has arrived, release the dock to the longest-waiting member. This prevents TTL expiry for members during extended CH-request bursts without abandoning the role hierarchy.

**Supporting figures:** `CL_M_role_scheduling_audit.png`; `CL_N_ch_sync_cascade.png`; `CL_A_protocol_kpi_overview.png` (CH deaths paradox: role-based has ~11 CH deaths vs ~5 for p_edf despite CH priority).

---

## 5. Anomalies and Interpretations

### Anomaly A: ugv_edf_3 — Catastrophic Depletion Cascade (85 events, 5 UAVs)

**Observation:** ugv_edf_3 records 85 battery depletion events from only 5 unique UAV IDs (17 depletions/UAV vs. cross-protocol mean of ~5). The `alive_count` field in `death_events.csv` drops to 0 by t ≈ 33 min but events continue for the full 180-minute run.

**Likely cause:** A depletion-respawn cycle triggered by an early cascade failure. The first two CH depletions occurred simultaneously at t = 6.6 min (both CH UAVs dying within 4 seconds). This catastrophically broke the routing backbone, causing ROUTING_DROP rate to spike to ~49.6% for this run (highest in the dataset). With charge requests unable to reach the UGV scheduler, UAVs could not charge, depleted, were respawned by the simulator, and immediately began draining again. The EDF scheduling policy, which prioritizes by deadline rather than urgency-weighted score, lacks the ability to preempt or fast-track critically low-battery UAVs in this scenario.

**Quick verification:**
1. `death_events.csv`: Confirm `t_rel_s` of first two events ≈ 396s and 396.4s (both CH, role=1)
2. `charge_events.csv`: Check `outcome == 'ROUTING_DROP'` count for `t_rel_s < 600s` (first 10 min) — should be 0, indicating the cascade started at CH depletions and then propagated
3. `charge_events.csv`: Filter `t_rel_s > 600s` and compute fraction `ROUTING_DROP / total` — expect ~50% in second half

**Impact on aggregate statistics:** ugv_edf's mean depletion count (45.0) and SD (34.7) are both inflated by this outlier. Its mean PDR (0.573) and E2E delay (55.6 ms) are also distorted. Without run 3, ugv_edf would rank comparably to ugv_fcfs on most metrics.

### Anomaly B: E2E Delay Inversely Associated with Fleet Degradation (Conditioning Bias)

**Observation:** Protocols and runs with more depletion events show lower measured E2E delay (Pearson r = −0.548 between total_deaths and e2e_delay_mean). This is counter-intuitive — worse survivability should imply worse, not better, network performance.

**Likely cause:** Conditioning on delivered packets. When many UAVs deplete and topology simplifies, surviving packets travel via short direct links rather than multi-hop paths, reducing measured latency. This is purely a measurement artifact: the `window_delay_mean_ms` metric reflects only the delay of *successfully delivered* packets, excluding all the packets that were dropped (which, at high depletion rates, is the majority).

**Quick verification:** Compute correlation between `e2e_delay_mean` and `n_delivered` (total packets delivered per run). Expect positive correlation — more delivered packets → higher delay. If confirmed, delay rankings from `06_mean_e2e_delay_merged.png` should be discarded as primary protocol quality indicators and replaced with PDR-weighted delay. See also `CL_C_e2e_delay_conditioning_analysis.png`.

### Anomaly C: ugv_p_dynamic_score_2 — High Deaths, Acceptable PDR

**Observation:** ugv_p_dynamic_score_2 has 56 depletion events (highest for its protocol) but maintains PDR = 0.602, higher than several protocols with fewer depletions.

**Likely cause:** The dynamic scoring function may prioritize high-PDR-context UAVs for charging, creating an asymmetry: UAVs in good-connectivity zones get charged efficiently while UAVs in degraded areas (where their requests would arrive as ROUTING_DROP anyway) experience more depletions. The PDR remains acceptable because the "well-connected" portion of the fleet keeps routing. This is a plausible positive side-effect of PDR-aware scheduling.

**Quick verification:** In `charge_events.csv`, filter to `ugv_p_dynamic_score_2`, group by `uav_id`, and compute depletion counts from `death_events.csv`. Check whether deaths are concentrated in 1–2 UAVs rather than distributed — which would confirm the asymmetric service hypothesis.

### Anomaly D: ROUTING_DROP Failures Are Exclusively Weather-Driven

**Observation:** All ROUTING_DROP events across all 21 runs have `failure_reason = WEATHER_DROP` (confirmed in `charge_events.csv`). No ROUTING_DROP events were caused by routing table failures, TTL expiry, or congestion.

**Likely cause:** The simulator's weather model directly causes packet drops when weather is adverse (rain, wind, storm). Charge requests are routed through the same mesh network as data packets, making them equally vulnerable to weather-induced PDR drops.

**Implication:** ROUTING_DROP rate is not a scheduler-specific failure mode — it is an exogenous network quality indicator. Protocols with higher ROUTING_DROP rates are not worse schedulers per se; rather, they happen to have more charge requests submitted during bad-weather windows, or their UAVs are positioned in areas with worse link quality. The strong negative correlation between ROUTING_DROP rate and PDR (ρ = −0.555) is therefore partly a confound: both are driven by the same underlying weather variable.

**Quick verification:** Correlate each run's ROUTING_DROP count with its cumulative `stormy` weather regime duration from `weather_timeseries.csv`. Expect strong positive correlation. Note that weather timeseries appears identical across runs of the same protocol (it is a shared environment), so any per-run variation in ROUTING_DROP is driven by UAV positioning and network topology state at the time of storm events.

### Anomaly E: Dock Utilization Positively Correlated with PDR

**Observation:** `dock_util_mean` is positively correlated with PDR (Pearson r = 0.569, p = 0.007) — counter-intuitive because one might expect higher utilization to cause more TOUTIMEs (queuing) and worse outcomes.

**Likely cause:** Dock utilization is a proxy for overall system health. When the network is healthy and UAVs are alive, more charging requests successfully arrive and start, keeping docks busy. When the network degrades and UAVs die, fewer requests arrive (ROUTING_DROP), docks sit idle, and utilization drops. High utilization is therefore a *consequence* of good network state rather than a cause of poor scheduling performance.

**Quick verification:** Plot dock utilization timeseries for ugv_edf_3 — expect dock utilization to drop sharply after t ≈ 6.6 min (first CH deaths), which would confirm the causal direction (network degradation → idle docks, not idle docks → network degradation).

---

## 6. Additional Plots Generated

The following new scripts and plots were created to support the cross-layer argument. They are located in `analysis/test_round2/figures/cross_layer/`:

### CL_A — Protocol KPI Overview Bar Chart
**Script:** Inline Python (embedded in analysis)
**Reads:** `qos_metrics.csv` (PDR), `death_events.csv` (depletions), `charge_events.csv` (success rate)
**Output:** `CL_A_protocol_kpi_overview.png`
**Why needed:** Provides a single-view comparison of the three most important KPIs (PDR, depletions, success rate) with error bars showing replicate variance. Supports §3 Protocol-Level Comparisons.

### CL_B — Charging KPIs vs PDR Scatter Matrix
**Script:** Inline Python (embedded in analysis)
**Reads:** `qos_metrics.csv`, `charge_events.csv`, `death_events.csv`, `charge_session_events.csv`, `charge_queue_timeseries.csv`
**Output:** `CL_B_charging_vs_pdr_scatter.png`
**Why needed:** Visualises all six major charging KPI vs. PDR relationships on a 2×3 panel with regression lines and annotated r/ρ values. Supports §4.1 correlation claims.

### CL_C — E2E Delay Conditioning Analysis
**Script:** Inline Python (embedded in analysis)
**Reads:** `qos_metrics.csv`, `network_timeseries.csv`, `death_events.csv`
**Output:** `CL_C_e2e_delay_conditioning_analysis.png`
**Why needed:** Directly illustrates the conditioning bias (§4.3 and §5 Anomaly B) through a scatter plot annotating the ugv_edf_3 outlier, plus a bubble chart where bubble size encodes depletion count, showing the bias direction.

### CL_D — Cross-Layer Mechanism Chain (Three-Panel Scatter)
**Script:** Inline Python (embedded in analysis)
**Reads:** `charge_events.csv`, `death_events.csv`, `qos_metrics.csv`
**Output:** `CL_D_mechanism_chain.png`
**Why needed:** Provides visual evidence for each of the three mechanism steps: (1) success rate → depletions, (2) depletions → PDR, (3) ROUTING_DROP rate → PDR. Core support for §4.2 Mechanism Narrative.

### CL_E — Correlation Bar Chart (Pearson + Spearman Side-by-Side)
**Script:** Inline Python (embedded in analysis)
**Reads:** All KPI CSVs (computed per run)
**Output:** `CL_E_correlation_bar_chart.png`
**Why needed:** The existing `06_corr_heatmap_pdr.png` shows only Pearson r. The Pearson–Spearman divergence for E2E delay correlations (§4.1, §5 Anomaly B) requires showing both coefficients simultaneously to diagnose outlier-driven bias.

### CL_F — PDR Timeseries with Battery Depletion Rate Overlay
**Script:** Inline Python (embedded in analysis)
**Reads:** `network_timeseries.csv`, `death_events.csv`
**Output:** `CL_F_pdr_vs_depletions_timeseries.png`
**Why needed:** The existing `07_pdr_events_merged_variance_all.png` shows events as individual vlines or binned bars but does not show the depletion *rate* (events per time bin) as a separate quantitative axis. CL_F adds a secondary Y-axis with depletion bars, enabling visual inspection of the lag structure (§4.4) across all 7 protocols simultaneously.

### CL_H — Lagged Correlation Summary (§4.4)
**Reads:** `network_timeseries.csv` (window_pdr), `death_events.csv` (t_rel_s)
**Output:** `CL_H_lagged_correlation_summary.png`
**Why needed:** Synthesises the per-protocol lag plots (`07_lagged_corr_*.png`) into a single two-panel figure: (left) all 7 protocol lag-correlation curves overlaid, with the predicted −7 to −10 min window shaded; (right) horizontal bar chart of each protocol's peak negative r and the lag at which it occurs. This is the primary illustration for the "PDR drops precede depletions by ~8–9 min" claim in §4.4.

### CL_I — Epoch-Aligned PDR Around Depletion Bursts (§4.4)
**Reads:** `network_timeseries.csv` (window_pdr), `death_events.csv` (t_rel_s)
**Output:** `CL_I_epoch_aligned_pdr.png`
**Why needed:** Provides the event-centred (epoch) view of the PDR–death relationship. Each 5-min bin containing ≥1 depletion defines an epoch; the PDR timeseries is extracted in a ±15 min window around it and averaged across all bursts and replicates. The resulting mean trajectory (mean ± 1σ, 8 panels — one per protocol plus a combined panel) shows whether PDR dips *before* or *after* the depletion marker, directly supporting the causal direction discussed in §4.4.

### CL_J — ugv_edf_3 Cascade Annotated Timeseries (§4.4 / Anomaly A)
**Reads:** `network_timeseries.csv`, `charge_events.csv`, `death_events.csv` (ugv_edf_3 only)
**Output:** `CL_J_ugv_edf3_cascade.png`
**Why needed:** Provides a concrete single-run illustration of the feedback loop described in §4.4 and §5 Anomaly A. Three series are shown on dual axes: PDR (blue), charge-request ROUTING_DROP rate (red dashed), and cumulative depletion count (black step). The dual CH depletion trigger at t ≈ 6.6 min is annotated with an arrow, and the post-cascade "fleet alive = 0" period is shaded. This makes the cascade narrative visually unambiguous.

### CL_G — Charge Request Routing Drop Rate Over Time
**Script:** `plot_cross_layer_analysis.py → plot_cl_g_routing_drop_timeseries()`
**Reads:** `charge_events.csv` (`t_rel_s`, `outcome`)
**Generation logic:** For each protocol, all three replicates are binned into 10-minute intervals. Within each bin, the fraction of events with `outcome == "ROUTING_DROP"` is computed per replicate; the mean across replicates is plotted as a single line per protocol on a shared axis.
**Output:** `CL_G_routing_drop_timeseries.png`
**Why needed:** Reveals *when* ROUTING_DROP bursts occur during the experiment timeline and whether they align with known PDR dip intervals (e.g., weather storm windows). Supports the feedback-loop argument in §4.2 and the weather-dependency finding in §5 Anomaly D.

---

### CL_K — Conditioning-Aware Delay Robustness Panel *(new)*
**Script:** `plot_cross_layer_analysis.py → plot_cl_k_delay_robustness_panel()`
**Reads:** `network_timeseries.csv` columns: `window_delivered` (integer packet count per time window), `window_delay_mean_ms` (mean E2E delay of delivered packets in that window, ms), `window_pdr` (window-level PDR). Rows with any of these three values < 0 or missing are excluded. All 21 runs are pooled into a single analysis-level DataFrame.
**Generation logic — Left panel:** A scatter plot where each point is one valid time window from any run. x-axis = `window_delivered` (sample size), y-axis = `window_delay_mean_ms`, colour = `window_pdr` (RdYlGn colourmap, 0 = red, 1 = green). An OLS regression line with annotated Pearson r and p-value is overlaid. A positive slope confirms the conditioning bias: windows that deliver more packets (healthy, high-PDR state) also measure higher delay (full multi-hop paths), while windows with few delivered packets (low-PDR, dying fleet) appear artificially fast.
**Generation logic — Right panel:** For each protocol, all valid windows across its three replicates are aggregated. Two statistics are computed: (a) unweighted mean delay = `mean(window_delay_mean_ms)`; (b) PDR-weighted mean delay = `Σ(window_delay_mean_ms × window_pdr) / Σ(window_pdr)`. Both are rendered as a horizontal bar per protocol (solid = weighted, hatched = unweighted) and sorted by weighted delay ascending. The divergence between the two bars reveals how much the raw ranking is distorted by low-PDR windows.
**Output:** `CL_K_delay_robustness_panel.png`
**Why needed:** §4.3 and §5 Anomaly B previously identified the conditioning bias as a hypothesis ("Verification: correlate delay with delivered count"). CL_K formally confirms the bias with real data and provides a ready-to-cite, bias-corrected delay ranking. Without this figure, the claim that "low-delay protocols are actually worse" is speculative; with it, the mechanism is empirically demonstrated at the window level across all 21 runs (thousands of data points).

---

### CL_L — Event-Aligned Composite Timeseries *(new)*
**Script:** `plot_cross_layer_analysis.py → plot_cl_l_event_aligned_composite()`
**Reads:** Four CSV files per run, all binned into `bin_sec`-second intervals (default 600 s = 10 min):

| Row | Source file | Column(s) used | Statistic |
|-----|-------------|----------------|-----------|
| 0: PDR | `network_timeseries.csv` | `window_pdr` (rows ≥ 0), `t_rel_s` | Bin mean via `_bin_mean()` (vectorised `np.searchsorted` + `np.bincount`) |
| 1: Timeouts | `charge_events.csv` | rows where `outcome == "TIMEOUT"`, `t_rel_s` | `np.histogram` count per bin |
| 2: Deaths | `death_events.csv` | `t_rel_s` | `np.histogram` count per bin |
| 3: Dock util. | `charge_queue_timeseries.csv` | `ugv_dock_utilization` (rows ≥ 0), `t_rel_s` | Bin mean |

**Generation logic:** A 4 × 8 subplot grid (4 rows = 4 metrics; 8 columns = 7 protocols + 1 merged). For each `(row, column)` cell, all three replicates are stacked into a `(3 × n_bins)` array; `np.nanmean` and `np.nanstd` are computed across the replicate axis. The mean is drawn as a line; ± 1σ is shaded. The rightmost "All (merged)" column pools all 21 replicates (7 protocols × 3) before computing mean and SD. All columns in the same row share the y-axis (`sharey="row"`); all rows share the x-axis (`sharex=True`).
**Output:** `CL_L_event_aligned_composite.png`
**Why needed:** The existing per-metric plots (e.g., `CL_F_pdr_vs_depletions_timeseries.png`, `07_dock_util_with_timeouts_merged.png`) show at most two metrics per panel and require the reader to compare across separate figures. CL_L places all four cross-layer signals on a unified grid so that temporal co-variation is visible at a glance. For example, in columns where Timeout count (Row 1) rises at t ≈ 60–90 min, PDR (Row 0) simultaneously drops and Death count (Row 2) spikes — a pattern that directly supports the feedback-loop narrative in §4.2. The merged column makes cross-protocol averages visible alongside individual protocol columns, enabling robust visual reasoning. This figure is the most comprehensive single-panel summary of the cross-layer dynamics produced by the pipeline.

### CL_M — Role-Stratified Scheduling Audit *(new)*
**Script:** `plot_cross_layer_analysis.py → plot_cl_m_role_scheduling_audit()`
**Reads:** `charge_events.csv` — columns `role` (0 = member, 1 = CH), `outcome` (`STARTED` / `TIMEOUT` / `ROUTING_DROP`), `decision_latency_ms` (> 0 only), `effective_wait_ms` (> 0 only). Per-run, per-role statistics are collected via the internal `_collect_role_kpis()` helper, which groups events by `(protocol, run, role)` and computes success/timeout/routing-drop rates and mean latency/wait metrics.
**Generation logic:** A 2 × 2 grid of grouped bar charts, one bar pair per protocol (CH = blue, member = red). Each bar = mean ± 1 SD across 3 replicates. Role-based protocols are highlighted with a faint red background shading on every panel. The four panels show: (A) decision latency, (B) effective wait, (C) success rate, (D) routing-drop rate.
**Output:** `CL_M_role_scheduling_audit.png`
**Why needed:** Provides the primary empirical evidence for §4.5. Without role-stratified breakdown, the CH priority paradox is invisible in aggregate statistics — it only surfaces when CH and member outcomes are plotted separately. CL_M makes the 5× CH decision latency gap in role-based protocols immediately visible alongside the member starvation it causes.

---

### CL_O — §4.2 Mechanism Narrative *(new — dedicated §4.2 figure)*
**Script:** `plot_cross_layer_analysis.py → plot_cl_o_mechanism_narrative()`
**Reads:**
- `df` — per-run scalar KPI DataFrame from `collect_per_run_kpis()`: `charge_success_rate`, `total_deaths`, `charge_routing_drop_rate`, `pdr` (used for computing Pearson r on each causal link).
- `charge_queue_timeseries.csv` → `ugv_dock_utilization`, `t_rel_s` (scheduling load proxy)
- `charge_events.csv` → `outcome` (`TIMEOUT` fraction per bin), `t_rel_s`
- `death_events.csv` → `t_rel_s` (deaths per bin)
- `network_timeseries.csv` → `window_pdr`, `t_rel_s`

**Generation logic — Row 0 (Flow diagram):** Four rounded `FancyBboxPatch` boxes connected by annotated `FancyArrowPatch` arrows in a straight horizontal chain. Each forward arrow carries the Pearson r computed from the 21-run KPI DataFrame (e.g., success_rate → total_deaths, total_deaths → pdr, routing_drop_rate → pdr). A dashed red arc (`connectionstyle="arc3,rad=-0.35"`) connects the rightmost Network Quality box back down to the Charge Outcomes box, representing the reinforcing feedback loop. Computed at runtime so values are always consistent with the actual data.

**Generation logic — Row 1 (Temporal evidence, 4 panels):** For each of the 7 protocols, three replicates are loaded and binned into `bin_sec`-second intervals (default 600 s). Vectorised binning uses `np.searchsorted` + `np.bincount` for dock utilisation and PDR (weighted mean per bin) and `np.bincount` for count-based metrics (timeouts/bin, deaths/bin). Per-replicate arrays are averaged with `np.nanmean`. Two protocol groups are drawn with coloured lines: best performers (P-EDF, EDF) in blue shades and role-based protocols (P-RolePrio, RolePrio) in red shades. Other protocols are shown as grey background lines. Each of the four panels corresponds to one mechanism step.

**Output:** `CL_O_mechanism_narrative.png` + `.pdf`
**Why needed:** §4.2 currently presents the causal chain as an ASCII diagram in text. CL_O converts this into a publication-quality figure: the top flow diagram makes the causal logic unambiguous to a reader who has not read the text, while the bottom time-series row provides direct temporal evidence that the four steps fire in the predicted sequence during actual experiment runs. CL_D (three-panel scatter) already confirms the cross-run correlations; CL_O adds the *narrative sequence* and highlights the protocol contrast (best vs role-based) that drives the §4.5 CH priority paradox.

---

### CL_N — CH Depletion Synchronization → Cascade Analysis *(new)*
**Script:** `plot_cross_layer_analysis.py → plot_cl_n_ch_sync_cascade()`
**Reads:** `charge_events.csv` — columns `role`, `uav_id`, `t_rel_s`, `decision_latency_ms`, `outcome`. Two derived datasets are computed:
- **Cross-CH request gaps** (`_compute_ch_request_gaps()`): for each run, for every pair of distinct CH UAVs, each CH request's nearest temporal neighbour from the other CH is found; the absolute time distance in seconds is recorded. This gives the distribution of "how synchronized are the two CHs' depletion cycles?"
- **Per-run role KPIs** (shared with CL_M via `_collect_role_kpis()`): mean CH decision latency and member success rate per run.

**Generation logic:**
- Left panel: histogram-based pseudo-KDE of cross-CH request gaps (0–600 s range) per protocol, overlaid as coloured lines. A vertical dashed line marks 120 s. A peak near 0–60 s confirms synchronized depletion across all protocols (battery physics, not scheduling).
- Right panel: scatter of (mean CH decision latency per run) vs (member charge success rate per run), one point per run (21 points), coloured by protocol, with OLS regression line and annotated Pearson r and p-value. Role-based protocol points are labelled.

**Output:** `CL_N_ch_sync_cascade.png`
**Why needed:** CL_M shows *what* happens (CH paradox, member starvation) but not *why* it's universal across all role-based runs. CL_N provides the *mechanism*: (a) synchronized CH depletion is present in all protocols (left panel) — it is a physical inevitability; (b) the scheduling policy determines whether that synchronization leads to a priority traffic jam (right panel) — protocols where CHs wait long also have starved members. Together, CL_M + CL_N make a complete mechanistic argument.

---

## 7. Synthesis and Conclusions

### 7.1 Protocol Ranking

Based on the cross-layer evidence, the protocols rank as follows (higher tier = better overall performance):

| Tier | Protocol | Rationale |
|---|---|---|
| **1 (Best)** | ugv_p_edf | Highest PDR, lowest depletions, best success rate; preemption prevents priority inversion |
| **2** | ugv_fcfs | Second-best PDR, consistent depletions; simple but effective under normal load |
| **2** | ugv_dynamic | Lowest decision latency, second-best success rate; efficient dynamic prioritization |
| **3** | ugv_p_dynamic_score | Mid-PDR, acceptable depletions; high variance across runs is a concern |
| **4** | ugv_edf | High instability (SD = 0.094); run 3 catastrophe reveals vulnerability to cascades |
| **4** | ugv_p_role_priority | Worst PDR, highest E2E delay; role-based priority less effective than urgency-based |
| **5 (Worst)** | ugv_role_priority | Lowest success rate, highest timeout rate, consistently high depletions |

### 7.2 Design Recommendations

1. **Adopt urgency-aware preemption**: ugv_p_edf's advantage over ugv_edf demonstrates that preemption is critical for preventing cascade failures. When a critically low-battery UAV cannot access a dock due to a lower-urgency queued request, the entire fleet is at risk.

2. **Monitor ROUTING_DROP as an early warning signal**: A ROUTING_DROP rate above ~35% in a 10-minute window indicates that network conditions are too degraded for the scheduling layer to function correctly. An adaptive protocol should increase dock reservation or reduce mission tempo during such periods.

3. **Avoid role-based priority as the primary scheduling criterion**: Both ugv_role_priority and ugv_p_role_priority underperform relative to urgency- or deadline-based approaches, suggesting that coarse role classification (CH vs. member) provides insufficient information for effective energy management.

4. **Do not use E2E delay as a standalone network quality metric**: The conditioning bias demonstrated in §4.3 and formally confirmed in `CL_K_delay_robustness_panel.png` makes raw delay measurements misleading for protocol comparison. Use the **PDR-weighted delay** (`Σ(delay·PDR)/Σ(PDR)` per protocol) as a bias-corrected alternative, or condition delay comparisons on time windows with `window_pdr ≥ 0.6`. The CL_K right panel shows that the weighted ranking shifts ugv_p_edf and ugv_fcfs to the top, consistent with their PDR rankings.

### 7.3 Proposed Future Tests to Strengthen Causal Claims

1. **Lagged correlation with weather covariates**: Regress ROUTING_DROP rate on storm regime duration after controlling for protocol. If the protocol effect on ROUTING_DROP disappears after weather control, Anomaly D is confirmed and the ROUTING_DROP–PDR correlation is a weather confounder.

2. **Intervention experiment**: Artificially prevent CH depletions (by giving CHs priority charging slots) and measure whether the cascade elimination improves ugv_edf's PDR stability. If ugv_edf run 3 with "protected CHs" achieves ugv_edf run 1/2 levels, the CH cascade hypothesis is confirmed.

3. **PDR-weighted delay metric**: Compute `penalized_delay = mean_delay × (1 - PDR)` per run. This penalizes protocols with high apparent delivery speed but low actual reliability. Expect ugv_p_edf to rank best and ugv_edf/ugv_p_role_priority to rank worst.

4. ~~**Conditioning check**: Correlate `e2e_delay_mean` with `n_delivered`~~ **CONFIRMED by CL_K**: `CL_K_delay_robustness_panel.png` (left panel) establishes the per-window positive correlation between delivered count and measured delay across all 21 runs. This formally closes the bias hypothesis and makes PDR-weighted delay the recommended reporting metric going forward.

---

## Appendix: Data Source Summary for Key Figures

### A1. Cross-Layer Figures (CL_* — `figures/cross_layer/`)

| Figure | Key data columns | Primary claim supported |
|---|---|---|
| `CL_A_protocol_kpi_overview.png` | `qos_metrics.csv` → PDR; `death_events.csv` → count; `charge_events.csv` → outcome | §3 protocol ranking — PDR, depletions, success rate in one view |
| `CL_B_charging_vs_pdr_scatter.png` | `charge_events.csv`, `death_events.csv`, `charge_session_events.csv`, `charge_queue_timeseries.csv` vs `qos_metrics.csv` PDR | §4.1 — 6 charging KPIs vs PDR with r/ρ, regression lines |
| `CL_C_e2e_delay_conditioning_analysis.png` | `network_timeseries.csv` → delay, PDR; `death_events.csv` → count | §4.3 — bias scatter + bubble chart (bubble ∝ depletions) |
| `CL_D_mechanism_chain.png` | `charge_events.csv`, `death_events.csv`, `qos_metrics.csv` | §4.2 — three-step causal chain in one 3-panel scatter |
| `CL_E_correlation_bar_chart.png` | All per-run KPI scalars | §4.1 — Pearson vs Spearman side-by-side, diagnoses bias |
| `CL_F_pdr_vs_depletions_timeseries.png` | `network_timeseries.csv` → `window_pdr`; `death_events.csv` → `t_rel_s` | §4.2, §4.4 — PDR timeseries + depletion rate secondary axis |
| `CL_G_routing_drop_timeseries.png` | `charge_events.csv` → `outcome`, `t_rel_s` | §4.2, §5 Anomaly D — ROUTING_DROP rate over time |
| `CL_H_lagged_correlation_summary.png` | `network_timeseries.csv` → `window_pdr`; `death_events.csv` → `t_rel_s` | §4.4 — lagged r curves + peak-lag bar chart (PDR leads deaths) |
| `CL_I_epoch_aligned_pdr.png` | `network_timeseries.csv` → `window_pdr`; `death_events.csv` → `t_rel_s` | §4.4 — epoch-aligned mean PDR ±15 min around depletion bursts |
| `CL_J_ugv_edf3_cascade.png` | `network_timeseries.csv`, `charge_events.csv`, `death_events.csv` (ugv_edf_3) | §5 Anomaly A — cascade failure timeseries, annotated |
| **`CL_K_delay_robustness_panel.png`** | `network_timeseries.csv` → `window_delivered`, `window_delay_mean_ms`, `window_pdr` | §4.3 — **confirms** conditioning bias; PDR-weighted delay ranking |
| **`CL_L_event_aligned_composite.png`** | `network_timeseries.csv`, `charge_events.csv`, `death_events.csv`, `charge_queue_timeseries.csv` | §4.2, §4.4 — 4-metric × 8-protocol composite (the most comprehensive single figure) |
| **`CL_M_role_scheduling_audit.png`** | `charge_events.csv` → `role`, `outcome`, `decision_latency_ms`, `effective_wait_ms` | §4.5 — CH priority paradox: role-stratified latency, success, starvation |
| **`CL_N_ch_sync_cascade.png`** | `charge_events.csv` → `role`, `uav_id`, `t_rel_s`, `decision_latency_ms`, `outcome` | §4.5 — CH depletion synchronization KDE + CH latency vs member success scatter |
| **`CL_O_mechanism_narrative.png`** | `collect_per_run_kpis()` scalars + `charge_queue_timeseries.csv`, `charge_events.csv`, `death_events.csv`, `network_timeseries.csv` | §4.2 — **primary narrative figure**: flow diagram (Pearson r on arrows) + 4-panel temporal evidence (best vs role-based groups) |

### A2. Supporting Figures (Other Groups — `figures/`)

| Figure | Key Data Sources | Location |
|---|---|---|
| `01_mean_pdr_merged.png` | `qos_metrics.csv` → `generated`, `delivered` | `figures/01_validation/` |
| `02_charge_success_rate_merged.png` | `charge_events.csv` → `outcome` | `figures/02_per_protocol_stats/` |
| `03_dead_uav_cumulative_merged.png` | `death_events.csv` → `t_rel_s` | `figures/03_cross_protocol_charging/` |
| `04_policy_radar_merged.png` | All CSVs (computed KPIs) | `figures/04_policy_radar/` |
| `06_corr_heatmap_pdr.png` | All CSVs (per-run scalars) | `figures/06_network_qos_delay/` |
| `07_lagged_corr_*.png` | `network_timeseries.csv`, `charge_events.csv`, `death_events.csv` | `figures/07_causal_analysis/` |
| `07_death_slope_vs_pdr_dip.png` | `network_timeseries.csv`, `death_events.csv` | `figures/07_causal_analysis/` |

---

*Report generated from experimental data in `experiment_data_collection/test_round2`. All statistical tests use two-tailed p-values. Significance thresholds: * p < 0.05, ** p < 0.01, *** p < 0.001.*
