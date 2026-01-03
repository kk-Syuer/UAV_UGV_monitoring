string msg_id               # optional human-readable id for logging
string src_id               # "uav_1", "user_3", "ugv_1", "sink"
string dst_id               # final destination (who should ultimately receive it)
string next_hop_id          # for this hop, who we are sending to (empty for broadcast)
string last_hop_id          # who forwarded this packet most recently

uint8 flow_type             # 0=DATA, 1=CONTROL
string control_type         # NONE, HELLO, HEARTBEAT, CHARGE_REQUEST, CHARGE_DECISION, etc.
uint32 seq                  # sequence number (per src + control_type)
uint32 hop_count
uint32 ttl                  # max hops allowed (0 = unlimited)
bool requires_ack           # whether the receiver should ACK this control message

string payload              # serialized application payload (JSON, CSV, plain text, ...)
builtin_interfaces/Time creation_time
builtin_interfaces/Time last_tx_time   # when the previous hop transmitted
builtin_interfaces/Time last_rx_time   # when this hop received it
string drop_reason                     # if control_type == DROP, why it was dropped
string[] recent_hops                   # history of forwarders (optional, bounded by producer)
