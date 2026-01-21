from collections import defaultdict, deque
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
from uav_msgs.msg import ChargeDecision
from uav_msgs.msg import ChargeRequest
from uav_msgs.msg import ClusterInfo
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

        self.charge_request_sub = self.create_subscription(
            ChargeRequest, '/uav_fleet/charge_requests', self.charge_request_cb, 50)

        self.charge_decision_sub = self.create_subscription(
            ChargeDecision, '/ugv/charge_decisions', self.charge_decision_cb, 50)
        task_point_qos = QoSProfile(depth=1, durability=DurabilityPolicy.TRANSIENT_LOCAL)
        self.task_point_sub = self.create_subscription(
            TaskPointArray, '/coverage_planner/task_points', self.task_point_cb, task_point_qos)
        self.traffic_sub = self.create_subscription(
            TrafficMessage, '/fanet/network_bus', self.traffic_cb, 50)
        self.delivered_sub = self.create_subscription(
            TrafficMessage, '/fanet/delivered', self.delivered_cb, 50)
        self.cluster_info_sub = self.create_subscription(
            ClusterInfo, '/ch_manager/cluster_info', self.cluster_info_cb, 20)

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

        # Queue + scheduling summary.
        self.pending_charges: Dict[str, Dict[str, Any]] = {}
        self.latest_policy = 'n/a'
        self.last_charge_request_time = None
        self.last_charge_decision_time = None
        self.last_charge_decision_target = None
        self.last_charge_decision_accepted = None

        # Control-plane snapshots.
        self.cluster_info = {}
        self.last_cluster_info_time = None
        self.last_task_points_time = None
        self.last_task_points_count = 0
        self.last_deployment_time = None
        self.last_deployment_target = None

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

        # Network stats for debug panels.
        self.traffic_total = 0
        self.delivered_total = 0
        self.drop_total = 0
        self.ack_total = 0
        self.control_type_counts = defaultdict(int)
        self.drop_reason_counts = defaultdict(int)
        self.last_msg_time = None
        self.last_drop_time = None
        self.last_delivered_time = None
        self.rate_window_sec = 10.0
        self.msg_timestamps = deque()
        self.drop_timestamps = deque()
        self.delivered_timestamps = deque()

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

    def charge_request_cb(self, msg: ChargeRequest):
        """Track outstanding charging requests."""
        # track outstanding charging requests to approximate queue size
        request_time = msg.stamp.sec + msg.stamp.nanosec * 1e-9
        self.pending_charges[msg.uav_id] = {
            'time': request_time,
            'battery': msg.battery_level,
            'role': int(msg.role),
        }
        self.last_charge_request_time = request_time

    def charge_decision_cb(self, msg: ChargeDecision):
        """Update scheduling policy and clear completed requests."""
        now = self._now_sec()
        self.latest_policy = msg.policy if msg.policy else 'n/a'
        self.last_charge_decision_time = now
        self.last_charge_decision_target = msg.uav_id
        self.last_charge_decision_accepted = msg.accepted
        # once a decision is made, remove from pending queue
        if msg.uav_id in self.pending_charges:
            self.pending_charges.pop(msg.uav_id)

    def task_point_cb(self, msg: TaskPointArray):
        self.task_points = list(msg.tasks)
        self.last_task_points_time = self._now_sec()
        self.last_task_points_count = len(self.task_points)

    def cluster_info_cb(self, msg: ClusterInfo):
        self.cluster_info[msg.cluster_id] = msg
        self.last_cluster_info_time = self._now_sec()

    def delivered_cb(self, msg: TrafficMessage):
        now = self._now_sec()
        self.delivered_total += 1
        self.last_delivered_time = now
        self._record_timestamp(self.delivered_timestamps, now)

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
        now = self._now_sec()
        self.traffic_total += 1
        self.control_type_counts[msg.control_type] += 1
        self.last_msg_time = now
        self._record_timestamp(self.msg_timestamps, now)

        if msg.control_type == 'DROP':
            self.drop_total += 1
            self.drop_reason_counts[msg.drop_reason or 'UNKNOWN'] += 1
            self.last_drop_time = now
            self._record_timestamp(self.drop_timestamps, now)
            return
        if msg.control_type == 'ACK':
            self.ack_total += 1
        if msg.control_type == 'CHARGE_REQUEST':
            request_time = msg.creation_time.sec + msg.creation_time.nanosec * 1e-9
            self.pending_charges[msg.src_id] = {
                'time': request_time,
                'battery': None,
                'role': None,
            }
            self.last_charge_request_time = request_time
            return
        if msg.control_type == 'CHARGE_DECISION':
            self.last_charge_decision_time = now
            self.last_charge_decision_target = msg.dst_id
            self.last_charge_decision_accepted = None
            if msg.dst_id in self.pending_charges:
                self.pending_charges.pop(msg.dst_id)
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

    def _record_timestamp(self, timestamps: deque, now: float) -> None:
        timestamps.append(now)
        self._prune_timestamps(timestamps, now)

    def _prune_timestamps(self, timestamps: deque, now: float) -> None:
        cutoff = now - self.rate_window_sec
        while timestamps and timestamps[0] < cutoff:
            timestamps.popleft()

    def _format_age(self, last_time: float) -> str:
        if last_time is None:
            return "n/a"
        age = self._now_sec() - last_time
        if age < 0:
            age = 0.0
        return f"{age:.1f}s"

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

        if self.uav_states:
            info_lines.append(self._title_line('Fleet status (pos [m], batt %)'))
            num_ch = sum(1 for st in self.uav_states.values() if st.role == 1)
            num_mem = sum(1 for st in self.uav_states.values() if st.role == 0)
            num_backbone = sum(1 for st in self.uav_states.values() if st.backbone_active)
            avg_capacity = sum(st.battery_capacity for st in self.uav_states.values()) / len(
                self.uav_states
            )
            info_lines.append(self._line(
                f"  UAVs: {len(self.uav_states)} | CH: {num_ch} | MEM: {num_mem} | "
                f"Backbone: {num_backbone}"
            ))
            info_lines.append(self._line(
                f"  Comm radius (CH): {self.comm_radius_ch:.1f} m | "
                f"Avg capacity: {avg_capacity:.1f}"
            ))
            for uid in sorted(self.uav_states.keys()):
                st = self.uav_states[uid]
                px = st.pose.position.x
                py = st.pose.position.y
                info_lines.append(self._line(
                    f"  {uid} [{self.role_label(st.role)}]: ({px:.1f}, {py:.1f}) | "
                    f"{st.battery_level:.1f}%"))
        else:
            info_lines.append(self._title_line('UAV status'))
            info_lines.append(self._line('  (no reports)'))

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
        now = self._now_sec()
        self._prune_timestamps(self.msg_timestamps, now)
        self._prune_timestamps(self.drop_timestamps, now)
        self._prune_timestamps(self.delivered_timestamps, now)
        msg_rate = len(self.msg_timestamps) / self.rate_window_sec
        drop_rate = len(self.drop_timestamps) / self.rate_window_sec
        delivered_rate = len(self.delivered_timestamps) / self.rate_window_sec
        drop_ratio = (self.drop_total / self.traffic_total) if self.traffic_total > 0 else 0.0
        top_controls = sorted(
            self.control_type_counts.items(), key=lambda item: item[1], reverse=True
        )[:3]
        top_drop_reasons = sorted(
            self.drop_reason_counts.items(), key=lambda item: item[1], reverse=True
        )[:3]
        net_lines = [
            self._title_line('Network routing'),
            self._line(f"  bus msgs: {self.traffic_total}"),
            self._line(f"  delivered: {self.delivered_total}"),
            self._line(f"  drops: {self.drop_total} ({drop_ratio:.1%})"),
            self._line(f"  acks: {self.ack_total}"),
            self._title_line(f"Rates (last {self.rate_window_sec:.0f}s)"),
            self._line(f"  bus: {msg_rate:.1f}/s | delivered: {delivered_rate:.1f}/s"),
            self._line(f"  drops: {drop_rate:.1f}/s"),
            self._title_line("Recent activity"),
            self._line(f"  Last msg age: {self._format_age(self.last_msg_time)}"),
            self._line(f"  Last drop age: {self._format_age(self.last_drop_time)}"),
            self._line(f"  Last deliver age: {self._format_age(self.last_delivered_time)}"),
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
        spots_now = self.compute_required_spots()
        load_ch_den = self.flight_time_ch_min + self.charge_time_ch_min
        load_mem_den = self.flight_time_mem_min + self.charge_time_mem_min
        load_ch = (self.charge_time_ch_min / load_ch_den) if load_ch_den > 0.0 else 0.0
        load_mem = (self.charge_time_mem_min / load_mem_den) if load_mem_den > 0.0 else 0.0
        queue_lines = [
            self._title_line('Charging queue'),
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
                role_label = self.role_label(int(role_val)) if role_val is not None else 'UNK'
                battery = meta.get('battery')
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

        # draw UAVs
        for uav_id, st in self.uav_states.items():
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
        cluster_ids = sorted({st.cluster_id for st in self.uav_states.values() if st.cluster_id})
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
