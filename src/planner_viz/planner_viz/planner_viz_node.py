import math
from typing import Dict

import rclpy
from rclpy.node import Node

from uav_msgs.msg import UavDeployment

import matplotlib
matplotlib.use("TkAgg")  # or Qt5Agg; TkAgg tends to work everywhere
import matplotlib.pyplot as plt
from matplotlib.patches import Circle


class PlannerVizNode(Node):
    def __init__(self):
        super().__init__("planner_viz_node")

        # ---- Parameters ----
        self.x_min = self.declare_parameter("x_min", 0.0).get_parameter_value().double_value
        self.x_max = self.declare_parameter("x_max", 600.0).get_parameter_value().double_value
        self.y_min = self.declare_parameter("y_min", 0.0).get_parameter_value().double_value
        self.y_max = self.declare_parameter("y_max", 600.0).get_parameter_value().double_value
        self.service_radius_ch = self.declare_parameter("service_radius_ch", 250.0) \
            .get_parameter_value().double_value

        # ---- Matplotlib figure ----
        plt.ion()
        self.fig, self.ax = plt.subplots()
        self.ax.set_title("Coverage planner view")
        self.ax.set_xlim(self.x_min, self.x_max)
        self.ax.set_ylim(self.y_min, self.y_max)
        self.ax.set_xlabel("X [m]")
        self.ax.set_ylabel("Y [m]")
        self.ax.set_facecolor("black")

        # Store artists
        self.sink_artist = None
        self.ugv_artist = None
        self.ch_artists: Dict[str, any] = {}
        self.member_artists: Dict[str, any] = {}
        self.ch_circles: Dict[str, Circle] = {}

        # legend placeholders
        self.sink_legend, = self.ax.plot([], [], "bo", label="sink")
        self.ugv_legend, = self.ax.plot([], [], "^", color="yellow", label="ugv")
        self.ax.legend(loc="upper right")

        # ---- ROS subscription ----
        self.sub = self.create_subscription(
            UavDeployment,
            "/coverage_planner/deployment",
            self.deployment_callback,
            10,
        )

        # small timer to regularly refresh the plot
        self.timer = self.create_timer(0.2, self.redraw)

        self.get_logger().info(
            f"Planner viz started: area=([{self.x_min},{self.x_max}] x "
            f"[{self.y_min},{self.y_max}]), R_s={self.service_radius_ch}"
        )

    # ------------------------------------------------------------------

    def deployment_callback(self, msg: UavDeployment):
        uid = msg.uav_id
        x = msg.target_pose.position.x
        y = msg.target_pose.position.y
        role = msg.role

        # role convention:
        # 0 = member UAV
        # 1 = CH
        # 2 = sink
        # 3 = UGV

        if uid == "sink_gateway" or role == 2:
            # SINK
            if self.sink_artist is None:
                self.sink_artist, = self.ax.plot(x, y, "bo")
            else:
                self.sink_artist.set_data([x], [y])
            self.get_logger().info(f"Viz: sink at ({x:.1f}, {y:.1f})")
            return

        if uid == "ugv" or role == 3:
            # UGV
            if self.ugv_artist is None:
                self.ugv_artist, = self.ax.plot(x, y, "^", color="yellow")
            else:
                self.ugv_artist.set_data([x], [y])
            self.get_logger().info(f"Viz: ugv at ({x:.1f}, {y:.1f})")
            return

        # UAVs
        if role == 1:
            # Cluster head
            if uid not in self.ch_artists:
                # point
                artist, = self.ax.plot(x, y, "ro")
                self.ch_artists[uid] = artist
                # text
                self.ax.text(x + 5.0, y + 5.0, uid,
                             color="white", fontsize=8)

                # coverage circle
                circ = Circle(
                    (x, y),
                    radius=self.service_radius_ch,
                    linewidth=1.0,
                    edgecolor="red",
                    facecolor="none",
                    alpha=0.7,
                    linestyle="--",
                )
                self.ax.add_patch(circ)
                self.ch_circles[uid] = circ
            else:
                artist = self.ch_artists[uid]
                artist.set_data([x], [y])
                circ = self.ch_circles[uid]
                circ.center = (x, y)

            self.get_logger().info(
                f"Viz: CH {uid} at ({x:.1f}, {y:.1f})"
            )

        else:
            # Member UAV
            if uid not in self.member_artists:
                artist, = self.ax.plot(x, y, "go")
                self.member_artists[uid] = artist
                self.ax.text(x + 5.0, y + 5.0, uid,
                             color="white", fontsize=8)
            else:
                artist = self.member_artists[uid]
                artist.set_data([x], [y])

            self.get_logger().info(
                f"Viz: member {uid} at ({x:.1f}, {y:.1f})"
            )

    # ------------------------------------------------------------------

    def redraw(self):
        # Just refresh
        self.fig.canvas.draw()
        self.fig.canvas.flush_events()


def main(args=None):
    rclpy.init(args=args)
    node = PlannerVizNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()
    plt.close("all")


if __name__ == "__main__":
    main()
