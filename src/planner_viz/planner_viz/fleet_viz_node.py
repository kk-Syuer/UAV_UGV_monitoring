from typing import Dict

import rclpy
from rclpy.node import Node

import matplotlib.pyplot as plt

from geometry_msgs.msg import Pose
from uav_msgs.msg import ChargeDecision
from uav_msgs.msg import ChargeRequest
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
            UavStatus, '/uav_fleet/status', self.status_cb, 50)

        self.deployment_sub = self.create_subscription(
            UavDeployment, '/coverage_planner/deployment', self.deployment_cb, 20)

        self.weather_sub = self.create_subscription(
            WeatherStatus, '/environment/weather', self.weather_cb, 10)

        self.charge_request_sub = self.create_subscription(
            ChargeRequest, '/uav_fleet/charge_requests', self.charge_request_cb, 50)

        self.charge_decision_sub = self.create_subscription(
            ChargeDecision, '/ugv/charge_decisions', self.charge_decision_cb, 50)
        self.task_point_sub = self.create_subscription(
            TaskPointArray, '/coverage_planner/task_points', self.task_point_cb, 10)
        self.traffic_sub = self.create_subscription(
            TrafficMessage, '/network/traffic', self.traffic_cb, 50)

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
        self.pending_charges: Dict[str, float] = {}
        self.latest_policy = 'n/a'

        # Plot setup for the main canvas and info panels.
        plt.ion()
        self.fig = plt.figure(figsize=(12, 7))
        gs = self.fig.add_gridspec(2, 2, width_ratios=[3.0, 1.1], height_ratios=[2.2, 1.0])

        # main grid
        self.ax = self.fig.add_subplot(gs[:, 0])
        self.ax.set_xlabel('X [m]')
        self.ax.set_ylabel('Y [m]')
        self.ax.set_title('Fleet live view')

        # info panels
        self.info_ax = self.fig.add_subplot(gs[0, 1])
        self.queue_ax = self.fig.add_subplot(gs[1, 1])
        for panel in (self.info_ax, self.queue_ax):
            panel.axis('off')

        # dark background for consistent contrast with white text
        self.fig.patch.set_facecolor("#000000")

        # area limits (can be tuned / parameterised)
        self.x_min = -1200.0
        self.x_max = 1200.0
        self.y_min = -1200.0
        self.y_max = 1200.0

        self.comm_radius_ch = float(self.declare_parameter('comm_radius_ch', 400.0).value)

        # Timer to refresh plot periodically.
        self.timer = self.create_timer(0.2, self.update_plot)

        self.get_logger().info("FleetVizNode started.")

    # --- callbacks ---

    def status_cb(self, msg: UavStatus):
        """Cache latest UAV status for plotting."""
        self.uav_states[msg.uav_id] = msg

    def deployment_cb(self, msg: UavDeployment):
        """Capture sink/UGV deployments for map anchors."""
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
        self.pending_charges[msg.uav_id] = self.get_clock().now().seconds_nanoseconds()[0]

    def charge_decision_cb(self, msg: ChargeDecision):
        """Update scheduling policy and clear completed requests."""
        self.latest_policy = msg.policy if msg.policy else 'n/a'
        # once a decision is made, remove from pending queue
        if msg.uav_id in self.pending_charges:
            self.pending_charges.pop(msg.uav_id)

    def task_point_cb(self, msg: TaskPointArray):
        self.task_points = list(msg.tasks)

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
        for tp in self.task_points:
            color = self.color_for_cluster(tp.cluster_id)
            self.ax.scatter(tp.position.x, tp.position.y,
                            marker='x', s=40, color=color, alpha=0.9)
            self.ax.text(tp.position.x, tp.position.y - 5,
                         tp.id, color=color, fontsize=7)

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

        self.ax.set_aspect('equal', adjustable='box')

        # info panels --------------------------------------------------
        self.info_ax.cla()
        self.info_ax.axis('off')
        info_lines = []
        # Weather
        if self.weather:
            info_lines.append('Weather')
            info_lines.append(f"  Rain: {self.weather.rain_intensity:.1f} mm/h")
            info_lines.append(f"  Wind: {self.weather.wind_speed:.1f} m/s @ {self.weather.wind_direction_deg:.0f}°")
            info_lines.append(f"  Temp: {self.weather.temperature_c:.1f} °C")
            info_lines.append(f"  State: {self.weather_state(self.weather)}")
        else:
            info_lines.append('Weather: (no data)')

        # UAV status summary
        if self.uav_states:
            info_lines.append('')
            info_lines.append('UAV status (pos [m], batt %)')
            for uid in sorted(self.uav_states.keys()):
                st = self.uav_states[uid]
                px = st.pose.position.x
                py = st.pose.position.y
                comm_label = f"{self.comm_radius_ch:.1f}" if st.role == 1 else "n/a"
                info_lines.append(
                    f"  {uid} [{self.role_label(st.role)}]: ({px:.1f}, {py:.1f}) | "
                    f"{st.battery_level:.1f}% | cap {st.battery_capacity:.1f} | "
                    f"comm {comm_label}")
                if len(info_lines) > 12:
                    info_lines.append('  ...')
                    break
        else:
            info_lines.append('')
            info_lines.append('UAV status: (no reports)')

        # UGV position
        info_lines.append('')
        if self.ugv_pose:
            info_lines.append(
                f"UGV position: ({self.ugv_pose.position.x:.1f}, {self.ugv_pose.position.y:.1f})")
        else:
            info_lines.append('UGV position: (pending deployment)')

        self.info_ax.text(0.02, 0.98, '\n'.join(info_lines),
                          va='top', ha='left', color='white', fontsize=9,
                          fontfamily='monospace')

        # queue / scheduling panel -------------------------------------
        self.queue_ax.cla()
        self.queue_ax.axis('off')
        queue_lines = [
            'Charging queue',
            f"  pending requests: {len(self.pending_charges)}",
            '',
            f"Scheduling: {self.latest_policy}"
        ]
        if self.pending_charges:
            queue_lines.append('  waiting:')
            for uid in sorted(self.pending_charges.keys()):
                queue_lines.append(f"    - {uid}")
        self.queue_ax.text(0.02, 0.98, '\n'.join(queue_lines),
                           va='top', ha='left', color='white', fontsize=10,
                           fontfamily='monospace', weight='bold')

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
