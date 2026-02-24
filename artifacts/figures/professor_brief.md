- In this dataset (observed in round1 logs), **UGV FCFS** shows the best overall reliability in the policy summary table (overall PDR ≈ **0.569**), while **UGV preemptive role-priority** is the lowest (overall PDR ≈ **0.436**).
- Delay tradeoff in round1 logs: **preemptive dynamic-score** has the lowest delivered-message median delay (about **1.26 ms**, IQR ≈ **2.16 ms**), while **EDF** has a higher median (about **1.86 ms**) and wider spread (IQR ≈ **3.44 ms**).
- Across policies, delivered-message delay medians are all low (roughly **1.3–1.9 ms**), so the stronger difference is dispersion (IQR), not absolute median.
- Survivability outcome is mixed in this dataset: by final death-events summary, **preemptive role-priority** has the fewest final deaths (**3**) and **preemptive EDF** the most (**23**), so reliability and survivability do not move together monotonically.
- Charging-wait fairness (role split) appears asymmetric in round1 logs: CH-like role (`0`) median waits are around **4.1–5.1 s**, while MEMBER-like role (`1`) median waits are around **1.0 s** across policies.
- For CH vs MEMBER spread, **preemptive role-priority** shows relatively larger MEMBER variability (MEMBER IQR ≈ **552 ms**) than FCFS/EDF-style cases (MEMBER IQR often tens of ms), suggesting more uneven member waits under some priority choices.
- Queue congestion is generally modest in this dataset (mean UGV queue length about **0.19–0.36**, p95 around **1–2**), indicating short bursts rather than persistent backlog.
- Throughput (delivered messages per second, from delivered-time windows) is highest for **FCFS** (about **3.06 msg/s**) and lowest for **preemptive role-priority** (about **1.19 msg/s**), consistent with observed low/high reliability extremes in round1 logs.

Caveats (data availability / interpretation):
- Role encoding in charging logs appears numeric (`role=0/1`), so CH vs MEMBER interpretation depends on a mapping assumption (`0`→CH-like, `1`→MEMBER-like) and should be verified against source semantics.
- Each policy here is effectively a **single run** in round1 artifacts, so confidence intervals are not available; all comparisons should be treated as directional, not statistically conclusive.
