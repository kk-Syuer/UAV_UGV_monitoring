## 📌 **Overview**

This project implements a complete simulation environment for a **heterogeneous aerial–ground network** consisting of:

* **Cluster-Head (CH) UAVs** forming a multi-hop backbone
* **Member UAVs** generating periodic traffic
* **Mobile user devices** sending data through the UAV network
* **A UGV charger** that handles energy scheduling and maintains fleet survivability
* **A weather server** influencing battery drain
* **A network monitor** tracking performance metrics

The system is built for research on:

* Energy-aware scheduling
* Routing reliability and multi-hop ad hoc networking
* UAV survivability
* Charging fairness & performance trade-offs
* CH failure handling (future)
* Dynamic coverage planning (future)

---

## 🏗 **System Architecture**

### **Core Components**

| Package              | Purpose                                                           |
| -------------------- | ----------------------------------------------------------------- |
| **uav_fleet**        | UAV node logic, routing, battery model, control-plane integration |
| **ugv_charger**      | Charger logic, scheduling queue, EDF/priority/dynamic policies    |
| **network_monitor**  | Collects GEN/DEL/CHG-REQ stats, delay, hop counts                 |
| **weather_server**   | Publishes temperature affecting battery drain                     |
| **user_devices_sim** | Simulated mobile phones sending uplink traffic                    |
| **ch_manager**       | (Optional / future) dynamic clustering and CH assignment          |
| **uav_msgs**         | Shared ROS 2 custom message definitions                           |

---

## 📡 **Routing Model**

* CHs operate as routers.
* Members forward to their CH.
* CHs forward based on a **per-destination routing table** (`routing_rules`).
* All packets (data + control) traverse **the same routed network plane**.
* Charging signals (`CHARGE_REQUEST`, `CHARGE_DECISION`) flow CH → CH → UGV and vice versa.

Each `TrafficMessage` includes:

```
msg_type: TEXT / CONTROL
src_id
dst_id
next_hop_id
hop_count
control_type (optional)
creation_time
```

---

## 🔋 **Energy Model**

* CHs have **larger battery capacity**.
* Drain depends on:

  * role (CH vs Member)
  * temperature (from weather server)
* Charging:

  * Not instantaneous
  * Per-UAV charging sessions with known duration
  * Charging interpolates battery level over time
  * UAV stops generating or forwarding traffic while charging

---

## 🔌 **Charging Scheduling**

UGV supports multiple policies:

* **FCFS**
* **Role Priority** (CH > member)
* **EDF** (earliest depletion first)
* **Dynamic Score**:
  `score = w_role*role  + w_batt*(100 - batt%)  + w_wait*waiting_time`

Routing-delivered **CHARGE_REQUEST** causes UGV queueing.
Routing-delivered **CHARGE_DECISION** triggers UAV charging.

---

## 📊 **Network Monitor**

Tracks:

* Packets generated
* Packets delivered
* Average end-to-end delay
* Per-message hop count
* Charging-related control-plane events

Acts as a ground-truth observer for experiments.

---

## 📁 **Folder Structure**

```
UAV_UGV_netmonitoring/
│
├── uav_fleet/
├── ugv_charger/
├── network_monitor/
├── user_devices_sim/
├── weather_server/
├── ch_manager/
│
├── uav_msgs/
│   ├── msg/
│   ├── srv/
│   └── action/
│
└── README.md
```

---

# 🚀 **Build Instructions**

```bash
cd UAV_UGV_netmonitoring
colcon build
source install/setup.bash
```

---

# 🧪 **How to Run the Simulation**

Below is the minimal working configuration we verified together.

---

## **1️⃣ Start Weather Server**

```bash
ros2 run weather_server weather_server_node
```

---

## **2️⃣ Launch UGV Charger**

```bash
ros2 run ugv_charger ugv_charger_node --ros-args \
    -p ugv_id:=ugv \
    -p uplink_ch_id:=uav_3 \
    -p charging_policy:=role_priority \
    -p charging_duration_sec:=20.0
```

---

## **3️⃣ Run UAV Nodes**

### **UAV 3 (CH closest to UGV)**

```bash
ros2 run uav_fleet uav_node --ros-args \
    -p uav_id:=uav_3 \
    -p role:=1 \
    -p default_dst_id:=sink_gateway \
    -p next_hop_to_sink:=sink_gateway \
    -p ugv_id:=ugv \
    -p routing_rules:="[sink_gateway:sink_gateway, ugv:ugv]"
```

### **UAV 2 (CH forwarding to UAV3)**

```bash
ros2 run uav_fleet uav_node --ros-args \
    -p uav_id:=uav_2 \
    -p role:=1 \
    -p default_dst_id:=sink_gateway \
    -p next_hop_to_sink:=uav_3 \
    -p ugv_id:=ugv \
    -p routing_rules:="[sink_gateway:uav_3, ugv:uav_3]"
```

### **UAV 1 (Member served by UAV2)**

```bash
ros2 run uav_fleet uav_node --ros-args \
    -p uav_id:=uav_1 \
    -p role:=0 \
    -p my_ch_id:=uav_2 \
    -p ugv_id:=ugv \
    -p routing_rules:="[sink_gateway:uav_2, ugv:uav_2]"
```

---

## **4️⃣ Start User Device Simulator**

```bash
ros2 run user_devices_sim user_device_node \
    -p user_id:=user_1 \
    -p cluster_id:=cluster_1
```

---

## **5️⃣ Start Network Monitor**

```bash
ros2 run network_monitor network_monitor_node
```

You should now see:

* `[GEN] msg_id=...`
* `[DEL] msg_id=...`
* `[CHG-REQ] ...`
* `[TX CTRL] UAV sending CHARGE_REQUEST`
* `UGV: enqueued …`
* `UGV: sending CHARGE_DECISION`
* `UAV X: starting charging session`

---

# ✔ **Expected Correct Behavior**

* UAV drains battery over time based on temperature
* Automatically sends routed CHARGE_REQUEST
* CH forwards requests to UGV
* UGV schedules charging
* Routed CHARGE_DECISION reaches UAV
* UAV starts charging session
* UAV pauses traffic generation during charging
* Dead UAVs stop forwarding & sending traffic

---

# 📝 **To-Do Roadmap**

### **Routing / Networking**

* [ ] Auto-generate routing tables from CH positions
* [ ] Dynamic re-routing after CH failure
* [ ] Re-assign members to nearest CH (coverage planner)

### **Coverage & Mobility**

* [ ] Implement CH placement algorithm
* [ ] Compute service radius intersection
* [ ] Member clustering

### **Charging Enhancements**

* [ ] Hybrid policies (EDF + dynamic score)
* [ ] Preemption
* [ ] Charging acknowledgments

### **Experimentation**

* [ ] CSV logging (delay, energy, queue size)
* [ ] Automated stressful scenarios
* [ ] Visualizations (RViz / Python)

### **Energy Model**

* [ ] Wind effects
* [ ] Movement-based energy consumption
* [ ] Battery degradation model

---

# 📜 **License**

MIT License 


