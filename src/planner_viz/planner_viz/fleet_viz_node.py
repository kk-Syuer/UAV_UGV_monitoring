from typing import Dict

import rclpy
from rclpy.node import Node

import matplotlib.pyplot as plt

from uav_msgs.msg import ChargeDecision
from uav_msgs.msg import ChargeRequest
from uav_msgs.msg import UavDeployment
from uav_msgs.msg import UavStatus
from uav_msgs.msg import WeatherStatus


class FleetVizNode(Node):
    def __init__(self):
        super().__init__('fleet_viz_node')

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

        # state
        self.uav_states = {}   # id -> last UavStatus
        self.sink_pose = None
        self.ugv_pose = None
        self.weather = None

        # queue + scheduling summary
        self.pending_charges: Dict[str, float] = {}
        self.latest_policy = 'n/a'

        # plot setup
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
        self.x_max =  1200.0
        self.y_min = -1200.0
        self.y_max =  1200.0

        # timer to refresh plot
        self.timer = self.create_timer(0.2, self.update_plot)

        self.get_logger().info("FleetVizNode started.")

    # --- callbacks ---

    def status_cb(self, msg: UavStatus):
        self.uav_states[msg.uav_id] = msg

    def deployment_cb(self, msg: UavDeployment):
        if msg.uav_id == 'sink_gateway':
            self.sink_pose = msg.target_pose
        elif msg.uav_id == 'ugv':
            self.ugv_pose = msg.target_pose

    def weather_cb(self, msg: WeatherStatus):
        self.weather = msg

    def charge_request_cb(self, msg: ChargeRequest):
        # track outstanding charging requests to approximate queue size
        self.pending_charges[msg.uav_id] = self.get_clock().now().seconds_nanoseconds()[0]

    def charge_decision_cb(self, msg: ChargeDecision):
        self.latest_policy = msg.policy if msg.policy else 'n/a'
        # once a decision is made, remove from pending queue
        if msg.uav_id in self.pending_charges:
            self.pending_charges.pop(msg.uav_id)

    # --- plotting ---

    def update_plot(self):
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

        # draw UAVs
        for uav_id, st in self.uav_states.items():
            x = st.pose.position.x
            y = st.pose.position.y

            if st.role == 1:
                # CH: red + service radius
                self.ax.scatter(x, y, c='red', s=30)
                self.ax.text(x, y + 3, uav_id, color='red', fontsize=8)

                # service_radius is in the status
                R = st.service_radius
                circle = plt.Circle((x, y), R, linestyle='--',
                                    fill=False, edgecolor='red', alpha=0.4)
                self.ax.add_patch(circle)
            else:
                # member: green
                self.ax.scatter(x, y, c='lime', s=20)
                self.ax.text(x, y + 3, uav_id, color='lime', fontsize=8)

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
                info_lines.append(f"  {uid}: ({px:.1f}, {py:.1f}) | {st.battery_level:.1f}%")
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
