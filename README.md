# UAV–UGV Disaster-Area Network Simulation (ROS 2)

A modular **ROS 2 simulation framework** for studying **UAV ad-hoc networks**, **UGV charging policies**, **coverage planning**, **routing**, **mobility**, and **environment-dependent behaviour** in disaster scenarios.

The system simulates:

* A **backbone** of *cluster-head UAVs (CHs)* that route data and control traffic
* **Member UAVs** connected to CHs
* A **UGV** acting as a mobile charging station
* A **sink gateway** representing the “Internet” and deployment gatekeeper
* **Mobile phone users** that inject traffic
* A **weather environment** affecting UAV power consumption
* A **network monitor** collecting metrics (CSV) under per-run directories
* A **coverage planner** generating deployment & routing
* A **visualizer** showing CH, member, sink, and UGV positions with backbone state

This repository is designed for **research experiments**, especially on charging scheduling, routing strategies, connectivity robustness, battery/weather interactions, and the interplay between deployment acknowledgement and mobility start.

---

## ✨ Key Features (Implemented so far)

### ✔ Multi-role UAV simulation

Each UAV runs its own `uav_node` instance with:

* **Roles:**

  * `role=1` → Cluster Head (CH)
  * `role=0` → Member UAV
* **Battery model** with temperature-dependent drain
* **Weather subscription** (temperature affects power usage)
* **Traffic generation** (if enabled)
* **Mobility simulation** with speed/step tuning and per-UAV enable switches
* **Hop-by-hop routing** through CH backbone (plus per-destination overrides)
* **Buffering/retry** for control/data frames with TTL- and retry-aware drops
* **Charging request logic** and **session execution**
* **Failure detection** (battery dead event)

### ✔ UGV Charger with multiple scheduling policies

`ugv_charger_node` supports:

* FCFS
* Role-priority (CH > member)
* EDF (earliest battery depletion first)
* Dynamic weighted scoring
* Network-based *charge decision delivery* using control packets

The UGV also computes a **capacity planning hint** (spots needed vs. target utilization), tracks neighbors via periodic status, and can simulate motion/coverage radius when enabled.

The UGV tracks UAV status, queues requests, assigns slots, and emulates charging.

### ✔ Coverage Planner (deployment + routing)

`coverage_planner_node`:

* Randomly generates **sink** and **UGV** positions inside the area
* Places CHs on a **grid layout** (or later hex layout)
* Assigns member UAVs to nearest CH
* Computes **CH backbone connectivity graph**
* Runs **Dijkstra from sink** to compute `next_hop_to_sink`
* Publishes `UavDeployment` messages on `/coverage_planner/deployment` with:

  * Position
  * Role
  * Cluster ID
  * CH ID
  * Next hop information
* Keeps track of expected devices, waits for deployment acknowledgements, and notifies the sink gateway via DEPLOYMENT traffic on the network bus

### ✔ Traffic forwarding framework

Fully working multi-hop routing:

* Member UAV → its CH
* CH → next CH → … → sink
* CH used as routing hubs
* Supports control traffic (charging decisions)
* Sink gateway and coverage planner inject deployments into the network bus so deployment acknowledgements and control follow the same simulated multi-hop path as data packets

### ✔ Network Monitor

`network_monitor_node` computes:

* Packet generation count
* Packet delivery count
* End-to-end delay
* Average delay
* Charging request timestamps
* Charging wait times
* Charging session counts (per outcome)
* Battery death count & timestamps

Useful for experiments & performance comparison.

### ✔ Planner Visualization (2D GUI)

Python `planner_viz_node` dynamically displays:

* CH UAVs (red + dashed coverage circles)
* Member UAVs (green)
* Sink (blue)
* UGV (yellow)
* Auto-updates positions on every deployment message and colors backbone-active CHs

### ✔ Sink Gateway Node

Handles:

* Delivery of packets addressed to the sink
* Publishing `/fanet/delivered` for monitoring
* Mirroring deployments onto `/fanet/network_bus_raw`
* Collecting DEPLOYMENT acknowledgements and broadcasting START_MOBILITY when everyone is live

