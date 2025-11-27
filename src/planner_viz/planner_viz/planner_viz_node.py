#!/usr/bin/env python3
import math
from typing import Dict

import rclpy
from rclpy.node import Node

import matplotlib.pyplot as plt

from uav_msgs.msg import UavDeployment


class PlannerVizNode(Node):
    def __init__(self) -> None:
        super().__init__("planner_viz_node")

        # Area & parameters (match coverage_planner)
        self.x_min = self.declare_parameter("x_min", 0.0).get_parameter_value().double_value
        self.x_max = self.declare_parameter("x_max", 500.0).get_parameter_value().double_value
        self.y_min = self.declare_parameter("y_min", 0.0).get_parameter_value().double_value
        self.y_max = self.declare_parameter("y_max", 500.0).get_parameter_value().double_value

        self.service_radius_ch = (
            self.declare_parameter("service_radius_ch", 250.0)
            .get_parameter_value()
            .double_value
        )

        # Sink & UGV positions
        self.sink_x = self.declare_parameter("sink_x", 0.0).get_parameter_value().double_value
        self.sink_y = self.declare_parameter("sink_y", 0.0).get_parameter_value().double_value

        self.ugv_x = self.declare_parameter("ugv_x", 0.0).get_parameter_value().double_value
        self.ugv_y = self.declare_parameter("ugv_y", 0.0).get_parameter_value().double_value

        # Storage for UAVs: uav_id -> dict(role, x, y, cluster_id)
        self.uavs: Dict[str, Dict] = {}

        self.sub_ = self.create_subscription(
            UavDeployment,
            "/coverage_planner/deployment",
            self.deployment_callback,
            10,
        )

        # Setup matplotlib figure
        plt.ion()
        self.fig, self.ax = plt.subplots()
        self.ax.set_facecolor("black")
        self.ax.set_xlim(self.x_min, self.x_max)
        self.ax.set_ylim(self.y_min, self.y_max)
        self.ax.set_aspect("equal", adjustable="box")
        self.ax.set_xlabel("X [m]")
        self.ax.set_ylabel("Y [m]")
        self.ax.set_title("Coverage planner view")

        # Timer to refresh the plot
        self.timer = self.create_timer(0.5, self.update_plot)

        self.get_logger().info(
            f"planner_viz started. Area=([{self.x_min},{self.x_max}] x "
            f"[{self.y_min},{self.y_max}]), sink=({self.sink_x},{self.sink_y}), "
            f"ugv=({self.ugv_x},{self.ugv_y})"
        )

    # ---- Callbacks ----

    def deployment_callback(self, msg: UavDeployment) -> None:
        # Store/update UAV info
        self.uavs[msg.uav_id] = {
            "role": msg.role,
            "x": msg.target_pose.position.x,
            "y": msg.target_pose.position.y,
            "cluster": msg.cluster_id,
        }

        self.get_logger().info(
            f"Deployment: {msg.uav_id} role={msg.role} cluster={msg.cluster_id} "
            f"pos=({msg.target_pose.position.x:.1f}, {msg.target_pose.position.y:.1f})"
        )

    # ---- Plotting ----

    def update_plot(self) -> None:
        # Clear axes
        self.ax.cla()
        self.ax.set_facecolor("black")
        self.ax.set_xlim(self.x_min, self.x_max)
        self.ax.set_ylim(self.y_min, self.y_max)
        self.ax.set_aspect("equal", adjustable="box")
        self.ax.set_xlabel("X [m]")
        self.ax.set_ylabel("Y [m]")
        self.ax.set_title("Coverage planner view")

        # Plot sink (blue)
        self.ax.scatter(
            [self.sink_x],
            [self.sink_y],
            c="blue",
            marker="o",
            s=60,
            label="sink",
        )

        # Plot UGV (yellow)
        self.ax.scatter(
            [self.ugv_x],
            [self.ugv_y],
            c="yellow",
            marker="^",
            s=70,
            label="ugv",
        )

        # Plot UAVs: CHs red + circles; members green
        for uav_id, info in self.uavs.items():
            x = info["x"]
            y = info["y"]
            role = info["role"]

            if role == 1:
                # CH
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
                # Member
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
