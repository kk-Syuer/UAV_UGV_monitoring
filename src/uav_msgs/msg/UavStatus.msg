string uav_id               # "uav_1", "uav_2", ...
uint8 role                  # 0 = MEMBER, 1 = CH, 2 = BACKUP_CH
string cluster_id           # e.g. "cluster_1"

float32 battery_level       # 0.0 - 100.0 (%)
float32 battery_capacity    # optional, in Wh or similar

geometry_msgs/Pose pose
geometry_msgs/Twist velocity

float32 service_radius      # meters (CH > MEMBER)

uint32 connected_users      # number of user devices associated
float32 traffic_load        # e.g. kbps or some abstract load

float32 packet_loss_estimate
float32 energy_consumption_rate
uint8 charging_state          # 0=ACTIVE,1=GOING_TO_UGV,2=CHARGING,3=RETURNING
bool intent_to_leave          # true if battery below threshold or queued
float32 eta_to_leave_sec
float32 comm_radius_m         # so receivers can simulate range consistently

builtin_interfaces/Time stamp
bool backbone_active
