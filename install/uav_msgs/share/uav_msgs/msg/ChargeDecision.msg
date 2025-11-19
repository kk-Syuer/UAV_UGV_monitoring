string uav_id                         # which UAV this decision is for
bool accepted                         # whether it got a charging slot
builtin_interfaces/Time slot_start_time  # when it should start charging
uint8 priority                        # priority level (e.g. 0=member,1=CH)
string policy                         # policy name used (fcfs, role_priority, ...)
