# Cross-layer analysis of charging protocol effects on network quality (Test Round 2)

- **Dataset root:** `experiment_data_collection/test_round2`
- **Figure root:** `analysis/test_round2/figures`
- **Computed tables for this report:**
  - `analysis/test_round2/figures/summary_tables/cross_layer_run_metrics.csv`
  - `analysis/test_round2/figures/summary_tables/cross_layer_protocol_summary.csv`
  - `analysis/test_round2/figures/summary_tables/cross_layer_correlations.csv`

## 0) Startup checklist results

1. **Loaded plot reference first:** `docs/analysis_plots_reference.md` and used it as the mapping from figures to source tables.
2. **Verified completeness:** detected **7 protocols × 3 replicates = 21 runs** under `experiment_data_collection/test_round2`.
3. **Generated merged replicate summaries:** see `cross_layer_protocol_summary.csv` and `cross_layer_run_metrics.csv`.
4. **Compared conclusions to prior `docs/test_round2_cross_layer_analysis.md`:**
   - Broad ranking agreement exists (e.g., `ugv_p_edf` strongest DPR, `ugv_role_priority` weak charging outcomes).
   - This revised report is more conservative where the prior report used significance p-values and `CL_*` figures not present in `analysis/test_round2/figures`.
   - Therefore, this version is more reproducible against the current repository state.

---

## 1) Executive summary (key findings)

- **`ugv_p_edf` is the strongest DPR protocol in this round** (mean DPR **0.630 ± 0.035** across replicates), with low death burden (**20.0 ± 3.0** deaths/run), consistent with stable network quality in `01_mean_pdr_merged.png` and `03_dead_uav_cumulative_merged.png`.
- **Worst DPR is `ugv_p_role_priority`** (mean **0.554 ± 0.094**), despite high measured delay; this protocol appears unstable across replicates (`01_mean_pdr_merged.png`, `06_mean_e2e_delay_merged.png`).
- **Charging success links to better DPR** across 21 runs (Pearson **+0.578**, Spearman **+0.552** in `cross_layer_correlations.csv`), while **routing-drop rate links to worse DPR** (Pearson **−0.527**, Spearman **−0.555**).
- **Deaths mediate the cross-layer effect:** total deaths correlate with lower DPR (Pearson **−0.567**, Spearman **−0.477**), aligning with `03_dead_uav_cumulative_merged.png` and `07_death_slope_vs_pdr_dip.png`.
- **E2E delay must be interpreted with conditioning caution:** low-DPR/high-death runs can show low delay (e.g., `ugv_edf_3`), because delay is computed on delivered packets only (`06_pdr_vs_e2e_scatter.png`, `06_e2e_delay_merged.png`).
- **Lag evidence is mixed but partially supportive:** several protocols show peak negative PDR↔death lag around **−8 to −9 min**, but not all protocols agree (`summary_tables/07_lagged_corr_summary.csv`, `07_lagged_corr_*.png`).
- **Mechanism supported by available evidence:** scheduling outcomes (success/routing-drop/timeout) influence charging continuity and deaths, which then shape topology continuity and DPR stability.

---

## 2) Data coverage & sanity checks

### 2.1 Coverage

- **Protocols found (7):** `ugv_dynamic`, `ugv_edf`, `ugv_fcfs`, `ugv_p_dynamic_score`, `ugv_p_edf`, `ugv_p_role_priority`, `ugv_role_priority`.
- **Replicates:** exactly 3 each (`_1`, `_2`, `_3`) for all protocols.
- **Missing required files:** none for checked core files (`network_timeseries.csv`, `charge_events.csv`, `charge_queue_timeseries.csv`, `death_events.csv`, `qos_metrics.csv`, `charge_session_events.csv`, `packet_delivered_events.csv`).
- **Run duration consistency:** all runs end near 180 min (`tmax` range ~180.003–180.027 min).

### 2.2 Column/value sanity

