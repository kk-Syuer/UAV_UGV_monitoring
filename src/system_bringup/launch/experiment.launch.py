#!/usr/bin/env python3

import os
import yaml

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory


def _load_yaml(path: str, fallback_root: str | None = None) -> dict:
    if not path:
        return {}
    resolved = path
    if not os.path.exists(resolved) and fallback_root and not os.path.isabs(path):
        resolved = os.path.join(fallback_root, path)
        if not os.path.exists(resolved):
            prefix = "system_bringup" + os.sep
            if path.startswith(prefix):
                resolved = os.path.join(fallback_root, path[len(prefix):])
    if not os.path.exists(resolved):
        return {}
    with open(resolved, "r") as f:
        return yaml.safe_load(f) or {}


def _bool_from_str(s: str, default: bool) -> bool:
    if s is None:
        return default
    if isinstance(s, bool):
        return s
    return str(s).strip().lower() in ("1", "true", "yes", "on")


def _get(d: dict, keys: list, default=None):
    cur = d
    for k in keys:
        if not isinstance(cur, dict) or k not in cur:
            return default
        cur = cur[k]
    return cur


def _make_nodes(context, *args, **kwargs):
    # Launch args
    cfg_path = LaunchConfiguration("config").perform(context)
    run_id_arg = LaunchConfiguration("run_id").perform(context)
    out_dir_arg = LaunchConfiguration("output_dir").perform(context)
    use_weather_arg = LaunchConfiguration("use_weather").perform(context)
    use_injector_arg = LaunchConfiguration("use_fault_injector").perform(context)

    cfg = _load_yaml(cfg_path, fallback_root=get_package_share_directory("system_bringup"))

    # Resolve run_id / output_dir
    run_id = run_id_arg if run_id_arg else _get(cfg, ["global", "run_id"], "run0")
    output_dir = out_dir_arg if out_dir_arg else _get(cfg, ["global", "output_dir"], "/tmp/fanet_logs")

    # Optional overrides
    use_weather = _bool_from_str(use_weather_arg, _get(cfg, ["weather", "enable"], True))
    use_injector = _bool_from_str(use_injector_arg, _get(cfg, ["fault_injector", "enable"], True))

    # Package + executable names (adjust only if your exec names differ)
    # You said most nodes live in their own packages; bringup just launches them.
    nodes = []

    # Common network params
    neighbor_timeout = float(_get(cfg, ["network", "neighbor_timeout_sec"], 3.0))
    comm_radius = float(_get(cfg, ["network", "comm_radius_m"], 400.0))
    ch_ids = _get(cfg, ["uavs", "ch_ids"], []) or []
    members = _get(cfg, ["uavs", "members"], []) or []

    # Monitor
    monitor_params = {
        "run_id": run_id,
        "output_dir": output_dir,
        "flush_period_sec": float(_get(cfg, ["monitor", "flush_period_sec"], 2.0)),
        "status_sample_period_sec": float(_get(cfg, ["monitor", "status_sample_period_sec"], 1.0)),
        "max_runtime_sec": float(_get(cfg, ["monitor", "max_runtime_sec"], 0.0)),
        "stop_on_backbone_loss": bool(_get(cfg, ["monitor", "stop_on_backbone_loss"], False)),
        "backbone_ids": ch_ids,
    }
    nodes.append(Node(
        package=_get(cfg, ["executables", "monitor_pkg"], "network_monitor"),  # change if needed
        executable=_get(cfg, ["executables", "monitor_exec"], "network_monitor_node"),
        name=f"network_monitor_{run_id}",
        output="screen",
        parameters=[monitor_params],
    ))

    # Weather
    if use_weather:
        weather_params = {
            "publish_period_sec": float(_get(cfg, ["weather", "publish_period_sec"], 1.0)),
            "wind_mean": float(_get(cfg, ["weather", "wind_mean"], 0.0)),
            "rain_mean": float(_get(cfg, ["weather", "rain_mean"], 0.0)),
            "temp_c": float(_get(cfg, ["weather", "temp_c"], 18.0)),
        }
        nodes.append(Node(
            package=_get(cfg, ["executables", "weather_pkg"], "weather_server"),
            executable=_get(cfg, ["executables", "weather_exec"], "weather_node"),
            name=f"weather_{run_id}",
            output="screen",
            parameters=[weather_params],
        ))

    # Fault injector
    if use_injector:
        inj = cfg.get("fault_injector", {})
        injector_params = {
            "enable_fault_injector": bool(_get(cfg, ["fault_injector", "enable"], True)),
            "drop_mode": str(_get(cfg, ["fault_injector", "drop_mode"], "bus")),
            "p0": float(inj.get("p0", 0.0)),
            "aw": float(inj.get("aw", 0.25)),
            "ar": float(inj.get("ar", 0.35)),
            "at": float(inj.get("at", 0.10)),
            "p_max": float(inj.get("p_max", 0.6)),
            "wind_max": float(inj.get("wind_max", 20.0)),
            "rain_max": float(inj.get("rain_max", 1.0)),
            "temp_opt": float(inj.get("temp_opt", 18.0)),
            "temp_span": float(inj.get("temp_span", 20.0)),
            "drop_control_multiplier": float(inj.get("drop_control_multiplier", 0.2)),
            "drop_data_multiplier": float(inj.get("drop_data_multiplier", 1.0)),
        }
        nodes.append(Node(
            package=_get(cfg, ["executables", "injector_pkg"], "fanet_faults"),
            executable=_get(cfg, ["executables", "injector_exec"], "fault_injector_node"),
            name=f"fault_injector_{run_id}",
            output="screen",
            parameters=[injector_params],
        ))

    # Sink
    sink_id = str(_get(cfg, ["sink", "sink_id"], "sink_gateway"))
    sink_uplink_ch_id = str(_get(cfg, ["sink", "uplink_ch_id"], ""))
    sink_params = {"sink_id": sink_id}
    if sink_uplink_ch_id:
        sink_params["uplink_ch_id"] = sink_uplink_ch_id
    nodes.append(Node(
        package=_get(cfg, ["executables", "sink_pkg"], "sink_gateway"),
        executable=_get(cfg, ["executables", "sink_exec"], "sink_gateway_node"),
        name=f"sink_gateway_{run_id}",
        output="screen",
        parameters=[sink_params],
    ))

    # UGV charger
    ugv_id = str(_get(cfg, ["ugv", "ugv_id"], "ugv_1"))
    ugv_params = {
        "ugv_id": ugv_id,
    }
    ugv_pkg = _get(cfg, ["executables", "ugv_pkg"], "ugv_charger")
    ugv_exec = _get(cfg, ["executables", "ugv_exec"], "ugv_charger_node")
    nodes.append(Node(
        package=ugv_pkg,
        executable=ugv_exec,
        name=f"ugv_charger_{run_id}",
        output="screen",
        parameters=[ugv_params],
    ))

    # Coverage planner (optional)
    if _bool_from_str(_get(cfg, ["coverage_planner", "enable"], True), True):
        member_ids = [str(m.get("id")) for m in members]
        uav_ids = [str(ch_id) for ch_id in ch_ids] + member_ids
        nodes.append(Node(
            package=_get(cfg, ["executables", "planner_pkg"], "coverage_planner"),
            executable=_get(cfg, ["executables", "planner_exec"], "coverage_planner_node"),
            name=f"coverage_planner_{run_id}",
            output="screen",
            parameters=[{
                "uav_ids": uav_ids,
                "num_ch": len(ch_ids),
                "ugv_id": ugv_id,
            }],
        ))

    # Fleet visualizer (optional)
    if _bool_from_str(_get(cfg, ["planner_viz", "enable"], True), True):
        nodes.append(Node(
            package=_get(cfg, ["executables", "planner_viz_pkg"], "planner_viz"),
            executable=_get(cfg, ["executables", "planner_viz_exec"], "fleet_viz"),
            name=f"fleet_viz_{run_id}",
            output="screen",
            parameters=[{}],
        ))

    # Cluster head manager(s) (optional)
    if _bool_from_str(_get(cfg, ["ch_manager", "enable"], True), True):
        clusters = _get(cfg, ["ch_manager", "clusters"], []) or []
        if clusters:
            cluster_configs = clusters
        else:
            cluster_prefix = str(_get(cfg, ["ch_manager", "cluster_prefix"], "cluster"))
            member_map: dict[str, list[str]] = {}
            for member in members:
                my_ch = str(member.get("my_ch_id", ""))
                member_map.setdefault(my_ch, []).append(str(member.get("id")))
            cluster_configs = [
                {
                    "cluster_id": f"{cluster_prefix}_{idx + 1}",
                    "ch_id": str(ch_id),
                    "member_ids": member_map.get(str(ch_id), []),
                }
                for idx, ch_id in enumerate(ch_ids)
            ]
        for cfg_entry in cluster_configs:
            nodes.append(Node(
                package=_get(cfg, ["executables", "ch_manager_pkg"], "ch_manager"),
                executable=_get(cfg, ["executables", "ch_manager_exec"], "ch_manager_node"),
                name=f"ch_manager_{cfg_entry['cluster_id']}",
                output="screen",
                parameters=[{
                    "cluster_id": cfg_entry["cluster_id"],
                    "ch_id": cfg_entry["ch_id"],
                    "member_ids": cfg_entry["member_ids"],
                }],
            ))

    # User device traffic (optional)
    if _bool_from_str(_get(cfg, ["traffic", "user_device_enable"], True), True):
        nodes.append(Node(
            package=_get(cfg, ["executables", "user_device_pkg"], "user_device"),
            executable=_get(cfg, ["executables", "user_device_exec"], "user_device_node"),
            name=f"user_device_{run_id}",
            output="screen",
            parameters=[{}],
        ))

    # UAV params shared
    task_tlm = cfg.get("task_telemetry", {})
    shared_uav_params = {
        "neighbor_timeout_sec": neighbor_timeout,
        "comm_radius_m": comm_radius,
        # task telemetry knobs (members generate telemetry on task reach)
        "task_telemetry_enable": bool(task_tlm.get("enable", True)),
        "task_telemetry_packets_min": int(task_tlm.get("packets_min", 1)),
        "task_telemetry_packets_max": int(task_tlm.get("packets_max", 3)),
        "task_telemetry_payload_bytes_min": int(task_tlm.get("payload_bytes_min", 50)),
        "task_telemetry_payload_bytes_max": int(task_tlm.get("payload_bytes_max", 200)),
        "task_telemetry_send_prob": float(task_tlm.get("send_prob", 1.0)),
        "task_telemetry_ttl": int(task_tlm.get("ttl_hops", 16)),
        "task_telemetry_flow_label": str(task_tlm.get("flow_label", "SEARCH_TELEMETRY")),
    }

    uav_pkg = _get(cfg, ["executables", "uav_pkg"], "uav_fleet")
    uav_exec = _get(cfg, ["executables", "uav_exec"], "uav_node")

    # CH UAVs
    for ch_id in ch_ids:
        nodes.append(Node(
            package=uav_pkg,
            executable=uav_exec,
            name=f"{ch_id}_{run_id}",
            output="screen",
            parameters=[shared_uav_params, {"uav_id": str(ch_id), "role": 1}],
        ))

    # Member UAVs
    for m in members:
        uid = str(m.get("id"))
        role = int(m.get("role", 0))
        my_ch = str(m.get("my_ch_id", ""))
        nodes.append(Node(
            package=uav_pkg,
            executable=uav_exec,
            name=f"{uid}_{run_id}",
            output="screen",
            parameters=[shared_uav_params, {"uav_id": uid, "role": role, "my_ch_id": my_ch}],
        ))

    return nodes


def generate_launch_description():
    bringup_share = get_package_share_directory("system_bringup")
    default_cfg = os.path.join(bringup_share, "config", "runs", "example_run.yaml")

    return LaunchDescription([
        DeclareLaunchArgument("config", default_value=default_cfg),
        DeclareLaunchArgument("run_id", default_value=""),
        DeclareLaunchArgument("output_dir", default_value=""),
        DeclareLaunchArgument("use_weather", default_value=""),
        DeclareLaunchArgument("use_fault_injector", default_value=""),
        OpaqueFunction(function=_make_nodes),
    ])
