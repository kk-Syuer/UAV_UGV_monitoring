# Section 4.6 — Missing Artifacts

## Missing computations

| What is missing | Why it is needed | Expected output location |
|---|---|---|
| Pearson r and Spearman ρ for charge success rate vs mean PDR in Round 3 (n = 21 runs) | The Round 2 cross-layer analysis reports r = 0.578, p = 0.006. The Round 3 per-run correlation must be recomputed to confirm whether the positive association remains statistically significant under fixed weather. | Report values in Section 4.6; optionally add to `analysis/test_round3/figures/cross_layer/CL_E_correlation_bar_chart.png` if not already present. |
| Pearson r and Spearman ρ for depletions vs mean PDR in Round 3 (n = 21 runs) | Round 2 reports r = −0.57. The compressed depletion range in Round 3 may weaken the correlation; the actual values should be reported. | Same as above. |

## Existing figures confirmed present

All figures referenced in the evidence map (Table 1 of the .tex file) have been verified to exist in both `analysis/test_round2/figures/` and `analysis/test_round3/figures/`. No missing plot files were detected.

## Notes

- The `CL_J_ugv_edf3_cascade.png` figure exists only in Round 2 (no equivalent cascade event in Round 3). This is expected and does not affect Section 4.6.
- The `CL_O_mechanism_narrative.png` figure exists only in Round 3. It is not referenced in Section 4.6 but could optionally be cited in 4.6.3 to illustrate the mechanism narrative under fixed weather.
- If the Round 3 per-run correlation values are already embedded in `CL_E_correlation_bar_chart.png`, the TODO in the .tex file can be resolved by reading those values from the figure and inserting them into the text.