- No missing `window_pdr` values detected in `network_timeseries.csv` across runs.
- Large number of exact `window_pdr = 0` windows exists (aggregate 109,227 windows), so dip analyses should always report window counts.
- Delay sample size (`delay_n`) varies substantially by run, so direct mean-delay ranking is potentially biased.
- Duplicate-key candidates exist in `charge_events.csv` when keyed as `(t_rel_s, node_id, outcome)` in 8 runs (small counts 1–3 each); these may be real repeated events at same timestamp, but should be validated before deduplication.
- No backward timestamp steps found in `network_timeseries.csv`.

### 2.3 Quick data-quality verdict

Data is complete enough for cross-layer inference, but **delay conditioning** and **possible event-key duplicates** should be explicitly controlled in any causal claim.

---

## 3) Protocol-level comparisons (replicates merged)

Source: `cross_layer_protocol_summary.csv` (means over 3 replicates per protocol).

| Protocol | DPR mean ± std | E2E delay mean ± std (ms) | Charge success | Timeout rate | Decision latency (ms) | Effective wait (ms) | Energy/session (Wh) | Mean deaths/run |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| ugv_p_edf | **0.630 ± 0.035** | 57.9 ± 16.5 | **0.486** | 0.289 | 3926.2 | 14858.9 | 61.2 | **20.0** |
| ugv_fcfs | 0.616 ± 0.041 | 64.2 ± 11.4 | 0.463 | **0.267** | 4908.2 | 14818.6 | 64.1 | 27.0 |
| ugv_dynamic | 0.595 ± 0.045 | 62.1 ± 9.6 | 0.471 | 0.230 | 3353.3 | 12197.7 | 65.0 | 23.0 |
| ugv_p_dynamic_score | 0.580 ± 0.033 | 52.8 ± 21.1 | 0.411 | 0.258 | 4303.3 | 15425.1 | 60.9 | 31.7 |
| ugv_edf | 0.573 ± 0.094 | 55.6 ± 23.8 | 0.390 | 0.262 | 4362.0 | 14042.5 | 63.2 | 45.0 |
| ugv_role_priority | 0.563 ± 0.071 | 66.7 ± 8.0 | **0.384** | **0.319** | **5212.9** | 16456.2 | **65.8** | 39.3 |
| ugv_p_role_priority | 0.554 ± 0.094 | **71.4 ± 4.8** | 0.438 | 0.295 | 4149.3 | **18377.5** | 62.4 | 26.7 |

**Interpretation highlights**

- **Reliability leader:** `ugv_p_edf` has best DPR and lowest deaths.
- **Charging-failure leader:** `ugv_role_priority` has lowest success and highest timeout burden.
- **Variance warning:** `ugv_edf` and `ugv_p_role_priority` show the highest replicate DPR variance (std ~0.094), indicating unstable behavior across scenarios.
- **Delay ranking is not sufficient alone** because low-delay outliers coincide with high-death, low-DPR runs (`06_pdr_vs_e2e_scatter.png`).

---

## 4) Cross-layer linkage analysis (core)

Source: `cross_layer_correlations.csv` (n=21 runs).

### 4.1 Charging KPIs vs DPR

| KPI | Pearson r with DPR | Spearman ρ with DPR | Reading |
|---|---:|---:|---|
| Success rate | **+0.578** | **+0.552** | More successful charging aligns with stronger DPR. |
| Routing-drop rate | **−0.527** | **−0.555** | Lost charge requests align with weaker DPR. |
| Total deaths | **−0.567** | **−0.477** | Survivability loss aligns with DPR loss. |
| Dock utilization | +0.569 | +0.412 | Better-utilized docks co-occur with better DPR (not necessarily causal). |
| Effective wait | −0.326 | −0.321 | Longer waits weakly align with lower DPR. |
| Timeout rate | −0.116 | −0.104 | Weak relationship in this round. |
| Decision latency | −0.175 | +0.036 | Near-zero relationship. |

**Mechanism narrative (evidence-consistent):**

