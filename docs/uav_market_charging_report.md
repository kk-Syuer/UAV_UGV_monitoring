# UAV Market & Charging Benchmark Report

**Target artifact:** `docs/uav_market_charging_report.md`  
**Scope:** Commercial multirotor UAVs and autonomous “drone-in-a-box” docking systems, benchmarked for endurance, battery energy, and charging behavior to validate simulator assumptions for UAV–UGV cooperative charging scheduling and preemption.

---

## 1. Executive Summary

- **Typical multirotor endurance today is ~35–55 minutes** on a single pack for mainstream commercial platforms. Consumer/prosumer flagships cluster around **~40–46 min**, while enterprise inspection platforms can reach **~41–55 min** depending on payload and dual-battery designs.
- **Battery energy** for prosumer drones is typically **~60–80 Wh** (e.g., DJI Air 3 at 62.6 Wh; DJI Mavic 3 series at 77 Wh). For enterprise platforms using larger packs, energy is commonly **~250–300 Wh per pack** (e.g., DJI TB65 at 263.2 Wh).
- **Charge times are highly product- and infrastructure-dependent:**
  - Consumer drones: **~80–96 min** to full (0–100%) with standard OEM chargers.
  - Enterprise ecosystems: battery stations often support **~50–60 min** for full charge (typically for *two batteries together*), and **~30 min** for “20→90%” style rapid turnaround.
  - Docking systems: recharge targets are optimized for continuous operations, commonly **~30–35 min for ~20/15→90/95%**.
- **Fast charging exists but is unevenly available**:
  - Consumer/prosumer: sometimes available via higher-power USB-C PD adapters + charging hubs (e.g., Air 3: 80 min → 60 min).
  - Enterprise/dock: commonly supported as part of an operational workflow (battery stations / docks designed around turnaround times).
- **For your simulator assumptions**:
  - **45 min endurance** is realistic for modern prosumer and some enterprise multirotors under “max endurance” conditions.
  - **90 min endurance** is *not typical* for multirotor UAVs currently dominating the commercial market; it generally implies either (i) fixed-wing/VTOL hybrids, or (ii) very large platforms not represented by mainstream “dock-ready” systems.
  - **60–90 min charging** is broadly realistic as a “standard charge to full” envelope for many batteries; however, operational systems often rely on **partial recharge (e.g., 20→90%)** with much shorter turnaround.

---

## 2. UAV Specification Table (Table 1 — UAV Technical Specifications)

> Notes:
> - “Fast charge” here means an OEM-published shorter recharge time using higher-power infrastructure **or** an explicit rapid-turnaround metric (e.g., 20→90%).
> - “Hot swap” means the aircraft can continue operation (or remain powered/ready) while swapping/servicing batteries without a full shutdown or without losing mission continuity; consumer drones are generally **battery-swappable but not hot-swappable**.

