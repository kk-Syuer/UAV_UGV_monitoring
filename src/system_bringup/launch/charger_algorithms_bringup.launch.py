from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def create_uav_nodes(prefix: str, count: int, role_value: int, extra_params=None):
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


def generate_launch_description():
    charging_policy = LaunchConfiguration("charging_policy")
    charging_duration = LaunchConfiguration("charging_duration_sec")

    declared_arguments = [
        DeclareLaunchArgument(
            "charging_policy",
            default_value="fcfs",
            description=(
                "UGV charging policy to exercise (fcfs, role_priority, edf, dynamic)"
            ),
        ),
        DeclareLaunchArgument(
            "charging_duration_sec",
            default_value="20.0",
            description="Simulated charging duration per UAV in seconds",
        ),
    ]

    nodes = []

    # Cluster-head UAVs (role 1)
    nodes.extend(create_uav_nodes("uav_ch", 3, role_value=1))

    # Member UAVs (role 0)

    nodes.extend(
        create_uav_nodes(
            "uav_mem",
            4,
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

    # UGV charger node with configurable policy
    nodes.append(
        Node(
            package="ugv_charger",
            executable="ugv_charger_node",
            name="ugv_charger",
            namespace="ugv",
            output="screen",
            parameters=[
                {
                    "charging_policy": charging_policy,
                    "charging_duration_sec": charging_duration,
                }
            ],
        )
    )

    # Network monitor to observe charge requests and assignments
    nodes.append(
        Node(
            package="network_monitor",
            executable="network_monitor_node",
            name="network_monitor",
            output="screen",
        )
    )

    # Weather server to supply environmental data to UAVs
    nodes.append(
        Node(
            package="weather_server",
            executable="weather_node",
            name="weather_server",
            output="screen",
        )
    )

    return LaunchDescription(declared_arguments + nodes)
