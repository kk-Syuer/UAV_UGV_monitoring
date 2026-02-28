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
CORE_BACKBONE_TOPICS = [
    "/fanet/network_bus_raw",
    "/fanet/network_bus",
    "/fanet/delivered",
    "/fanet/status",
    "/fanet/routing_table",
    "/routing_manager/alerts",
]


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
    lines.append("  splines=ortho;")
    lines.append("  overlap=false;")
    lines.append("  newrank=true;")
    lines.append("  concentrate=true;")
    lines.append("  ranksep=1.35;")
    lines.append("  nodesep=0.55;")
    lines.append("  graph [fontsize=18, fontname=Helvetica, pad=0.35, bgcolor=white, size=\"11.69,8.27!\", ratio=compress, dpi=300];")
    lines.append("  node [fontname=Helvetica, fontsize=14];")
    lines.append("  edge [fontname=Helvetica, fontsize=11, arrowsize=0.9];")

    by_pkg = {}
    for n in nodes:
        by_pkg.setdefault(n["package"], []).append(n)

    for pkg, members in sorted(by_pkg.items()):
        color = SUBSYSTEM_COLORS.get(pkg, "#FFFFFF")
        lines.append(f'  subgraph cluster_{pkg} {{')
        lines.append(f'    label="{pkg}"; style=filled; color="{color}"; fontsize=16;')
        for n in members:
            nid = f"{n['package']}/{n['executable']}"
            label = f"{nid}\\n[{n.get('node_name_hint') or 'name dynamic'}]"
            lines.append(f'    "{esc(nid)}" [shape=box, style="rounded,filled", fillcolor=white, label="{esc(label)}"];')
        lines.append("  }")

    lines.append("  subgraph cluster_topics {")
    lines.append("    label=\"topics\"; style=\"rounded,dashed\"; color=\"#D0D3D4\"; fontsize=16;")
    for topic, tmeta in sorted(topics.items()):
        tid = topic_node_id(topic)
        label = f"{topic}\\n{tmeta.get('type') or 'unknown'}"
        topic_fill = "#EBF5FB" if topic in BUS_TOPICS else "#F8F9F9"
        lines.append(f'    "{esc(tid)}" [shape=ellipse, label="{esc(label)}", style=filled, fillcolor="{topic_fill}", fontsize=12];')
    lines.append("  }")

    lines.append("  {")
    lines.append("    rank=same;")
    for topic in CORE_BACKBONE_TOPICS:
        if topic in topics:
            lines.append(f'    "{esc(topic_node_id(topic))}";')
    lines.append("  }")

    for i in range(len(CORE_BACKBONE_TOPICS) - 1):
        a = CORE_BACKBONE_TOPICS[i]
        b = CORE_BACKBONE_TOPICS[i + 1]
        if a in topics and b in topics:
            lines.append(
                f'  "{esc(topic_node_id(a))}" -> "{esc(topic_node_id(b))}" '
                ' [style=invis, weight=40, constraint=true];'
            )

    for topic, tmeta in sorted(topics.items()):
        tid = topic_node_id(topic)
        bold = topic in BUS_TOPICS
        style = 'penwidth=3, color="#1F618D", weight=12, minlen=2' if bold else 'penwidth=1, color="#5D6D7E", weight=2'
        for pub in sorted(set(tmeta.get("publishers", []))):
            lines.append(f'  "{esc(pub)}" -> "{esc(tid)}" [{style}, headport=w];')
        for sub in sorted(set(tmeta.get("subscribers", []))):
            lines.append(f'  "{esc(tid)}" -> "{esc(sub)}" [{style}, tailport=e];')

    lines.append("}")
    DOT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote {DOT}")


if __name__ == "__main__":
    main()