| Manufacturer | Model | Release year | Flight Time (min) | Battery (Wh) | Battery Voltage | Charge Time (min) | Fast Charge | Fast Charge Time | Hot Swap | Dock Support | Approx. price range | Official / trusted sources |
|---|---|---:|---:|---:|---:|---:|---|---:|---|---|---|---|
| DJI | Air 3 | 2023 | 46 | 62.6 | 14.76 V | 80 | Yes | 60 | No | No | ~US$1,099 (base) | DJI specs (battery + charge time): https://www.dji.com/air-3/specs ; Release/price (retail/press): https://www.techradar.com/cameras/drones/dji-air-3 |
| DJI | Mavic 3 Classic | 2022 | 46 | 77 | 15.4 V | 96 | No (not marketed as fast charge) | Not publicly specified | No | No | ~US$1,599 (launch price) | DJI press release (launch): https://www.dji.com/newsroom/news/dji-unveils-mavic-3-classic ; DJI charging time guidance: https://support.dji.com/help/content?customId=en-us03400006769 ; DJI battery technical specs table: https://repair.dji.com/help/content?customId=en-us03400006564&lang=en&re=US&spaceId=34 |
| Autel Robotics | EVO II Pro V3 | 2022 | 40 | 82 | 11.55 V | 90 | No | Not publicly specified | No | No | ~US$2,099–2,999 (bundle-dependent) | Autel support PDF (battery + charge + endurance): https://auteldrones.com/products/evo-ii-pro-v3 ; Autel official store pricing: https://shop.autelrobotics.com/collections/autel-evo-ii-series |
| DJI | Matrice 30T | 2022 | 41 | 263.2 (TB30) | Not publicly specified | 50 (0–100% for **two** TB30 via BS30) | Yes | 30 (20→90% for **two** TB30) | Not publicly specified | Indirect (fleet/remote ops; not “Dock 2”) | ~US$11,656 (B&H listing example) | DJI TB30/BS30 times & TB30 energy: https://enterprise.dji.com/matrice-30/specs ; Launch announcement: https://www.mynewsdesk.com/us/dji-enterprise/pressreleases/dji-launches-matrice-30-series-3229469 ; Example pricing: https://www.bhphotovideo.com/c/buy/matrice-drones/ci/33453 |
| DJI | Matrice 350 RTK | 2023 | 55 | 263.2 (TB65) | 44.76 V | 60 (0–100% for **two** TB65 via BS65) | Yes | 30 (20→90% for **two** TB65) | Yes (dual-battery hot-swap design) | No (separate from Dock 2 ecosystem) | Not publicly specified (varies by bundle) | DJI TB65/BS65 + voltage & charge times: https://enterprise.dji.com/matrice-350-rtk/specs ; Hot swap statement: https://enterprise.dji.com/matrice-350-rtk/specs# (Hot-Swappable Batteries section) |
| Skydio | X10 | 2023 | Not publicly specified (commonly cited ~40) | Not publicly specified | Not publicly specified | 105 (100W; 0–100%) | Yes | 60 (230W; 0–100%) | Not publicly specified | Yes (Dock for X10) | Not publicly specified (enterprise quote-based) | Skydio X10 technical specs (charge times): https://www.skydio.com/x10/technical-specs ; Skydio X10 product: https://www.skydio.com/x10 |
| DJI | Matrice 3D (Dock 2 ecosystem) | 2023 | 50 | 115.2 | 14.76 V | Not publicly specified (0–100) | Yes (turnaround metric) | 32 (20→90% via Dock 2) | Not publicly specified | Yes (Dock 2) | Dock bundle typically ~US$9k–20k (region/bundle dependent) | DJI Dock 2 specs (flight time): https://enterprise.dji.com/dock-2/specs ; DJI support (20→90% = 32 min, no auto battery swap): https://www.dji.com/global/support/product/dock-2 ; DJI battery PDF: https://dl.djicdn.com/downloads/DJI_Dock_2/20240326/Matrice_3D_Series_Intelligent_Flight_Battery_Production_Information_Multi.pdf ; Example retail pricing: https://djinyc.com/collections/dji-dock |

---

## 3. Charging Comparison Table (Table 2 — Charging Efficiency Comparison)

**Definitions**
- **Charge/Flight Ratio** = `standard_charge_time / max_flight_time` (lower is better for turnaround).
- **Fast Improvement %** = `(standard - fast) / standard`.

| Model | Flight Time (min) | Charge Time (min) | Fast Charge Time (min) | Charge/Flight Ratio | Fast Improvement % |
|---|---:|---:|---:|---:|---:|
| DJI Air 3 | 46 | 80 | 60 | 1.74 | 25% |
| DJI Mavic 3 Classic | 46 | 96 | Not publicly specified | 2.09 | Not publicly specified |
| Autel EVO II Pro V3 | 40 | 90 | Not publicly specified | 2.25 | Not publicly specified |
| DJI Matrice 30T (TB30 via BS30) | 41 | 50 (0–100% for **two** batteries) | 30 (20→90% for **two** batteries) | 1.22 | 40% |
| DJI Matrice 350 RTK (TB65 via BS65) | 55 | 60 (0–100% for **two** batteries) | 30 (20→90% for **two** batteries) | 1.09 | 50% |
| Skydio X10 | Not publicly specified | 105 (100W; 0–100%) | 60 (230W; 0–100%) | Not publicly specified | 42.9% |
| DJI Dock 2 ecosystem turnaround (Matrice 3D/3TD) | 50 | Not publicly specified | 32 (20→90% in dock) | Not publicly specified | Not publicly specified |

---

## 4. Fast Charging Analysis

### 4.1 Is fast charging common?

- **Consumer/prosumer**: moderately common **as an option**, typically via USB-C PD high-power adapters and/or a charging hub (e.g., DJI Air 3). OEMs often publish two charge times depending on the charger/hub combination. (DJI Air 3 spec sheet publishes both 65W and 100W hub-based times.)
- **Enterprise**: fast/rapid turnaround is common as part of the **battery station workflow**, but published metrics often use “20→90%” rather than “0→100%” because **operations rarely require 100% to resume service**.
- **Docking systems**: “fast” is effectively the default because docks are designed to minimize mission interval (e.g., Dock 2 “20→90% in 32 min”; Skydio Dock for X10 “15→95% ~35 min at 25°C”).

