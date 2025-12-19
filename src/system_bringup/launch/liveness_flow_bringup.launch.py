from launch import LaunchDescription
from launch_ros.actions import Node


def create_uav_nodes(prefix: str, count: int, role_value: int, extra_params=None):
    """Create a batch of UAV nodes with a shared role and parameter set."""
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


def create_ch_managers(cluster_configs):
    """Spin up CH manager nodes per cluster configuration."""
    nodes = []
    for cfg in cluster_configs:
        nodes.append(
            Node(
                package="ch_manager",
                executable="ch_manager_node",
                name=f"ch_manager_{cfg['cluster_id']}",
                output="screen",
                parameters=[{
                    "cluster_id": cfg["cluster_id"],
                    "ch_id": cfg["ch_id"],
                    "member_ids": cfg["member_ids"],
                }],
            )
        )
    return nodes


def generate_launch_description():
    """Launch a liveness flow with CHs, members, and monitoring nodes."""
    nodes = []

    # Two CHs with four members split across clusters.
    ch_ids = [f"uav_ch_{idx}" for idx in range(1, 3)]
    mem_ids = [f"uav_mem_{idx}" for idx in range(1, 5)]
    all_uavs = ch_ids + mem_ids

    cluster_configs = [
        {"cluster_id": "cluster_1", "ch_id": "uav_ch_1", "member_ids": mem_ids[0:2]},
        {"cluster_id": "cluster_2", "ch_id": "uav_ch_2", "member_ids": mem_ids[2:4]},
    ]

    # Cluster-head and member UAVs emitting heartbeats and failure events
    nodes.extend(create_uav_nodes("uav_ch", len(ch_ids), role_value=1))
    nodes.extend(
        create_uav_nodes(
            "uav_mem",
            len(mem_ids),
            role_value=0,
            extra_params={"auto_traffic_enabled": False},
        )
    )

    # Sink gateway coordinating deployment and forwarding heartbeats
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

    # Cluster managers listen for failures to validate liveness handling
    nodes.extend(create_ch_managers(cluster_configs))

    # Network monitor to log traffic, charging, and failure telemetry
    nodes.append(
        Node(
            package="network_monitor",
            executable="network_monitor_node",
            name="network_monitor",
            output="screen",
        )
    )

    # Fleet visualizer to observe movement after deployment flag
    nodes.append(
        Node(
            package="planner_viz",
            executable="fleet_viz",
            name="fleet_viz",
            output="screen",
        )
    )

    # Coverage planner seeds positions and tracks deployment acknowledgments (launched after UAVs and visualizer)
    nodes.append(
        Node(
            package="coverage_planner",
            executable="coverage_planner_node",
            name="coverage_planner",
            output="screen",
            parameters=[{
                "uav_ids": all_uavs,
                "num_ch": len(ch_ids),
                "planner_id": "liveness_flow_test",
            }],
        )
    )

    return LaunchDescription(nodes)