### ✔ Weather Server

Publishes `WeatherStatus` (temperature, etc.).
UAVs use temperature to scale energy consumption via a piecewise function.

### ✔ User Device Simulator

Simulates mobile phones generating traffic into the UAV network.

---

# 🗂 Project Structure Overview

```
UAV_UGV_netmonitoring/
│
├── src/
│   ├── uav_msgs/               # All custom message types
│   ├── uav_fleet/              # uav_node implementation
│   ├── ugv_charger/            # UGV charger & scheduling policies
│   ├── sink_gateway/           # Internet gateway node
│   ├── coverage_planner/       # CH placement, routing, deployments
│   ├── planner_viz/            # 2D live visualizer
│   ├── network_monitor/        # Logging & metrics
│   ├── ch_manager/             # Cluster membership (currently static)
│   ├── weather_server/         # Environment model
│   ├── user_devices_sim/       # Simulated mobile phones
│   ├── fault_injector/         # Failure injection (basic)
│   └── system_bringup/         # (for future launch files)
│
├── commands-to-run             # Useful command sequences
└── README.md
```

---

# 🚀 How It Works (Data Flow)

### 1. **Coverage Planner starts first**

* Generates positions for CHs, members, sink and UGV
* Computes Dijkstra routing for CH→sink
* Publishes `UavDeployment` for each UAV and UGV

### 2. **UAVs receive deployment**

Each UAV updates:

* Position
* Role
* Cluster
* CH assignment
* `next_hop_to_sink` (for CHs)

### 3. **Weather server influences UAV battery drain**

UAV battery consumption = base × weather factor.

### 4. **Traffic flows**

Member → CH → CH → … → Sink

### 5. **Charging requests**

If battery < threshold:

* UAV sends `CHARGE_REQUEST` as a control packet (routed via backbone)
* UGV evaluates queue and sends `CHARGE_DECISION`
* UAV travels into charging state

### 6. **Network monitor gathers statistics**

Per-packet and per-session metrics.

---

## ℹ️ Operational Notes

### Deployment delivery and acknowledgements

* The coverage planner publishes `UavDeployment` directly on `/coverage_planner/deployment`; UAVs consume it immediately when `accept_direct_deployment:=true`.
* The sink gateway also mirrors each deployment into a `TrafficMessage` on `/network/traffic`, letting deployments and acknowledgement packets follow the same multi-hop route as normal traffic.
* Deployment acknowledgements are sent as control packets via the backbone; enabling direct deployments on each UAV avoids the need to rerun nodes to trigger the first acknowledgement.

### Liveness and failure monitoring

* Every UAV publishes periodic heartbeats on `/uav_fleet/heartbeat` and prunes neighbors using configurable hello timers (default 1 s), providing liveness detection for routing and monitoring.
* Battery or node-failure events are emitted on `/uav_fleet/failure_events`, where the cluster-head manager and network monitor subscribe to log the outage and update topology-aware metrics.

### Multi-hop network simulation

* All control and data traffic uses `/network/traffic` (`uav_msgs/TrafficMessage`) with destination IDs, TTL, and hop counts; frames are forwarded hop by hop based on `next_hop_id` and dropped if TTL expires or the next hop does not match.
* Cluster heads relay member traffic toward the sink using their `next_hop_to_sink`, while also forwarding deployments, charging commands, and other control packets along the same simulated backbone.
* Delivered packets are reported on `/network/traffic_delivered` so the sink gateway and metrics nodes can track end-to-end performance.

### Weather model and Markov chain

* The weather server evolves a three-state Markov chain (SUNNY, WINDY, STORMY) each tick: SUNNY usually persists (≈85%), WINDY persists ≈65%, and STORMY persists ≈40%, with the remaining probability split across transitions to the other states.
* For the chosen state, the node samples temperature, wind speed, rain rate, and a slowly drifting wind direction from regime-specific normal distributions, clamps negatives to zero, and publishes `WeatherStatus` on `/environment/weather`.
* UAVs subscribe to `/environment/weather`, cache the latest conditions, and apply temperature-dependent scaling to their battery consumption.