### 4.2 Quantitative comparisons

- **DJI Air 3**: Standard ~80 min → fast ~60 min (25% reduction). Source: DJI Air 3 specs.
- **DJI Matrice battery stations**: Published as 20→90% in 30 min versus 0→100% in 50–60 min (40–50% reduction in time-to-ready, depending on platform). Source: DJI enterprise spec pages.
- **Skydio X10**: 100W 0→100% ~105 min versus 230W 0→100% ~60 min (42.9% reduction). Source: Skydio X10 technical specs.

### 4.3 Battery health and lifecycle considerations (what the market implies)

Manufacturers rarely publish explicit “lifespan penalty” numbers in consumer spec sheets, but their product designs and published workflows suggest:
- **Rapid turnaround charging is operationally expected** in enterprise fleets, and is supported by **smart batteries + managed battery stations**.
- Many systems emphasize **partial charge windows** (e.g., 20→90% or 15→95%), which aligns with common lithium charging behavior: the final portion (near full) is slower (constant-voltage phase), and partial cycles can be operationally more efficient.

---

## 5. Dock-Based Charging Systems

### 5.1 DJI Dock 2 (with Matrice 3D / 3TD)

- **Turnaround metric:** With ambient ~25°C and battery at ~20% after landing, Dock 2 takes **32 minutes to charge to 90%**; Dock 2 **does not support automatic battery replacement**.  
  Source: DJI support for Dock 2.
- **Endurance:** DJI Dock 2 specs list the Matrice 3D series max flight time as **50 minutes** (test conditions documented by DJI).  
  Source: DJI Dock 2 specs.

### 5.2 Skydio Dock for X10

- **Turnaround metric:** Skydio states **15%→95% takes ~35 minutes at 25°C**, and can take longer in extreme temperatures (up to a stated max in their FAQ).  
  Source: Skydio Dock for X10 FAQs.

### 5.3 What docking implies for “charging scheduling”

Docks effectively hard-code a **single-service-station** model per deployed dock: one aircraft per dock, with deterministic or bounded recharge turnaround. This shifts the scheduling problem from “many UAVs sharing one UGV charger” to:
- **Fleet-level allocation of docks** (where to place them, how many), and
- **Airspace/mission dispatch scheduling** (which mission to run next),
while **energy replenishment becomes more predictable** than ad-hoc field charging.

---

## 6. Implications for UAV–UGV Cooperative Charging Simulation

### 6.1 Endurance realism check

- **Simulator assumption: 45 min endurance**  
  This is realistic for “max endurance” conditions in the prosumer market (e.g., DJI Air 3 and DJI Mavic 3 series advertise **46 minutes**; Autel EVO II Pro V3 advertises **40 minutes**). Sources: DJI specs/support and Autel specs PDF.

- **Simulator assumption: ~90 min endurance (e.g., CH)**  
  A ~90 minute endurance assumption is **not representative of the mainstream multirotor commercial market** captured by DJI/Autel/Skydio class systems. If your CH is intended to be a multirotor similar to these, 90 min is likely optimistic unless you explicitly model:
  - larger-than-mainstream aircraft,
  - hybrid VTOL/fixed-wing,
  - reduced payload / specific flight profiles.

### 6.2 Charging time realism check

- **Simulator assumption: 60–90 min charging to full**  
  This matches common consumer/prosumer behavior (Air 3: ~80 min standard; Mavic 3: ~96 min; Autel EVO II Pro V3: ~90 min). It also overlaps with enterprise full charge times when using battery stations (e.g., 50–60 min for two-battery sets).

- **However:** Real operations often avoid “0→100%” as the primary metric.
  Many enterprise/dock systems publish “20→90%” or “15→95%” turnaround numbers because that’s what matters for mission continuity (Dock 2: 32 min to 90%; Skydio Dock: 35 min to 95%; DJI BS65/BS30: 30 min 20→90%).

### 6.3 Preemption and partial charging realism

**Is “mid-charge interruption” realistic?**
- Operationally: yes. Most charging workflows do not require uninterrupted 0→100% charging to be useful; turnaround metrics are often explicitly defined for partial windows.
- Physically: lithium batteries charge faster in the early phase and slower near full; this supports the *idea* that partial charging can produce meaningful usable energy quickly.

**Caveat for simulator abstraction:**
- Real smart-battery systems include internal management (balancing, temperature conditioning, safety checks). Preempting at arbitrary states is feasible, but the *marginal gain* of charging depends on the charge curve and thermal constraints—especially under “fast charge” power.

---

## 7. References

Primary manufacturer / support sources used in this report:

- DJI Air 3 technical specs (battery, charge time): https://www.dji.com/air-3/specs
- DJI intelligent flight battery technical specs table (Air 3 / Mavic 3): https://repair.dji.com/help/content?customId=en-us03400006564&lang=en&re=US&spaceId=34
- DJI Mavic 3 Series charging guide (incl. ~96 min per battery): https://support.dji.com/help/content?customId=en-us03400006769
- DJI Mavic 3 Classic press release (release timing): https://www.dji.com/newsroom/news/dji-unveils-mavic-3-classic
- Autel EVO II Pro V3 support/spec PDF (battery energy, flight time, charge time): https://auteldrones.com/products/evo-ii-pro-v3
- Autel official store (price examples): https://shop.autelrobotics.com/collections/autel-evo-ii-series
- DJI Matrice 30 specs (TB30 energy; BS30 charge times): https://enterprise.dji.com/matrice-30/specs
- DJI Matrice 350 RTK specs (TB65 energy/voltage; BS65 charge times; hot swap section): https://enterprise.dji.com/matrice-350-rtk/specs
- DJI Dock 2 specs (Matrice 3D series endurance): https://enterprise.dji.com/dock-2/specs
- DJI Dock 2 support Q&A (32 min 20→90%, no battery swap): https://www.dji.com/global/support/product/dock-2
- DJI Matrice 3D battery production info PDF (battery data): https://dl.djicdn.com/downloads/DJI_Dock_2/20240326/Matrice_3D_Series_Intelligent_Flight_Battery_Production_Information_Multi.pdf
- Skydio X10 technical specs (charge times 100W vs 230W): https://www.skydio.com/x10/technical-specs
- Skydio Dock for X10 FAQs (15→95% ~35 min): https://www.skydio.com/dock/faqs

Secondary sources used only for pricing examples where OEM does not publish MSRP:

- Example DJI Dock 2 retail listing (bundle pricing varies): https://djinyc.com/collections/dji-dock
- Example DJI enterprise listing (Matrice 30T): https://www.bhphotovideo.com/c/buy/matrice-drones/ci/33453

---

## 8. Implications for Preemptive Charging Policies (Required)

This section connects the market findings directly to **preemptive scheduling and partial charging policies** in your UAV–UGV simulator.

### 8.1 Do real UAVs benefit from partial charge cycles?

Yes—market evidence strongly suggests that **partial charging is operationally meaningful**:
- DJI Dock 2 defines mission interval by **20→90% in 32 minutes**, not by full recharge.
- Skydio Dock for X10 defines readiness by **15→95% in ~35 minutes**.
- DJI enterprise battery stations publish **20→90% in 30 minutes**.

This aligns with the practical reality that operators care about “return-to-service time,” and that “near-full” charging is slower.

### 8.2 Does fast charging reduce the need for preemption?

Fast charging reduces *pressure* on preemption but does not eliminate it:
- For **single-charger bottlenecks** (UGV with limited pads), faster charging reduces queueing delay but does not solve **contention** when multiple UAVs arrive simultaneously.
- When rapid charging is available, the preemption policy may shift from “interrupt for emergencies” to “interrupt to maintain fairness / prevent starvation / keep mission-critical UAVs above a minimum state-of-charge.”

Quantitatively, the largest deltas in this study are ~25–50% reductions in time-to-ready depending on infrastructure (Air 3: 25%; enterprise stations: 40–50%; Skydio X10: ~43%). These are large enough to change queue dynamics, but not enough to make scheduling irrelevant.

### 8.3 Does enterprise docking reduce scheduling complexity?

Docking systems reduce *charging* scheduling complexity by turning energy replenishment into a **bounded, almost deterministic service process** (one dock ↔ one aircraft), but they introduce other allocation problems:
- How many docks are deployed and where.
- Fleet dispatch and mission assignment across docks.
- Operational constraints (weather, comms, regulatory limits).

In other words: **docks simplify energy scheduling per aircraft**, but shift complexity to **infrastructure placement and mission orchestration**.

### 8.4 How realistic is the “cooperative UGV charger” abstraction?

- If your simulator represents **field operations without permanent docking infrastructure**, a UGV-as-mobile-charger abstraction remains realistic—especially in disaster settings where fixed infrastructure may be unavailable.
- The market shows that real ecosystems already use **battery stations, managed charging, and rapid turnaround targets**, which supports modeling the UGV charger as a constrained service resource.

**Key realism knob for your simulator:** consider modeling *charge-to-ready* targets (e.g., 20→90% windows) rather than forcing all UAVs to request/receive only full charges. This matches how modern dock/enterprise systems express operational recharge.

