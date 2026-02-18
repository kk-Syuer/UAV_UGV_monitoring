import json
import os
from typing import Any, Dict

import rclpy
from rclpy.node import Node

import matplotlib

_headless_env = os.environ.get("FLEET_VIZ_HEADLESS", "").lower()
_headless = _headless_env in ("1", "true", "yes") or not os.environ.get("DISPLAY")
matplotlib.use("Agg" if _headless else "TkAgg")
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
from matplotlib.widgets import Button

from geometry_msgs.msg import Pose
from rclpy.qos import DurabilityPolicy
from rclpy.qos import QoSProfile
from std_msgs.msg import String
from uav_msgs.msg import ClusterInfo
from uav_msgs.msg import FailureEvent
from uav_msgs.msg import TaskPointArray
from uav_msgs.msg import TrafficMessage
from uav_msgs.msg import UavDeployment
from uav_msgs.msg import UavStatus
from uav_msgs.msg import WeatherStatus


class FleetVizNode(Node):
    def __init__(self):
        super().__init__('fleet_viz_node')

        # Subscriptions provide live data for the visualization panels.
        self.status_sub = self.create_subscription(
            UavStatus, '/fanet/status', self.status_cb, 50)

        self.deployment_sub = self.create_subscription(
            UavDeployment, '/coverage_planner/deployment', self.deployment_cb, 20)

        self.weather_sub = self.create_subscription(
            WeatherStatus, '/environment/weather', self.weather_cb, 10)

        task_point_qos = QoSProfile(depth=1, durability=DurabilityPolicy.TRANSIENT_LOCAL)
        self.task_point_sub = self.create_subscription(
            TaskPointArray, '/coverage_planner/task_points', self.task_point_cb, task_point_qos)
        self.traffic_sub = self.create_subscription(
            TrafficMessage, '/fanet/network_bus', self.traffic_cb, 50)
        self.traffic_raw_sub = self.create_subscription(
            TrafficMessage, '/fanet/network_bus_raw', self.traffic_raw_cb, 50)
        self.delivered_sub = self.create_subscription(
            TrafficMessage, '/fanet/delivered', self.delivered_cb, 50)
        self.network_stats_sub = self.create_subscription(
            String, '/network_monitor/stats', self.network_stats_cb, 10)
        self.cluster_info_sub = self.create_subscription(
            ClusterInfo, '/ch_manager/cluster_info', self.cluster_info_cb, 20)
        self.failure_sub = self.create_subscription(
            FailureEvent, '/uav_fleet/failure_events', self.failure_cb, 20)
        self.charging_snapshot_sub = self.create_subscription(
            String, '/ugv/charging_snapshot', self.charging_snapshot_cb, 20)

        # Cached state for plotting and info panels.
        self.uav_states = {}   # id -> last UavStatus
        self.sink_pose = None
        self.ugv_pose = None
        self.weather = None
        self.task_points = []
        self.cluster_colors = {}
        self.cluster_palette = [
            'cyan', 'magenta', 'orange', 'lime', 'yellow', 'plum', 'deepskyblue', 'white'
        ]

        # Queue + scheduling summary (fanet-sourced).
        self.pending_charges: Dict[str, Dict[str, Any]] = {}
        self.latest_policy = 'n/a (fanet)'
        self.last_charge_request_time = None
        self.last_charge_decision_time = None
        self.last_charge_decision_target = None
        self.last_charge_decision_accepted = None
        self.charge_request_ids = set()
        self.charge_decision_ids = set()
        self.charging_snapshot: Dict[str, Any] = {}
        self.last_charging_snapshot_time = None

        # Control-plane snapshots.
        self.cluster_info = {}
        self.last_cluster_info_time = None
        self.last_task_points_time = None
        self.last_task_points_count = 0
        self.last_deployment_time = None
        self.last_deployment_target = None
        self.dead_uavs: Dict[str, Dict[str, Any]] = {}
        self.member_task_assignments: Dict[str, list] = {}

        # Plot setup for the main canvas and info panels.
        plt.ion()
        self.fig = plt.figure(figsize=(14, 8), constrained_layout=True)
        gs = self.fig.add_gridspec(1, 2, width_ratios=[3.3, 1.4])

        # main grid (left column)
        self.ax = self.fig.add_subplot(gs[0, 0])
        self.ax.set_xlabel('X [m]')
        self.ax.set_ylabel('Y [m]')
        self.ax.set_title('Fleet live view')

        # info panel (right column)
        self.info_ax = self.fig.add_subplot(gs[0, 1])
        self.info_ax.axis('off')

        # page navigation for the info panel
        self.info_pages = ['Status', 'Network', 'Charging']
        self.info_page_index = 0
        self.info_scroll = {name: 0.0 for name in self.info_pages}
        self.prev_button = None
        self.next_button = None
        if not _headless:
            self._init_page_buttons()
            self.fig.canvas.mpl_connect('scroll_event', self._on_scroll)

        # dark background for consistent contrast with white text
        self.fig.patch.set_facecolor("#000000")

        # area limits (can be tuned / parameterised)
        self.x_min = -1200.0
        self.x_max = 1200.0
        self.y_min = -1200.0
        self.y_max = 1200.0

        self.comm_radius_ch = float(self.declare_parameter('comm_radius_ch', 400.0).value)
        self.target_utilization = float(self.declare_parameter('target_utilization', 0.8).value)
        self.flight_time_ch_min = float(self.declare_parameter('flight_time_ch_min', 90.0).value)
        self.charge_time_ch_min = float(self.declare_parameter('charge_time_ch_min', 30.0).value)
        self.flight_time_mem_min = float(self.declare_parameter('flight_time_mem_min', 45.0).value)
        self.charge_time_mem_min = float(self.declare_parameter('charge_time_mem_min', 20.0).value)

        # Network stats for debug panels (fed by network_monitor).
        self.monitor_stats = {
            'generated_total': 0,
            'delivered_total': 0,
            'drop_total': 0,
            'ack_total': 0,
            'generated_rate': 0.0,
            'delivered_rate': 0.0,
            'drop_rate': 0.0,
            'window_sec': 10.0,
            'last_msg_age': None,
            'last_drop_age': None,
            'last_delivered_age': None,
            'control_type_counts': {},
            'drop_reason_counts': {},
        }
        self.last_monitor_stats_time = None

        # Timer to refresh plot periodically.
        self.timer = self.create_timer(0.2, self.update_plot)

        self.get_logger().info("FleetVizNode started.")

    def _init_page_buttons(self):
        self.fig.canvas.draw()
        info_pos = self.info_ax.get_position()
        total_width = info_pos.width * 0.7
        gap = info_pos.width * 0.04
        button_width = (total_width - gap) / 2
        button_height = info_pos.height * 0.06
        left = info_pos.x0 + (info_pos.width - total_width) / 2
        bottom = info_pos.y0 + info_pos.height * 0.02
        self.prev_button_ax = self.fig.add_axes([left, bottom, button_width, button_height])
        self.next_button_ax = self.fig.add_axes(
            [left + button_width + gap, bottom, button_width, button_height]
        )
        for ax in (self.prev_button_ax, self.next_button_ax):
            ax.set_facecolor('#111111')
        self.prev_button = Button(self.prev_button_ax, '◀ Prev', color='#111111',
                                  hovercolor='#222222')
        self.next_button = Button(self.next_button_ax, 'Next ▶', color='#111111',
                                  hovercolor='#222222')
        self.prev_button.label.set_color('white')
        self.next_button.label.set_color('white')
        self.prev_button.on_clicked(self._on_prev_page)
        self.next_button.on_clicked(self._on_next_page)

    def _on_prev_page(self, _event):
        self.info_page_index = (self.info_page_index - 1) % len(self.info_pages)

    def _on_next_page(self, _event):
        self.info_page_index = (self.info_page_index + 1) % len(self.info_pages)

    def _on_scroll(self, event):
        if event.inaxes != self.info_ax:
            return
        page_name = self.info_pages[self.info_page_index]
        delta = 1.0 if event.button == 'up' else -1.0
        self.info_scroll[page_name] = max(
            0.0,
            self.info_scroll.get(page_name, 0.0) - delta * 0.6
        )

    # --- callbacks ---

    def status_cb(self, msg: UavStatus):
        """Cache latest UAV status for plotting."""
        self.uav_states[msg.uav_id] = msg
        if msg.battery_level <= 0.0 and msg.uav_id not in self.dead_uavs:
            death_time = msg.stamp.sec + msg.stamp.nanosec * 1e-9
            if death_time <= 0.0:
                death_time = self._now_sec()
            self.dead_uavs[msg.uav_id] = {
                'time': death_time,
                'reason': 'BATTERY_DEAD',
                'description': 'Battery depleted (from status update)',
            }
        if msg.uav_id == 'ugv' or msg.uav_id.startswith('ugv_'):
            self.ugv_pose = msg.pose
        elif msg.uav_id == 'sink_gateway':
            self.sink_pose = msg.pose

    def failure_cb(self, msg: FailureEvent):
        if msg.failure_type != 1:
            return
        death_time = msg.stamp.sec + msg.stamp.nanosec * 1e-9
        if death_time <= 0.0:
            death_time = self._now_sec()
        self.dead_uavs[msg.uav_id] = {
            'time': death_time,
            'reason': 'BATTERY_DEAD',
            'description': msg.description or 'Battery depleted',
        }
        if msg.uav_id in self.pending_charges:
            self.pending_charges.pop(msg.uav_id)

    def deployment_cb(self, msg: UavDeployment):
        """Capture sink/UGV deployments for map anchors."""
        self.last_deployment_time = self._now_sec()
        self.last_deployment_target = msg.uav_id
        if msg.uav_id == 'sink_gateway':
            self.sink_pose = msg.target_pose
        elif msg.uav_id == 'ugv':
            self.ugv_pose = msg.target_pose

    def weather_cb(self, msg: WeatherStatus):
        """Cache current weather for the info panel."""
        self.weather = msg

    def task_point_cb(self, msg: TaskPointArray):
        self.task_points = list(msg.tasks)
        self.member_task_assignments.clear()
        self.last_task_points_time = self._now_sec()
        self.last_task_points_count = len(self.task_points)

    def cluster_info_cb(self, msg: ClusterInfo):
        self.cluster_info[msg.cluster_id] = msg
        self.last_cluster_info_time = self._now_sec()

    def delivered_cb(self, msg: TrafficMessage):
        now = self._now_sec()
        if msg.control_type == 'CHARGE_DECISION':
            if msg.msg_id and msg.msg_id in self.charge_decision_ids:
                return
            if msg.msg_id:
                self.charge_decision_ids.add(msg.msg_id)
            self.last_charge_decision_time = now
            self.last_charge_decision_target = msg.dst_id
            self.last_charge_decision_accepted = 'REJECT' not in (msg.payload or '')
            if msg.dst_id in self.pending_charges:
                self.pending_charges.pop(msg.dst_id)

    def charging_snapshot_cb(self, msg: String):
        try:
            payload = json.loads(msg.data)
        except json.JSONDecodeError as exc:
            self.get_logger().warning(f"Failed to parse charging snapshot: {exc}")
            return
        if not isinstance(payload, dict):
            self.get_logger().warning('Charging snapshot payload must be an object')
            return
        self.charging_snapshot = payload
        self.last_charging_snapshot_time = self._now_sec()

    def color_for_cluster(self, cluster_id: str) -> str:
        """Deterministic color mapping for cluster IDs."""
        if not cluster_id:
            return 'white'
        if cluster_id not in self.cluster_colors:
            color = self.cluster_palette[len(self.cluster_colors) % len(self.cluster_palette)]
            self.cluster_colors[cluster_id] = color
        return self.cluster_colors[cluster_id]

    def traffic_cb(self, msg: TrafficMessage):
        """Track UGV pose from HELLO traffic so we can show motion."""
        if msg.control_type == 'DROP':
            return
        if msg.control_type == 'ACK':
            return
        if msg.control_type == 'CHARGE_REQUEST':
            if msg.msg_id and msg.msg_id in self.charge_request_ids:
                return
            if msg.msg_id:
                self.charge_request_ids.add(msg.msg_id)
            request_time = msg.creation_time.sec + msg.creation_time.nanosec * 1e-9
            if msg.src_id not in self.pending_charges:
                self.pending_charges[msg.src_id] = {
                    'time': request_time,
                    'battery': None,
                    'role': None,
                }
            else:
                self.pending_charges[msg.src_id]['time'] = request_time
            self.last_charge_request_time = request_time
            return
        if msg.control_type == 'CHARGE_DECISION':
            return
        if msg.control_type in ('DEPLOYMENT', 'DEPLOYMENT_CMD'):
            parts = msg.payload.split(',')
            if len(parts) >= 6:
                try:
                    x = float(parts[3])
                    y = float(parts[4])
                    z = float(parts[5])
                except ValueError:
                    return
                if msg.dst_id == 'sink_gateway':
                    if self.sink_pose is None:
                        self.sink_pose = Pose()
                    self.sink_pose.position.x = x
                    self.sink_pose.position.y = y
                    self.sink_pose.position.z = z
                    self.sink_pose.orientation.w = 1.0
                elif msg.dst_id == 'ugv':
                    if self.ugv_pose is None:
                        self.ugv_pose = Pose()
                    self.ugv_pose.position.x = x
                    self.ugv_pose.position.y = y
                    self.ugv_pose.position.z = z
                    self.ugv_pose.orientation.w = 1.0
            return
        if msg.control_type != 'HELLO':
            return
        if not msg.payload.startswith('UGV,'):
            return
        parts = msg.payload.split(',')
        if len(parts) < 3:
            return
        try:
            x = float(parts[1])
            y = float(parts[2])
        except ValueError:
            return
        if self.ugv_pose is None:
            self.ugv_pose = Pose()
        self.ugv_pose.position.x = x
        self.ugv_pose.position.y = y

    def traffic_raw_cb(self, msg: TrafficMessage):
        if msg.flow_type != 1:
            return
        if msg.control_type == 'CLUSTER_REASSIGN':
            member_id = msg.dst_id
            new_ch = (msg.payload or '').strip()
            if not member_id or not new_ch:
                return
            state = self.uav_states.get(member_id)
            if state is not None:
                state.cluster_id = new_ch
            self.cluster_info.pop(member_id, None)
            return
        if msg.control_type == 'TASK_ASSIGN':
            member_id = msg.dst_id
            payload = (msg.payload or '').strip()
            if not member_id:
                return
            assigned = []
            if payload:
                for token in payload.split(';'):
                    parts = token.split(',')
                    if len(parts) < 2:
                        continue
                    try:
                        x = float(parts[0])
                        y = float(parts[1])
                    except ValueError:
                        continue
                    assigned.append((x, y))
            self.member_task_assignments[member_id] = assigned
            return
        if msg.control_type == 'NEW_DEPLOYMENT':
            ch_id = msg.dst_id
            parts = (msg.payload or '').split(',')
            if len(parts) < 3:
                return
            try:
                x = float(parts[0])
                y = float(parts[1])
                z = float(parts[2])
            except ValueError:
                return
            state = self.uav_states.get(ch_id)
            if state is not None:
                state.pose.position.x = x
                state.pose.position.y = y
                state.pose.position.z = z

    def network_stats_cb(self, msg: String):
        try:
            payload = json.loads(msg.data)
        except json.JSONDecodeError as exc:
            self.get_logger().warning(f"Failed to parse network stats: {exc}")
            return
        self.monitor_stats['generated_total'] = payload.get('generated_total', 0)
        self.monitor_stats['delivered_total'] = payload.get('delivered_total', 0)
        self.monitor_stats['drop_total'] = payload.get('drop_total', 0)
        self.monitor_stats['ack_total'] = payload.get('ack_total', 0)
        self.monitor_stats['generated_rate'] = payload.get('generated_rate', 0.0)
        self.monitor_stats['delivered_rate'] = payload.get('delivered_rate', 0.0)
        self.monitor_stats['drop_rate'] = payload.get('drop_rate', 0.0)
        self.monitor_stats['window_sec'] = payload.get('window_sec', 10.0)
        self.monitor_stats['last_msg_age'] = payload.get('last_msg_age', None)
        self.monitor_stats['last_drop_age'] = payload.get('last_drop_age', None)
        self.monitor_stats['last_delivered_age'] = payload.get('last_delivered_age', None)
        self.monitor_stats['control_type_counts'] = payload.get('control_type_counts', {})
        self.monitor_stats['drop_reason_counts'] = payload.get('drop_reason_counts', {})
        self.last_monitor_stats_time = self._now_sec()

    @staticmethod
    def role_label(role: int) -> str:
        if role == 1:
            return 'CH'
        if role == 0:
            return 'MEM'
        return 'UNK'

    @staticmethod
    def weather_state(weather: WeatherStatus) -> str:
        if weather.rain_intensity >= 5.0 or weather.wind_speed >= 12.0:
            return 'Stormy'
        if weather.rain_intensity >= 1.0 or weather.wind_speed >= 6.0:
            return 'Windy'
        return 'Sunny'

    def compute_required_spots(self) -> int:
        num_ch = sum(1 for st in self.uav_states.values() if st.role == 1)
        num_members = sum(1 for st in self.uav_states.values() if st.role == 0)

        load_ch = 0.0
        if (self.flight_time_ch_min + self.charge_time_ch_min) > 0.0:
            load_ch = self.charge_time_ch_min / (self.flight_time_ch_min + self.charge_time_ch_min)

        load_mem = 0.0
        if (self.flight_time_mem_min + self.charge_time_mem_min) > 0.0:
            load_mem = self.charge_time_mem_min / (self.flight_time_mem_min + self.charge_time_mem_min)

        total_load = num_ch * load_ch + num_members * load_mem
        effective_target = self.target_utilization if self.target_utilization > 0.0 else 0.9
        raw_spots = total_load / effective_target if effective_target > 0.0 else 0.0
        num_spots = int(raw_spots + 0.999999)
        if num_spots < 1 and total_load > 0.0:
            num_spots = 1
        return num_spots

    def _now_sec(self) -> float:
        return self.get_clock().now().nanoseconds * 1e-9

    def _format_age(self, last_time: float) -> str:
        if last_time is None:
            return "n/a"
        age = self._now_sec() - last_time
        if age < 0:
            age = 0.0
        return f"{age:.1f}s"

    @staticmethod
    def _format_age_seconds(age_sec: float) -> str:
        if age_sec is None or age_sec < 0:
            return "n/a"
        return f"{age_sec:.1f}s"

    def _line(self, text: str, size: int = 9, weight: str = 'normal', spacing: float = 1.0):
        return {
            'text': text,
            'size': size,
            'weight': weight,
            'spacing': spacing,
        }

    def _title_line(self, text: str) -> dict:
        return self._line(text, size=11, weight='bold')

    def _page_header(self, text: str) -> dict:
        return self._line(text, size=12, weight='bold', spacing=1.0)

    def _render_info_lines(self, lines, scroll_offset=0.0):
        background = self.info_ax.add_patch(
            plt.Rectangle(
                (0.01, 0.02), 0.98, 0.96,
                transform=self.info_ax.transAxes,
                facecolor='#111111',
                edgecolor='#333333',
                linewidth=1.0,
                zorder=0
            )
        )
        y = 0.965 + scroll_offset
        base_step = 0.032
        for line in lines:
            size = line.get('size', 9)
            weight = line.get('weight', 'normal')
            spacing = line.get('spacing', 1.0)
            text = line.get('text', '')
            if text:
                self.info_ax.text(
                    0.03, y, text,
                    va='top', ha='left',
                    color='white',
                    fontsize=size,
                    fontfamily='monospace',
                    fontweight=weight,
                    transform=self.info_ax.transAxes,
                    clip_on=True,
                    clip_path=background
                )
            y -= base_step * (size / 9) * spacing

    def _status_lines(self):
        info_lines = []
        if self.weather:
            info_lines.append(self._title_line(
                f"Weather ({self.weather_state(self.weather)})"
            ))
            info_lines.append(self._line(f"  Rain: {self.weather.rain_intensity:.1f} mm/h"))
            info_lines.append(self._line(
                f"  Wind: {self.weather.wind_speed:.1f} m/s @ {self.weather.wind_direction_deg:.0f}°"))
            info_lines.append(self._line(f"  Temp: {self.weather.temperature_c:.1f} °C"))
        else:
            info_lines.append(self._title_line('Weather'))
            info_lines.append(self._line('  (no data)'))

        fleet_states = {
            uid: st for uid, st in self.uav_states.items()
            if uid != 'sink_gateway' and not uid.startswith('ugv') and uid not in self.dead_uavs
        }
        if fleet_states:
            info_lines.append(self._title_line('Fleet status (pos [m], batt %)'))
            num_ch = sum(1 for st in fleet_states.values() if st.role == 1)
            num_mem = sum(1 for st in fleet_states.values() if st.role == 0)
            avg_capacity = sum(st.battery_capacity for st in fleet_states.values()) / len(
                fleet_states
            )
            info_lines.append(self._line(
                f"  UAVs: {len(fleet_states)} | CH: {num_ch} | MEM: {num_mem}"
            ))
            info_lines.append(self._line(
                f"  Comm radius (CH): {self.comm_radius_ch:.1f} m | "
                f"Avg capacity: {avg_capacity:.1f}"
            ))
            for uid in sorted(fleet_states.keys()):
                st = fleet_states[uid]
                px = st.pose.position.x
                py = st.pose.position.y
                info_lines.append(self._line(
                    f"  {uid} [{self.role_label(st.role)}]: ({px:.1f}, {py:.1f}) | "
                    f"{st.battery_level:.1f}%"))
        else:
            info_lines.append(self._title_line('UAV status'))
            info_lines.append(self._line('  (no reports)'))

        if self.dead_uavs:
            info_lines.append(self._title_line('Death events'))
            for uid in sorted(self.dead_uavs.keys()):
                death_info = self.dead_uavs[uid]
                info_lines.append(self._line(
                    f"  {uid}: {death_info.get('reason', 'UNKNOWN')}"
                ))
                info_lines.append(self._line(
                    f"    time: t={death_info.get('time', 0.0):.1f}s"
                ))
                info_lines.append(self._line(
                    f"    desc: {death_info.get('description', 'n/a')}"
                ))

        if self.ugv_pose:
            info_lines.append(self._title_line('UGV position'))
            info_lines.append(self._line(
                f"  ({self.ugv_pose.position.x:.1f}, {self.ugv_pose.position.y:.1f})"))
        else:
            info_lines.append(self._title_line('UGV position'))
            info_lines.append(self._line('  (pending deployment)'))

        info_lines.append(self._title_line('Control status'))
        if self.cluster_info:
            ch_ids = {info.ch_id for info in self.cluster_info.values() if info.ch_id}
            info_lines.append(self._line(
                f"  CH manager: {len(self.cluster_info)} clusters | CHs: {len(ch_ids)}"
            ))
        else:
            info_lines.append(self._line("  CH manager: (no cluster info)"))
        info_lines.append(self._line(
            f"  CH update age: {self._format_age(self.last_cluster_info_time)}"))
        info_lines.append(self._line(
            f"  Planner tasks: {self.last_task_points_count} | "
            f"age: {self._format_age(self.last_task_points_time)}"
        ))
        deployment_target = self.last_deployment_target or "n/a"
        info_lines.append(self._line(
            f"  Last deploy: {deployment_target} | "
            f"age: {self._format_age(self.last_deployment_time)}"
        ))
        return info_lines

    def _network_lines(self):
        drop_ratio = (
            self.monitor_stats['drop_total'] / self.monitor_stats['generated_total']
            if self.monitor_stats['generated_total'] > 0 else 0.0
        )
        top_controls = sorted(
            self.monitor_stats['control_type_counts'].items(),
            key=lambda item: item[1],
            reverse=True
        )[:6]
        top_drop_reasons = sorted(
            self.monitor_stats['drop_reason_counts'].items(),
            key=lambda item: item[1],
            reverse=True
        )[:3]
        window_sec = self.monitor_stats.get('window_sec', 10.0)
        net_lines = [
            self._title_line('Network routing (monitor)'),
            self._line(f"  generated: {self.monitor_stats['generated_total']}"),
            self._line(f"  delivered: {self.monitor_stats['delivered_total']}"),
            self._line(f"  drops: {self.monitor_stats['drop_total']} ({drop_ratio:.1%})"),
            self._line(f"  acks: {self.monitor_stats['ack_total']}"),
            self._title_line(f"Rates (last {window_sec:.0f}s)"),
            self._line(
                "  generated: "
                f"{self.monitor_stats['generated_rate']:.1f}/s | "
                f"delivered: {self.monitor_stats['delivered_rate']:.1f}/s"
            ),
            self._line(f"  drops: {self.monitor_stats['drop_rate']:.1f}/s"),
            self._title_line("Recent activity"),
            self._line(
                "  Last msg age: "
                f"{self._format_age_seconds(self.monitor_stats['last_msg_age'])}"
            ),
            self._line(
                "  Last drop age: "
                f"{self._format_age_seconds(self.monitor_stats['last_drop_age'])}"
            ),
            self._line(
                "  Last deliver age: "
                f"{self._format_age_seconds(self.monitor_stats['last_delivered_age'])}"
            ),
            self._line(
                "  Monitor update age: "
                f"{self._format_age(self.last_monitor_stats_time)}"
            ),
            self._title_line("Top control types")
        ]
        if top_controls:
            for name, count in top_controls:
                net_lines.append(self._line(f"  - {name}: {count}"))
        else:
            net_lines.append(self._line("  - (no traffic)"))
        if top_drop_reasons:
            net_lines.append(self._title_line("Top drop reasons"))
            for name, count in top_drop_reasons:
                net_lines.append(self._line(f"  - {name}: {count}"))
        return net_lines

    def _queue_lines(self):
        now = self._now_sec()
        snapshot_fresh = (
            self.last_charging_snapshot_time is not None and
            (now - self.last_charging_snapshot_time) <= 3.0
        )

        if snapshot_fresh:
            waiting_queue = self.charging_snapshot.get('waiting_queue', [])
            accepted_sessions = self.charging_snapshot.get('active_sessions', [])
            rejected = self.charging_snapshot.get('rejected', [])
            last_request = self.charging_snapshot.get('last_request', {})
            last_decision = self.charging_snapshot.get('last_decision', {})
            policy = self.charging_snapshot.get('policy', 'n/a')

            queue_lines = [
                self._title_line('Charging queue (UGV snapshot)'),
                self._line(f"  snapshot age: {self._format_age(self.last_charging_snapshot_time)} (fresh)"),
                self._title_line('Scheduling'),
                self._line(f"  {policy}"),
                self._title_line(f"Current Waiting List (Q): {len(waiting_queue)}"),
            ]
            for item in waiting_queue:
                uid = item.get('uav_id', 'unknown')
                role = self.role_label(int(item.get('live_role', -1)))
                batt = float(item.get('live_battery', 0.0))
                batt = max(0.0, min(100.0, batt))
                req_t = item.get('request_time')
                age_s = max(0.0, now - float(req_t)) if req_t is not None else 0.0
                queue_lines.append(self._line(
                    f"    - {uid} [{role}] | {batt:.1f}% | age {age_s:.1f}s"
                ))

            queue_lines.append(self._title_line(f"Accepted List: {len(accepted_sessions)}"))
            for item in accepted_sessions:
                uid = item.get('uav_id', 'unknown')
                st = float(item.get('start', 0.0))
                en = float(item.get('end', 0.0))
                role = self.role_label(int(item.get('live_role', -1)))
                batt = float(item.get('live_battery', 0.0))
                batt = max(0.0, min(100.0, batt))
                age_s = max(0.0, now - st)
                reason = str(item.get('session_end_reason', 'in_progress'))
                last_seen_batt = float(item.get('last_battery_seen', batt))
                progress_age = float(item.get('last_progress_age', 0.0))
                queue_lines.append(self._line(
                    f"    - {uid} [{role}] | live {batt:.1f}% | last {last_seen_batt:.1f}% | "
                    f"progress_age {progress_age:.1f}s | age {age_s:.1f}s | [{st:.1f}, {en:.1f}] | {reason}"
                ))

            recent_endings = self.charging_snapshot.get('recent_session_endings', [])
            queue_lines.append(self._title_line(f"Recent session endings: {len(recent_endings)}"))
            for item in recent_endings[:8]:
                uid = item.get('uav_id', 'unknown')
                reason = str(item.get('session_end_reason', 'n/a'))
                duration = float(item.get('charge_duration_sec', 0.0))
                ended_t = item.get('end')
                end_age_s = max(0.0, now - float(ended_t)) if ended_t is not None else 0.0
                queue_lines.append(self._line(
                    f"    - {uid} | {reason} | duration {duration:.1f}s | ended {end_age_s:.1f}s ago"
                ))

            queue_lines.append(self._title_line(f"UGV intake rejected: {len(rejected)}"))
            for item in rejected:
                uid = item.get('uav_id', 'unknown')
                reason = item.get('reason', 'n/a')
                rej_t = item.get('time')
                age_s = max(0.0, now - float(rej_t)) if rej_t is not None else 0.0
                status = self.uav_states.get(uid)
                role = self.role_label(status.role) if status is not None else 'UNK'
                batt = status.battery_level if status is not None else None
                if batt is None:
                    queue_lines.append(self._line(
                        f"    - {uid} [{role}] | age {age_s:.1f}s | {reason}"
                    ))
                else:
                    queue_lines.append(self._line(
                        f"    - {uid} [{role}] | {batt:.1f}% | age {age_s:.1f}s | {reason}"
                    ))

            queue_lines.append(self._title_line('Last request'))
            if last_request:
                uid = last_request.get('uav_id', 'unknown')
                role = self.role_label(int(last_request.get('role', -1)))
                batt = max(0.0, min(100.0, float(last_request.get('battery', 0.0))))
                t = last_request.get('time')
                msg_id = last_request.get('msg_id', 'n/a')
                status = last_request.get('status', 'n/a')
                age_s = max(0.0, now - float(t)) if t is not None else 0.0
                queue_lines.append(self._line(
                    f"  {uid} [{role}] | {batt:.1f}% | age {age_s:.1f}s"
                ))
                queue_lines.append(self._line(
                    f"  msg_id: {msg_id} | status: {status}"
                ))
            else:
                queue_lines.append(self._line('  (none)'))

            queue_lines.append(self._title_line('Last decision'))
            if last_decision:
                uid = last_decision.get('uav_id', 'unknown')
                accepted = bool(last_decision.get('accepted', False))
                t = last_decision.get('time')
                age_s = max(0.0, now - float(t)) if t is not None else 0.0
                st = float(last_decision.get('slot_start', 0.0))
                en = float(last_decision.get('slot_end', 0.0))
                priority = int(last_decision.get('priority', -1))
                queue_lines.append(self._line(
                    f"  {uid} | {'accepted' if accepted else 'rejected'} | "
                    f"pri {priority} | age {age_s:.1f}s | [{st:.1f}, {en:.1f}]"
                ))
            else:
                queue_lines.append(self._line('  (none)'))
            return queue_lines

        spots_now = self.compute_required_spots()
        load_ch_den = self.flight_time_ch_min + self.charge_time_ch_min
        load_mem_den = self.flight_time_mem_min + self.charge_time_mem_min
        load_ch = (self.charge_time_ch_min / load_ch_den) if load_ch_den > 0.0 else 0.0
        load_mem = (self.charge_time_mem_min / load_mem_den) if load_mem_den > 0.0 else 0.0
        queue_lines = [
            self._title_line('Charging queue (fanet fallback)'),
            self._line(f"  snapshot stale: {self._format_age(self.last_charging_snapshot_time)}"),
            self._line(f"  pending requests: {len(self.pending_charges)}"),
            self._line(f"  Last request age: {self._format_age(self.last_charge_request_time)}"),
            self._line(f"  Last decision age: {self._format_age(self.last_charge_decision_time)}"),
            self._title_line("Scheduling"),
            self._line(f"  {self.latest_policy}"),
            self._title_line("Spots"),
            self._line(
                f"  ceil((Nch*{load_ch:.2f} + Nmem*{load_mem:.2f})/"
                f" {self.target_utilization:.2f})"),
            self._line(f"  spots now: {spots_now}")
        ]
        if self.last_charge_decision_target:
            decision_state = (
                "accepted" if self.last_charge_decision_accepted
                else "rejected" if self.last_charge_decision_accepted is False
                else "sent"
            )
            queue_lines.append(self._line(
                f"  Last decision: {self.last_charge_decision_target} ({decision_state})"
            ))
        if self.uav_states:
            queue_lines.append(self._title_line('Fleet status (batt %)'))
            for uid in sorted(self.uav_states.keys()):
                st = self.uav_states[uid]
                queue_lines.append(self._line(
                    f"  {uid} [{self.role_label(st.role)}] | {st.battery_level:.1f}%"
                ))
        if self.pending_charges:
            queue_lines.append(self._title_line('Waiting'))
            ordered = sorted(
                self.pending_charges.items(),
                key=lambda item: item[1].get('time') or 0.0
            )
            for uid, meta in ordered:
                role_val = meta.get('role')
                battery = meta.get('battery')
                status = self.uav_states.get(uid)
                if role_val is None and status is not None:
                    role_val = status.role
                if battery is None and status is not None:
                    battery = status.battery_level
                role_label = self.role_label(int(role_val)) if role_val is not None else 'UNK'
                if battery is None:
                    queue_lines.append(self._line(f"    - {uid} [{role_label}]"))
                else:
                    queue_lines.append(self._line(
                        f"    - {uid} [{role_label}] | {battery:.1f}%"
                    ))
        return queue_lines

    # --- plotting ---

    def update_plot(self):
        """Redraw the full figure from cached state."""
        self.ax.cla()
        self.ax.set_facecolor('black')
        self.ax.set_xlabel('X [m]', color='white')
        self.ax.set_ylabel('Y [m]', color='white')
        self.ax.set_title('Fleet live view', color='white')
        self.ax.set_xlim(self.x_min, self.x_max)
        self.ax.set_ylim(self.y_min, self.y_max)
        self.ax.grid(False)
        self.ax.tick_params(colors='white')
        for spine in self.ax.spines.values():
            spine.set_color('white')

        # draw sink and ugv
        if self.sink_pose is not None:
            self.ax.scatter(self.sink_pose.position.x,
                            self.sink_pose.position.y,
                            marker='o', s=80, edgecolors='white', facecolors='none')
            self.ax.text(self.sink_pose.position.x,
                         self.sink_pose.position.y + 5,
                         'sink', color='white', fontsize=8)

        if self.ugv_pose is not None:
            self.ax.scatter(self.ugv_pose.position.x,
                            self.ugv_pose.position.y,
                            marker='^', s=80, edgecolors='yellow', facecolors='none')
            self.ax.text(self.ugv_pose.position.x,
                         self.ugv_pose.position.y + 5,
                         'ugv', color='yellow', fontsize=8)

        # draw task points first so UAVs appear above
        task_legend = None
        for tp in self.task_points:
            color = self.color_for_cluster(tp.cluster_id)
            self.ax.scatter(tp.position.x, tp.position.y,
                            marker='x', s=40, color=color, alpha=0.9)
            self.ax.text(tp.position.x, tp.position.y - 5,
                         tp.id, color=color, fontsize=7)
        if self.task_points:
            task_legend = self.ax.scatter([], [], marker='x', s=40, color='white',
                                          label='task point')

        # draw assignment points from recovery first
        assigned_legend = None
        for member_id, points in self.member_task_assignments.items():
            if not points:
                continue
            state = self.uav_states.get(member_id)
            color = self.color_for_cluster(state.cluster_id if state else '')
            for idx, (x, y) in enumerate(points):
                self.ax.scatter(x, y, marker='.', s=22, color=color, alpha=0.9)
                if idx == 0:
                    self.ax.text(x, y - 6, f"{member_id}:assign", color=color, fontsize=6)
            if assigned_legend is None:
                assigned_legend = self.ax.scatter([], [], marker='.', s=22, color='white',
                                                  label='recovery task assign')

        # draw UAVs
        for uav_id, st in self.uav_states.items():
            if uav_id in self.dead_uavs:
                continue
            x = st.pose.position.x
            y = st.pose.position.y
            color = self.color_for_cluster(st.cluster_id)
            role_tag = self.role_label(st.role)

            if st.role == 1:
                # CH: red + service radius
                self.ax.scatter(x, y, c=color, s=30)
                self.ax.text(x, y + 3, f"{uav_id} ({role_tag})", color=color, fontsize=8)

                # service_radius is in the status; comm radius is a viz parameter
                service_circle = plt.Circle((x, y), st.service_radius, linestyle='--',
                                            fill=False, edgecolor=color, alpha=0.4)
                self.ax.add_patch(service_circle)

                if self.comm_radius_ch > 0.0:
                    comm_circle = plt.Circle((x, y), self.comm_radius_ch, linestyle=':',
                                             linewidth=1.2, fill=False,
                                             edgecolor='white', alpha=0.35)
                    self.ax.add_patch(comm_circle)
            else:
                # member: green
                self.ax.scatter(x, y, c=color, s=20)
                self.ax.text(x, y + 3, f"{uav_id} ({role_tag})", color=color, fontsize=8)

        legend_handles = []
        if task_legend:
            legend_handles.append(task_legend)
        if assigned_legend:
            legend_handles.append(assigned_legend)
        cluster_ids = sorted({
            st.cluster_id for uid, st in self.uav_states.items()
            if st.cluster_id and uid not in self.dead_uavs
        })
        for cluster_id in cluster_ids:
            legend_handles.append(Line2D(
                [0], [0],
                marker='o',
                color='none',
                label=f"cluster {cluster_id}",
                markerfacecolor=self.color_for_cluster(cluster_id),
                markeredgecolor=self.color_for_cluster(cluster_id),
                markersize=6
            ))
        if legend_handles:
            self.ax.legend(
                handles=legend_handles,
                loc='upper right',
                facecolor='black',
                edgecolor='white',
                labelcolor='white'
            )

        self.ax.set_aspect('equal', adjustable='box')

        # info panel ---------------------------------------------------
        self.info_ax.cla()
        self.info_ax.axis('off')
        self.info_ax.set_facecolor('#111111')
        page_name = self.info_pages[self.info_page_index]
        header = [self._page_header(
            f"{page_name} ({self.info_page_index + 1}/{len(self.info_pages)})")]
        if not _headless:
            header.append(self._line("Use ◀/▶ to browse"))
        if page_name == 'Status':
            info_lines = header + self._status_lines()
        elif page_name == 'Network':
            info_lines = header + self._network_lines()
        else:
            info_lines = header + self._queue_lines()
        self._render_info_lines(info_lines, self.info_scroll.get(page_name, 0.0))

        self.fig.canvas.draw()
        plt.pause(0.001)


def main(args=None):
    rclpy.init(args=args)
    node = FleetVizNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()
    plt.close('all')
