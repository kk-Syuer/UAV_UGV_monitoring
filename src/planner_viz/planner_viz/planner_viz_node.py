#!/usr/bin/env python3
import math
from typing import Dict

import rclpy
from rclpy.node import Node

import matplotlib.pyplot as plt

from uav_msgs.msg import UavStatus


class PlannerVizNode(Node):
    def __init__(self) -> None:
        super().__init__("planner_viz_node")

        # ---- Area & parameters (match coverage_planner) ----
        self.x_min = self.declare_parameter("x_min", 0.0).get_parameter_value().double_value
        self.x_max = self.declare_parameter("x_max", 600.0).get_parameter_value().double_value
        self.y_min = self.declare_parameter("y_min", 0.0).get_parameter_value().double_value
        self.y_max = self.declare_parameter("y_max", 600.0).get_parameter_value().double_value

        self.service_radius_ch = (
            self.declare_parameter("service_radius_ch", 250.0)
            .get_parameter_value()
            .double_value
        )

        # Sink & UGV positions
        self.sink_x = self.declare_parameter("sink_x", 300.0).get_parameter_value().double_value
        self.sink_y = self.declare_parameter("sink_y", 300.0).get_parameter_value().double_value

        self.ugv_x = self.declare_parameter("ugv_x", 50.0).get_parameter_value().double_value
        self.ugv_y = self.declare_parameter("ugv_y", 50.0).get_parameter_value().double_value

        # Storage for UAVs: uav_id -> dict(role, x, y, cluster_id)
        self.uavs: Dict[str, Dict] = {}

        # Subscribe to UAV status (periodic, dynamic)
        self.status_sub = self.create_subscription(
            UavStatus,
            "/uav_fleet/status",
            self.status_callback,
            50,
        )

        # Matplotlib setup
        plt.ion()
        self.fig, self.ax = plt.subplots()
        self._init_axes()

        # Timer to refresh plot
        self.timer = self.create_timer(0.5, self.update_plot)

        self.get_logger().info(
            f"planner_viz started. Area=([{self.x_min},{self.x_max}] x "
            f"[{self.y_min},{self.y_max}]), sink=({self.sink_x},{self.sink_y}), "
            f"ugv=({self.ugv_x},{self.ugv_y})"
        )

    # ---- Callbacks ----

    def status_callback(self, msg: UavStatus) -> None:
        """Store latest UAV pose/role/cluster from status."""
        self.uavs[msg.uav_id] = {
            "role": msg.role,               # 0 = MEMBER, 1 = CH
            "x": msg.pose.position.x,
            "y": msg.pose.position.y,
            "cluster": msg.cluster_id,
        }

    # ---- Plot helpers ----

    def _init_axes(self) -> None:
        self.ax.set_facecolor("black")
        self.ax.set_xlim(self.x_min, self.x_max)
        self.ax.set_ylim(self.y_min, self.y_max)
        self.ax.set_aspect("equal", adjustable="box")
        self.ax.set_xlabel("X [m]")
        self.ax.set_ylabel("Y [m]")
        self.ax.set_title("Coverage planner view")

    def update_plot(self) -> None:
        """Redraw the plot with current UAV positions."""
        self.ax.cla()
        self._init_axes()

        # Sink (blue)
        self.ax.scatter(
            [self.sink_x],
            [self.sink_y],
            c="blue",
            marker="o",
            s=60,
            label="sink",
        )

        # UGV (yellow)
        self.ax.scatter(
            [self.ugv_x],
            [self.ugv_y],
            c="yellow",
            marker="^",
            s=70,
            label="ugv",
        )

        # UAVs
        for uav_id, info in self.uavs.items():
            x = info["x"]
            y = info["y"]
            role = info["role"]

            if role == 1:
                # Cluster Head: red dot + coverage circle
                self.ax.scatter([x], [y], c="red", marker="o", s=50)
                circ = plt.Circle(
                    (x, y),
                    self.service_radius_ch,
                    edgecolor="red",
                    facecolor="none",
                    linestyle="--",
                    linewidth=1,
                    alpha=0.7,
                )
                self.ax.add_patch(circ)
                self.ax.text(x, y, uav_id, color="white", fontsize=8, ha="center", va="center")
            else:
                # Member: green dot
                self.ax.scatter([x], [y], c="green", marker="o", s=30)
                self.ax.text(x, y, uav_id, color="white", fontsize=7, ha="center", va="center")

        self.ax.legend(loc="upper right", fontsize=8)

        self.fig.canvas.draw()
        plt.pause(0.001)


def main(args=None) -> None:
    rclpy.init(args=args)
    node = PlannerVizNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
