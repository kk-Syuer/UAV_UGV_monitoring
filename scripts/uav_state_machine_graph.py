#!/usr/bin/env python3
"""Generate a node graph for the UAV state machine and save it to artifacts/."""

from pathlib import Path


def circle_node(cx: int, cy: int, r: int, label: str) -> str:
    return f'''
  <circle cx="{cx}" cy="{cy}" r="{r}" fill="#8ecae6" stroke="#023047" stroke-width="2" />
  <text x="{cx}" y="{cy + 5}" text-anchor="middle" font-family="Arial" font-size="16" font-weight="bold" fill="#023047">{label}</text>'''


def arrow(x1: int, y1: int, x2: int, y2: int) -> str:
    return f'\n  <line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="#023047" stroke-width="3" marker-end="url(#arrow)" />'


def main() -> None:
    output_path = Path("artifacts/uav_state_machine_node_graph.svg")
    output_path.parent.mkdir(parents=True, exist_ok=True)

    svg = f'''<svg xmlns="http://www.w3.org/2000/svg" width="1000" height="500" viewBox="0 0 1000 500">
  <defs>
    <marker id="arrow" markerWidth="12" markerHeight="12" refX="10" refY="6" orient="auto" markerUnits="strokeWidth">
      <path d="M0,0 L0,12 L12,6 z" fill="#023047" />
    </marker>
  </defs>
  <rect width="100%" height="100%" fill="#ffffff" />
  <text x="500" y="45" text-anchor="middle" font-family="Arial" font-size="28" font-weight="bold" fill="#1d3557">UAV State Machine</text>
{arrow(220, 180, 380, 180)}
{arrow(500, 230, 500, 340)}
{arrow(380, 390, 220, 390)}
{arrow(120, 340, 120, 230)}
{circle_node(120, 180, 80, 'Active')}
{circle_node(500, 180, 90, 'Going to UGV')}
{circle_node(500, 390, 80, 'Charging')}
{circle_node(120, 390, 80, 'Returning')}
</svg>
'''

    output_path.write_text(svg, encoding="utf-8")
    print(f"Saved graph to {output_path}")


if __name__ == "__main__":
    main()
