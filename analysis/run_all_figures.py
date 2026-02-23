#!/usr/bin/env python3
"""Run all analysis figure scripts sequentially.

# requirements.txt snippet:
# pandas>=1.5
# numpy>=1.23
# matplotlib>=3.6
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run all figure scripts for round1")
    parser.add_argument("--data_root", required=True, type=Path)
    parser.add_argument("--out_dir", type=Path, default=Path("analysis/test_round1"))
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    cmds = [
        [sys.executable, "-m", "analysis.plot_network_timeseries", "--data_root", str(args.data_root), "--out_dir", str(args.out_dir), "--metric", metric, "--flow_filter", "ALL"]
        for metric in ("pdr", "delay", "jitter")
    ]

    for cmd in cmds:
        print("Running:", " ".join(cmd))
        proc = subprocess.run(cmd)
        if proc.returncode != 0:
            return proc.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
