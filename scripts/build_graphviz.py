#!/usr/bin/env python3
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / "artifacts"
INP = ART / "ros_interfaces.json"
DOT = ART / "ros_graph.dot"

SUBSYSTEM_COLORS = {
    "uav_fleet": "#D6EAF8",
    "routing_manager": "#D5F5E3",
    "ugv_charger": "#FDEBD0",
    "sink_gateway": "#FADBD8",
    "coverage_planner": "#E8DAEF",
    "recovery_manager": "#FCF3CF",
    "fault_injector": "#F5CBA7",
    "weather_server": "#D4EFDF",
    "network_monitor": "#EBDEF0",
    "planner_viz": "#D1F2EB",
    "ch_manager": "#EAEDED",
    "user_devices_sim": "#F9E79F",
}
BUS_TOPICS = {"/fanet/network_bus_raw", "/fanet/network_bus", "/fanet/delivered"}


def esc(s: str) -> str:
    return s.replace('"', '\\"')


def topic_node_id(topic: str) -> str:
    safe = topic.replace('/', '_').replace(':', '_').replace('{', '_').replace('}', '_').replace(' ', '_')
    return f"topic{safe}"


def main():
    data = json.loads(INP.read_text())
    nodes = data["nodes"]
    topics = data["topic_inventory"]

    lines = []
    lines.append("digraph ROSComms {")
    lines.append("  rankdir=LR;")
    lines.append("  graph [fontsize=10, fontname=Helvetica];")
    lines.append("  node [fontname=Helvetica];")
    lines.append("  edge [fontname=Helvetica, fontsize=9];")

    by_pkg = {}
    for n in nodes:
        by_pkg.setdefault(n["package"], []).append(n)

    for pkg, members in sorted(by_pkg.items()):
        color = SUBSYSTEM_COLORS.get(pkg, "#FFFFFF")
        lines.append(f'  subgraph cluster_{pkg} {{')
        lines.append(f'    label="{pkg}"; style=filled; color="{color}";')
        for n in members:
            nid = f"{n['package']}/{n['executable']}"
            label = f"{nid}\\n[{n.get('node_name_hint') or 'name dynamic'}]"
            lines.append(f'    "{esc(nid)}" [shape=box, style="rounded,filled", fillcolor=white, label="{esc(label)}"];')
        lines.append("  }")

    for topic, tmeta in sorted(topics.items()):
        tid = topic_node_id(topic)
        label = f"{topic}\\n{tmeta.get('type') or 'unknown'}"
        lines.append(f'  "{esc(tid)}" [shape=ellipse, label="{esc(label)}", style=filled, fillcolor="#F8F9F9"];')

    for topic, tmeta in sorted(topics.items()):
        tid = topic_node_id(topic)
        bold = topic in BUS_TOPICS
        style = 'penwidth=3, color="#1F618D"' if bold else 'penwidth=1'
        for pub in sorted(set(tmeta.get("publishers", []))):
            lines.append(f'  "{esc(pub)}" -> "{esc(tid)}" [{style}];')
        for sub in sorted(set(tmeta.get("subscribers", []))):
            lines.append(f'  "{esc(tid)}" -> "{esc(sub)}" [{style}];')

    lines.append("}")
    DOT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote {DOT}")


if __name__ == "__main__":
    main()