Scheduling policy influences whether charge attempts become `STARTED` versus `ROUTING_DROP/TIMEOUT`; this changes delivered charging opportunities and survival; survival affects topology continuity; topology continuity affects DPR. This chain is directly consistent with `03_charge_outcome_breakdown_merged.png`, `03_dead_uav_cumulative_merged.png`, and `01_network_pdr_over_time_merged.png`.

### 4.2 Charging KPIs vs E2E delay (conditioning bias included)

- Delay has weak monotonic coupling to most charging KPIs (Spearman values near 0 for many metrics).
- Strongest Pearson negatives are with deaths (−0.548) and routing-drop rate (−0.511), but corresponding Spearman values are weak (−0.089, −0.171), signaling outlier-sensitive/non-monotonic behavior.
- This matches the known conditioning issue: E2E delay uses delivered packets only; when DPR collapses, delivered samples come from easier/shorter surviving paths, reducing measured delay.

### 4.3 Time-aligned evidence (lagged)

From `summary_tables/07_lagged_corr_summary.csv` and `07_lagged_corr_*.png`:

- For `ugv_dynamic`, `ugv_fcfs`, `ugv_role_priority`, and `ugv_p_edf`, peak PDR↔death lag is negative around **−8 to −9 min** (PDR deterioration leading death increase).
- For `ugv_edf`, `ugv_p_dynamic_score`, and `ugv_p_role_priority`, the relation is weaker or sign-inverted, indicating protocol/run-specific dynamics and likely sensitivity to extreme runs.
- Dock utilization vs timeout links are generally modest (absolute peak correlations mostly <0.30), so overload-only explanations are insufficient.

**Causal caution:** results are **evidence consistent with** the mechanism, not definitive causality. A stronger test would control for weather regime and isolate pre/post-event windows.

---

## 5) Anomalies and interpretations

### A1) `ugv_edf_3` low delay + very poor network state

- **Observed:** very low delay (~28.5 ms) with low DPR (~0.476), high deaths (85), and high routing-drop rate (~0.496).
- **Likely cause:** delay conditioning on delivered packets under partial network collapse.
- **Quick verification:** inspect `ugv_edf_3/network_timeseries.csv` (`window_delay_mean_ms`, `window_pdr`) jointly with delivered sample count in `packet_delivered_events.csv`.

### A2) Delay can improve while network worsens

- **Observed:** several low-delay runs are not high-DPR runs.
- **Likely cause:** same conditioning artifact plus topology simplification after node loss.
- **Quick verification:** compute per-window delay against per-window delivered count and active UAV count.

### A3) Small duplicate candidates in charge events

- **Observed:** 8 runs contain 1–3 duplicate keys `(t_rel_s,node_id,outcome)`.
- **Likely cause:** either genuine same-timestamp repeated events or logging duplication.
- **Quick verification:** inspect adjacent rows in `charge_events.csv` for duplicated keys and compare other fields (`failure_reason`, queue state, request IDs if available).

### A4) Many exact PDR zeros

- **Observed:** large aggregate zero-PDR window count.
- **Likely cause:** repeated blackout periods or windowing threshold effects.
- **Quick verification:** compare zero-PDR windows with death/timeouts in `07_pdr_events_merged_all_protocols.png` and raw `network_timeseries.csv`.

---

## 6) Additional plots needed (optional but recommended)

Current figures already support a defensible thesis-level narrative. However, two additions would materially improve robustness:

1. **Conditioning-aware delay robustness panel**
   - Inputs: `network_timeseries.csv` + per-window delivered packet count.
   - Output: delay vs delivered-sample-size and DPR-weighted delay ranking.
   - Why: directly addresses known delay conditioning bias.

2. **Event-aligned composite (per protocol and merged)**
   - Inputs: binned series from `network_timeseries.csv`, `charge_events.csv`, `death_events.csv`, `charge_queue_timeseries.csv`.
   - Output: synchronized panes (DPR, timeout count, deaths, dock utilization).
   - Why: improves interpretability of the proposed mechanism sequence.

No pipeline changes were required for this report; only new summary tables were generated via ad-hoc analysis code.
