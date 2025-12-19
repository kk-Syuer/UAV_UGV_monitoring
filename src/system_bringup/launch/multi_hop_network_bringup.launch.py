from launch import LaunchDescription
from launch.actions import ExecuteProcess, TimerAction
from launch_ros.actions import Node


def create_uav_nodes(prefix: str, count: int, role_value: int, extra_params=None):
    """Helper to spawn UAV nodes with consistent naming and parameters."""
    nodes = []
    extra_params = extra_params or {}
    for idx in range(1, count + 1):
        uav_id = f"{prefix}_{idx}"
        nodes.append(
            Node(
                package="uav_fleet",
                executable="uav_node",
                name=uav_id,
                namespace=uav_id,
                output="screen",
                parameters=[{
                    "uav_id": uav_id,
                    "role": role_value,
                    **extra_params,
                }],
            )
        )
    return nodes


def create_user_devices(count: int):
    """Spawn simulated user devices that generate traffic."""
    nodes = []
    for idx in range(1, count + 1):
        dev_id = f"user_dev_{idx}"
        nodes.append(
            Node(
                package="user_devices_sim",
                executable="user_device_node",
                name=dev_id,
                namespace=dev_id,
                output="screen",
                parameters=[{"user_id": dev_id}],
            )
        )
    return nodes


def generate_launch_description():
    """Launch a multi-hop networking test with UAVs, UGV, and users."""
    nodes = []

    # Four CHs form a backbone with members and user devices.
    ch_ids = [f"uav_ch_{idx}" for idx in range(1, 5)]
    mem_ids = [f"uav_mem_{idx}" for idx in range(1, 7)]
    all_uavs = ch_ids + mem_ids

    # UAV nodes representing cluster heads and members
    nodes.extend(create_uav_nodes("uav_ch", len(ch_ids), role_value=1))
    nodes.extend(
        create_uav_nodes(
            "uav_mem",
            len(mem_ids),
            role_value=0,
            extra_params={"auto_traffic_enabled": False},
        )
    )

    # Sink node
    nodes.append(
        Node(
            package="sink_gateway",
            executable="sink_gateway_node",
            name="sink",
            namespace="sink",
            output="screen",
            parameters=[{"sink_id": "sink_gateway"}],
        )
    )

    # UGV charger to participate in network relays
    nodes.append(
        Node(
            package="ugv_charger",
            executable="ugv_charger_node",
            name="ugv_charger",
            namespace="ugv",
            output="screen",
        )
    )

    # Network monitor for observing link quality and routes
    nodes.append(
        Node(
            package="network_monitor",
            executable="network_monitor_node",
            name="network_monitor",
            output="screen",
        )
    )

    # User devices generating traffic over the network
    nodes.extend(create_user_devices(4))

    # Coverage planner to position UAVs for multi-hop networking (launched after UAVs)
    nodes.append(
        Node(
            package="coverage_planner",
            executable="coverage_planner_node",
            name="coverage_planner",
            output="screen",
            parameters=[{
                "uav_ids": all_uavs,
                "num_ch": len(ch_ids),
                "planner_id": "multi_hop_test",
            }],
        )
    )

    # Inject a debug packet after startup to observe multi-hop routing
    nodes.append(
        TimerAction(
            period=8.0,
            actions=[
                ExecuteProcess(
                    cmd=[
                        "ros2",
                        "service",
                        "call",
                        f"/uav_fleet/{ch_ids[0]}/send_debug_text",
                        "uav_msgs/srv/SendDebugText",
                        "{dst_id: 'sink_gateway', text: 'multi-hop debug trace'}",
                    ],
                    output="screen",
                )
            ],
        )
    )

    return LaunchDescription(nodes)
