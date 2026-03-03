#!/usr/bin/env python3
"""Generate UAV state machine graph artifacts using weather-graph visual style."""

from pathlib import Path


ARTIFACTS_DIR = Path("artifacts")
DOT_PATH = ARTIFACTS_DIR / "uav_state_machine_node_graph.dot"
SVG_PATH = ARTIFACTS_DIR / "uav_state_machine_node_graph.svg"


def build_dot() -> str:
    return """digraph UAVStateMachine {
  rankdir=LR;
  bgcolor="white";
  splines=true;
  overlap=false;
  node [shape=circle, style=filled, fillcolor="#f4f8ff", color="#2f5f98", penwidth=1.8, fontname="Helvetica", fontsize=13];
  edge [color="#556270", fontname="Helvetica", fontsize=11, arrowsize=0.8, penwidth=2.15];

  active [label="Active"];
  going_to_ugv [label="Going to UGV"];
  charging [label="Charging"];
  returning [label="Returning"];

  active -> going_to_ugv [label="low battery"];
  going_to_ugv -> charging [label="dock"];
  charging -> returning [label="charged"];
  returning -> active [label="resume mission"];
}
"""


def build_svg() -> str:
    return """<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="980" height="420" viewBox="0 0 980 420">
<defs>
<marker id="arrow" markerWidth="10" markerHeight="8" refX="9" refY="4" orient="auto" markerUnits="strokeWidth">
<path d="M0,0 L10,4 L0,8 z" fill="#445566"/>
</marker>
</defs>
<rect width="100%" height="100%" fill="white"/>

<path d="M 204 120 Q 315 95 420 120" fill="none" stroke="#445566" stroke-width="2.15" marker-end="url(#arrow)" opacity="0.74"/>
<rect x="285" y="86" width="76" height="18" rx="4" ry="4" fill="white" fill-opacity="0.92" stroke="#c7d4e2" stroke-width="0.8"/>
<text x="323" y="99" text-anchor="middle" font-family="Helvetica" font-size="12" font-weight="700" fill="#162635">low battery</text>

<path d="M 560 152 Q 650 210 740 268" fill="none" stroke="#445566" stroke-width="2.15" marker-end="url(#arrow)" opacity="0.74"/>
<rect x="643" y="195" width="36" height="18" rx="4" ry="4" fill="white" fill-opacity="0.92" stroke="#c7d4e2" stroke-width="0.8"/>
<text x="661" y="208" text-anchor="middle" font-family="Helvetica" font-size="12" font-weight="700" fill="#162635">dock</text>

<path d="M 740 332 Q 650 360 560 332" fill="none" stroke="#445566" stroke-width="2.15" marker-end="url(#arrow)" opacity="0.74"/>
<rect x="632" y="357" width="52" height="18" rx="4" ry="4" fill="white" fill-opacity="0.92" stroke="#c7d4e2" stroke-width="0.8"/>
<text x="658" y="370" text-anchor="middle" font-family="Helvetica" font-size="12" font-weight="700" fill="#162635">charged</text>

<path d="M 420 300 Q 312 274 204 300" fill="none" stroke="#445566" stroke-width="2.15" marker-end="url(#arrow)" opacity="0.74"/>
<rect x="270" y="323" width="93" height="18" rx="4" ry="4" fill="white" fill-opacity="0.92" stroke="#c7d4e2" stroke-width="0.8"/>
<text x="316.5" y="336" text-anchor="middle" font-family="Helvetica" font-size="12" font-weight="700" fill="#162635">resume mission</text>

<circle cx="160" cy="210" r="52" fill="#f4f8ff" stroke="#2f5f98" stroke-width="2.2"/>
<text x="160" y="215" text-anchor="middle" font-family="Helvetica" font-size="14" font-weight="700" fill="#1d3148">Active</text>

<circle cx="500" cy="120" r="62" fill="#f4f8ff" stroke="#2f5f98" stroke-width="2.2"/>
<text x="500" y="125" text-anchor="middle" font-family="Helvetica" font-size="14" font-weight="700" fill="#1d3148">Going to UGV</text>

<circle cx="780" cy="300" r="52" fill="#f4f8ff" stroke="#2f5f98" stroke-width="2.2"/>
<text x="780" y="305" text-anchor="middle" font-family="Helvetica" font-size="14" font-weight="700" fill="#1d3148">Charging</text>

<circle cx="500" cy="300" r="56" fill="#f4f8ff" stroke="#2f5f98" stroke-width="2.2"/>
<text x="500" y="305" text-anchor="middle" font-family="Helvetica" font-size="14" font-weight="700" fill="#1d3148">Returning</text>
</svg>
"""


def main() -> None:
    ARTIFACTS_DIR.mkdir(parents=True, exist_ok=True)
    DOT_PATH.write_text(build_dot(), encoding="utf-8")
    SVG_PATH.write_text(build_svg(), encoding="utf-8")
    print(f"Saved graph definitions to {DOT_PATH} and {SVG_PATH}")


if __name__ == "__main__":
    main()
