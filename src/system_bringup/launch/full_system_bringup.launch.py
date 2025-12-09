from launch import LaunchDescription
from launch_ros.actions import Node


def create_uav_nodes(prefix: str, count: int, role_value: int):
    nodes = []
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
                }],
            )
        )
    return nodes


def create_user_device_nodes(count: int):
    nodes = []
    for idx in range(1, count + 1):
        device_id = f"user_dev_{idx}"
        nodes.append(
            Node(
                package="user_devices_sim",
                executable="user_device_node",
                name=device_id,
                namespace=device_id,
                output="screen",
                parameters=[{
                    "user_id": device_id,
                }],
            )
        )
    return nodes


def generate_launch_description():
    nodes = []

    # Cluster-head UAVs (role 1)
    nodes.extend(create_uav_nodes("uav_ch", 3, role_value=1))

    # Member UAVs (role 0)
    nodes.extend(create_uav_nodes("uav_mem", 4, role_value=0))

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

    # UGV charger node
    nodes.append(
        Node(
            package="ugv_charger",
            executable="ugv_charger_node",
            name="ugv_charger",
            namespace="ugv",
            output="screen",
        )
    )

    # Fleet visualizer
    nodes.append(
        Node(
            package="planner_viz",
            executable="fleet_viz",
            name="fleet_viz",
            output="screen",
        )
    )

    # Weather server
    nodes.append(
        Node(
            package="weather_server",
            executable="weather_node",
            name="weather_server",
            output="screen",
        )
    )

    # Network monitor
    nodes.append(
        Node(
            package="network_monitor",
            executable="network_monitor_node",
            name="network_monitor",
            output="screen",
        )
    )

    # User devices
    nodes.extend(create_user_device_nodes(3))

    return LaunchDescription(nodes)
