📌 Project Overview

This project simulates a multi-UAV ad hoc network equipped with:
Cluster Head (CH) backbone routing
Member UAVs & mobile users connecting to CHs
A UGV charger serving all UAVs with limited energy
Battery drain influenced by environmental temperature
Full multi-hop network traffic simulation
Charging scheduling algorithms for fleet sustainability
Network performance monitoring
This system is designed for research on:
UAV survivability under energy constraints
Scheduling fairness & priority
Routing reliability in dynamic ad hoc networks
CH-failure recovery (future work)
Coverage planning & connectivity

It forms the basis for a complete thesis project in networked robotic systems.

📡 System Architecture
Nodes
Component	Description
uav_fleet/	UAV node (both members & CHs) with routing, battery, traffic generation
ugv_charger/	Central ground charger with scheduling queue & charging decisions
network_monitor/	Global observer for delay, throughput, delivery ratio
weather_server/	Generates environmental temperature affecting battery drain
ch_manager/	(Future) automated CH assignment & cluster management
user_devices_sim/	Simulated mobile users sending traffic via nearest CH
Message Types (uav_msgs)

UavStatus.msg

Heartbeat.msg

TrafficMessage.msg (used for ALL routed packets)

ClusterInfo.msg

ChargeRequest.msg

ChargeDecision.msg

WeatherStatus.msg

🔧 Technical Implementation Summary
✔ Routing
Per-destination routing tables (routing_rules)
Multi-hop forwarding by CHs
All traffic (data + control) uses the same routing plane
Dead UAVs (battery=0) stop routing and generating traffic

✔ Energy Model
Separate drain rates for Members vs CHs
Temperature-adjusted drain (weather factor)
Full charging model (duration, interpolation)
Charging disables traffic generation

✔ Charging Scheduling Policies
FCFS
Role Priority (CH > Member)
EDF (earliest depletion first)
Dynamic Score (role + battery + wait time)
Hybrid policies planned

✔ Control Plane Integration
Charging requests & decisions are routed network packets:
UAV → CH → … → UGV (CHARGE_REQUEST)
UGV → CH → … → UAV (CHARGE_DECISION)

✔ Network Monitor
Computes:
delivered / generated packets
average delay
hop count
shows CHG-REQ propagation
detects dead UAVs (indirectly)

📁 Suggested Folder Structure
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
└── README.md (this file)

🚀 How to Build
cd UAV_UGV_netmonitoring
colcon build
source install/setup.bash

🧪 How to Run the Simulation (step-by-step)

Below is the exact procedure used during development to test all modules.

1️⃣ Start Weather Server
ros2 run weather_server weather_server_node


Temperature will oscillate over time and influence UAV drain.

2️⃣ Launch UGV Charger

Example:

ros2 run ugv_charger ugv_charger_node \
    -p ugv_id:=ugv \
    -p uplink_ch_id:=uav_3 \
    -p charging_policy:=role_priority \
    -p charging_duration_sec:=20.0

3️⃣ Launch Routing-Enabled UAVs
UAV 3 (CH)
ros2 run uav_fleet uav_node --ros-args \
    -p uav_id:=uav_3 \
    -p role:=1 \
    -p default_dst_id:=sink_gateway \
    -p next_hop_to_sink:=sink_gateway \
    -p ugv_id:=ugv \
    -p routing_rules:="[sink_gateway:sink_gateway, ugv:ugv]"

UAV 2 (CH)
ros2 run uav_fleet uav_node --ros-args \
    -p uav_id:=uav_2 \
    -p role:=1 \
    -p default_dst_id:=sink_gateway \
    -p next_hop_to_sink:=uav_3 \
    -p ugv_id:=ugv \
    -p routing_rules:="[sink_gateway:uav_3, ugv:uav_3, uav_1:uav_1]"

UAV 1 (Member)
ros2 run uav_fleet uav_node --ros-args \
    -p uav_id:=uav_1 \
    -p role:=0 \
    -p default_dst_id:=sink_gateway \
    -p my_ch_id:=uav_2 \
    -p ugv_id:=ugv \
    -p routing_rules:="[sink_gateway:uav_2, ugv:uav_2]"

4️⃣ Launch a Simulated User Device
ros2 run user_devices_sim user_device_node \
    -p user_id:=user_1 \
    -p cluster_id:=cluster_1
    
5️⃣ Start the Network Monitor
ros2 run network_monitor network_monitor_node


You should see:

GEN (generated messages)
DEL (delivered messages)
CHG-REQ (charging requests entering routing plane)

✔ Expected Output (Correct Behavior)
UAV sends charging request when battery < threshold
CH forwards request toward UGV
UGV logs queue insertion
UGV sends CHARGE_DECISION
CH forwards decision down to the UAV
UAV starts charging session
No new traffic generated during charging
Dead UAVs (battery = 0) stop all activity
If all these occur, system is functioning correctly.

📝 TODO — Remaining Work (Roadmap)
Phase 5 — Routing Improvements

✔ Per-destination rules
⬜ Auto-generate routing tables based on CH positions
⬜ Implement dynamic re-routing after CH failure
⬜ Repair member → CH re-attachment mechanism

Phase 6 — Coverage Planner
⬜ Compute CH positions automatically
⬜ Assign member UAVs to nearest CH
⬜ Validate coverage radius
⬜ Visualization tools (optional)

Phase 7 — Experimentation Framework
⬜ Automated batch experiments
⬜ CSV logging for metrics
⬜ Failure-injection events (CH death, link down)

Phase 8 — Extended Energy Model
⬜ Movement energy cost
⬜ Altitude effect
⬜ Battery degradation
⬜ Weather wind model

Phase 9 — Hybrid Charging Policies
⬜ Combine EDF + Dynamic Score
⬜ Preemption support
⬜ Compare policies under stress

Phase 10 — Thesis Final Experiments
⬜ Quantitative comparison between policies
⬜ Survival rates
⬜ Network stability under failures
⬜ Coverage recovery delays

📚 License

MIT License (or add your own)
