from launch import LaunchDescription
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
    nodes = []

    ch_ids = [f"uav_ch_{idx}" for idx in range(1, 4)]
    mem_ids = [f"uav_mem_{idx}" for idx in range(1, 5)]
    all_uavs = ch_ids + mem_ids

    # Bring up UAVs used by the planner
    nodes.extend(create_uav_nodes("uav_ch", len(ch_ids), role_value=1))
    nodes.extend(
        create_uav_nodes(
            "uav_mem",
            len(mem_ids),
            role_value=0,
            extra_params={"auto_traffic_enabled": False},
        )
    )

    # Sink node to coordinate deployment acknowledgments
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

    # Visualizer to inspect deployment outputs
    nodes.append(
        Node(
            package="planner_viz",
            executable="fleet_viz",
            name="fleet_viz",
            output="screen",
        )
    )

    # Network monitor for telemetry
    nodes.append(
        Node(
            package="network_monitor",
            executable="network_monitor_node",
            name="network_monitor",
            output="screen",
        )
    )

    # Coverage planner to compute initial deployments (launched after UAVs and visualizer)
    nodes.append(
        Node(
            package="coverage_planner",
            executable="coverage_planner_node",
            name="coverage_planner",
            output="screen",
            parameters=[{
                "uav_ids": all_uavs,
                "num_ch": len(ch_ids),
                "planner_id": "coverage_planner_init_test",
            }],
        )
    )

    return LaunchDescription(nodes)
