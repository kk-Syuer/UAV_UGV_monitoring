string uav_id
geometry_msgs/Pose target_pose


uint8 role        # 0 = MEMBER, 1 = CH
string cluster_id # e.g. "cluster_1"
string ch_id      # for members: their CH; for CHs: self

# Backbone routing: where should this UAV send packets towards the sink?
# For CHs: usually another CH id or "sink_gateway".
# For members: can be empty.
string next_hop_to_sink