---

# 🧪 Running the System (Example)

### Build and source

```bash
colcon build
source install/setup.bash
```

### Example command sequence

*(from `commands-to-run`)*

```bash
# 1. Weather
ros2 run weather_server weather_node

# 2. Coverage planner
ros2 run coverage_planner coverage_planner_node --ros-args \
    -p uav_ids:="['uav_1','uav_2','uav_3']" \
    -p num_ch:=2 \
    -p x_min:=0 -p x_max:=600 \
    -p y_min:=0 -p y_max:=600 \
    -p service_radius_ch:=250 -p comm_radius_ch:=400

# 3. Sink
ros2 run sink_gateway sink_gateway_node

# 4. UGV
ros2 run ugv_charger ugv_charger_node --ros-args \
    -p ugv_id:=ugv \
    -p uplink_ch_id:=uav_1 \
    -p charging_policy:=fcfs

# 5. UAVs
ros2 run uav_fleet uav_node --ros-args -p uav_id:=uav_1
ros2 run uav_fleet uav_node --ros-args -p uav_id:=uav_2
ros2 run uav_fleet uav_node --ros-args -p uav_id:=uav_3

# 6. Mobile users
ros2 run user_devices_sim user_device_node

# 7. Network monitor
ros2 run network_monitor network_monitor_node

# 8. Visualizer
ros2 run planner_viz planner_viz_node
```

---

# 🎯 Sample 3×CH + 2×Member Deployment

Launch sequence for three cluster heads, two members, one sink, one UGV, the fleet
visualizer, and the coverage planner:

```bash
# Terminal 1: weather
ros2 run weather_server weather_node

# Terminal 2: coverage planner (3 CHs, 2 members)
ros2 run coverage_planner coverage_planner_node --ros-args \
  -p uav_ids:="['uav_1','uav_2','uav_3','uav_4','uav_5']" \
  -p num_ch:=3 \
  -p x_min:=0 -p x_max:=600 \
  -p y_min:=0 -p y_max:=600 \
  -p service_radius_ch:=250 -p comm_radius_ch:=400

# Terminal 3: sink
ros2 run sink_gateway sink_gateway_node

# Terminal 4: UGV
ros2 run ugv_charger ugv_charger_node --ros-args \
  -p ugv_id:=ugv \
  -p uplink_ch_id:=uav_1 \
  -p charging_policy:=fcfs

# Terminals 5-9: UAVs
ros2 run uav_fleet uav_node --ros-args -p uav_id:=uav_1 -p role:=1
ros2 run uav_fleet uav_node --ros-args -p uav_id:=uav_2 -p role:=1
ros2 run uav_fleet uav_node --ros-args -p uav_id:=uav_3 -p role:=1
ros2 run uav_fleet uav_node --ros-args -p uav_id:=uav_4 -p role:=0
ros2 run uav_fleet uav_node --ros-args -p uav_id:=uav_5 -p role:=0

# Terminal 10: fleet visualizer
ros2 run planner_viz fleet_viz_node
```

---

# 📊 What You Can Study With This Framework

* Packet delivery rate vs. routing quality
* Delay distribution over multi-hop backbone
* Battery drain vs. environmental conditions
* Charging wait time under different policies
* Number of battery-dead events
* Impact of CH topology on coverage
* Routing robustness (future CH-failure handling)

---

# 🛠️ Next Steps (Roadmap)

### **Routing**

* Extend Dijkstra to compute `next_hop_to_ugv`
* Introduce a generic routing table per CH
* Add dynamic re-routing when CH fails

### **Cluster Management**

* Replace static cluster manager with **geometry-based clustering**
* Add periodic membership recalculation

### **Mobility & Simulation**

* Introduce simple motion model for:

  * UAV movement
  * UGV travelling to charging locations
* Later: integrate with Gazebo

### **Visualizer**

* Add path traces
* Add battery colour indicators
* Add animation for motion

### **Metrics**

* Export to CSV for offline analysis
* Add end-to-end path logging

### **Robustness**

* Fault injector should trigger full topology recomputation

---

# 📎 License

TBD.

---
