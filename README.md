# FANET-Based UAV–UGV Cooperative Monitoring System (Flood / Disaster Scenarios)

## Overview

This project implements a **fully simulated FANET (Flying Ad‑hoc Network)** for **UAV–UGV cooperative monitoring in disaster and flood scenarios**, with a strong focus on **network robustness, routing under mobility, charging logistics, and weather‑induced failures**.

All network behavior (routing, buffering, drops, acknowledgements) is **explicitly modeled at application level**, enabling precise QoS measurement and reproducible experiments.

The project is structured to support:

* Highly mobile UAV swarms
* Dynamic network partitioning
* Store–carry–forward routing
* Energy‑aware charging via UGVs
* Weather‑driven network instability
* End‑to‑end QoS evaluation

---

## Key Concepts

### 1. FANET Network Abstraction

All packets (DATA and CONTROL) are routed through a **logical FANET bus**:

* `/fanet/network_bus_raw` – packets before impairment
* `/fanet/network_bus` – packets after fault injection
* `/fanet/delivered` – authoritative delivery events

This separation allows us to **inject failures without modifying node logic** and to **measure true end‑to‑end performance**.

---

### 2. Routing Model (No Global Knowledge)

Each UAV performs:

* Local neighbor discovery
* Greedy next‑hop selection
* Charging‑aware routing penalty
* Loop guard + TTL enforcement
* Store–carry–forward buffering

---

### 3. Roles in the System

* **Member UAVs**
  Perform area scanning and generate search telemetry when reaching task points.

* **Cluster‑Head (CH) UAVs**
  Act as aggregation and relay nodes; may temporarily disconnect from the backbone.

* **UGV Charger**
  Provides charging service; charging requests and decisions are routed through the FANET.

* **Sink Gateway**
  Collects all delivered data and control packets.

* **User Device**
  Generates external traffic to stress the network.

---

### 4. Charging Protocol Evaluation

Charging is treated as a **networked control problem**:

* `ChargeRequest` and `ChargeDecision` packets are routed like any other traffic
* Decisions may be delayed, dropped, or arrive too late
* Charging success depends on **both network QoS and energy state**

The system records:

* Request acceptance / rejection
* Timeouts and drops
* Docking success
* Charging latency

This enables **direct comparison of charging policies under network stress**.

---

### 5. Weather‑Driven Fault Injection

A dedicated **fault injector** models packet drops as a function of:

* Wind intensity
* Rain intensity
* Temperature deviation

Drops affect:

* Data traffic
* Control traffic (with configurable attenuation)

This allows controlled experiments on **network resilience under adverse environmental conditions**.

---

## Architecture

```
+-------------------+
| Weather Node      |
+---------+---------+
          |
          v
+-------------------+       
| Fault Injector    |
+---------+---------+       
          |
          v
+-------------------+        +---------------------+
| FANET Raw Bus (In)|<------ | UAV / UGV / Sink    |
+---------+---------+        +---------------------+
          |
          v
+-------------------+        +---------------------+
| FANET Raw Bus(Out)|------> | UAV / UGV / Sink    |
+-------------------+        +---------------------+
                                        |
                                        v
                             +---------------------+
                             | Delivered Channel   |
                             +---------------------+
                                        |
                                        v
                             +-------------------+
                             | Network Monitor   |
                             +-------------------+
```

---

## Metrics & Logging (Step F)

The system automatically records **experiment‑ready metrics**:

### Per‑Message (`messages.csv`)

* End‑to‑end delay
* Hop count / forwarding overhead
* Delivery or drop reason
* Loop / TTL / weather drops

### Charging Events (`charge_events.csv`)

* Success vs failure
* Decision latency
* Failure causes (drop, timeout, energy)

### UAV State Time Series (`status_timeseries.csv`)

* Battery level
* Charging state
* Backbone participation
* Position over time

### Run Summary (`summary.json`)

* PDR by traffic type
* Delay statistics (mean / p95)
* Drop breakdown
* Charging success rate

All outputs are written to:

```
<output_dir>/<run_id>/
```

---

## System Bringup

The entire system is launched via the **`system_bringup` package**.

### Example

```bash
ros2 launch system_bringup experiment.launch.py \
  config:=system_bringup/config/runs/example_run.yaml \
  run_id:=demo_weather_high \
  output_dir:=/tmp/fanet_logs
```

---

## Reproducible Experiments

Experiments are defined via YAML files:

* Network parameters
* Weather regime
* Charging policy
* Traffic intensity
* Random seeds

This allows **batch execution and fair comparison** across scenarios.

---

## Intended Use

This project is intended for:

* Academic research
* FANET / disaster‑response simulation
* Energy‑aware networking studies

---


## License

MIT
