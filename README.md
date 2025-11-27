# UAV–UGV Disaster-Area Network Simulation (ROS2)

A modular **ROS2-based simulation framework** for studying **UAV ad-hoc networks**, **UGV charging policies**, **coverage planning**, **routing**, and **environment-dependent behaviour** in disaster scenarios.

The system simulates:

* A **backbone** of *cluster-head UAVs (CH)*
* **Member UAVs** connected to CHs
* A **UGV** acting as a mobile charging station
* A **sink gateway** representing the “Internet”
* **Mobile phone users**
* A **weather environment** affecting UAV power consumption
* A **network monitor** collecting metrics
* A **coverage planner** generating deployment & routing
* A **visualizer** showing CH, member, sink, and UGV positions

This repository is designed for **research experiments**, especially on charging scheduling, routing strategies, connectivity robustness, and battery/weather interactions.

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
* **Hop-by-hop routing** through CH backbone
* **Charging request logic**
* **Charging session execution**
* **Failure detection** (battery dead event)

### ✔ UGV Charger with multiple scheduling policies

`ugv_charger_node` supports:

* FCFS
* Role-priority (CH > member)
* EDF (earliest battery depletion first)
* Dynamic weighted scoring
* Network-based *charge decision delivery* using control packets

The UGV tracks UAV status, queues requests, assigns slots, and emulates charging.

### ✔ Coverage Planner (deployment + routing)

`coverage_planner_node`:

* Randomly generates **sink** and **UGV** positions inside the area
* Places CHs on a **grid layout** (or later hex layout)
* Assigns member UAVs to nearest CH
* Computes **CH backbone connectivity graph**
* Runs **Dijkstra from sink** to compute `next_hop_to_sink`
* Publishes `UavDeployment` messages with:

  * Position
  * Role
  * Cluster ID
  * CH ID
  * Next hop information

### ✔ Traffic forwarding framework

Fully working multi-hop routing:

* Member UAV → its CH
* CH → next CH → … → sink
* CH used as routing hubs
* Supports control traffic (charging decisions)

### ✔ Network Monitor

`network_monitor_node` computes:

* Packet generation count
* Packet delivery count
* End-to-end delay
* Average delay
* Charging request timestamps
* Charging wait times
* Charging session counts
* Battery death count & timestamps

Useful for experiments & performance comparison.

### ✔ Planner Visualization (2D GUI)

Python `planner_viz_node` dynamically displays:

* CH UAVs (red + dashed coverage circles)
* Member UAVs (green)
* Sink (blue)
* UGV (yellow)
* Auto-updates positions on every deployment message

### ✔ Sink Gateway Node

Handles:

* Delivery of packets addressed to the sink
* Publishing `/network/traffic_delivered` for monitoring
* Listening to deployment updates for visualization

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
