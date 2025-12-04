import math
import rclpy
from rclpy.node import Node

import matplotlib.pyplot as plt

from uav_msgs.msg import UavStatus
from uav_msgs.msg import UavDeployment


class FleetVizNode(Node):
    def __init__(self):
        super().__init__('fleet_viz_node')

        self.status_sub = self.create_subscription(
            UavStatus, '/uav_fleet/status', self.status_cb, 50)

        self.deployment_sub = self.create_subscription(
            UavDeployment, '/coverage_planner/deployment', self.deployment_cb, 20)

        # state
        self.uav_states = {}   # id -> last UavStatus
        self.sink_pose = None
        self.ugv_pose = None

        # plot setup
        plt.ion()
        self.fig, self.ax = plt.subplots()
        self.ax.set_xlabel('X [m]')
        self.ax.set_ylabel('Y [m]')
        self.ax.set_title('Fleet live view')

        # area limits (can be tuned / parameterised)
        self.x_min = -250.0
        self.x_max =  250.0
        self.y_min = -250.0
        self.y_max =  250.0

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

    # --- plotting ---

    def update_plot(self):
        self.ax.cla()
        self.ax.set_facecolor('black')
        self.ax.set_xlabel('X [m]')
        self.ax.set_ylabel('Y [m]')
        self.ax.set_title('Fleet live view')
        self.ax.set_xlim(self.x_min, self.x_max)
        self.ax.set_ylim(self.y_min, self.y_max)
        self.ax.grid(False)

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
