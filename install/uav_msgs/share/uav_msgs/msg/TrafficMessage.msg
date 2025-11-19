string msg_id               # unique per message
string src_id               # "uav_1", "user_3", "ugv", "sink_gateway"
string dst_id               # same style

uint8 msg_type              # 0=TEXT, 1=IMAGE, 2=VIDEO_CHUNK, 3=CONTROL_ALERT
uint8 priority              # 0=low ... 3=emergency (you decide later)
uint32 size_bytes

builtin_interfaces/Time creation_time
uint32 hop_count
