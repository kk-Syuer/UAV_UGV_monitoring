#!/usr/bin/env python3
"""Plot message-level metric distributions by policy (violin plot).

# requirements.txt snippet:
# pandas>=1.5
# numpy>=1.23
# matplotlib>=3.6
"""

from __future__ import annotations

from tools.plot_message_distributions import run


def main() -> int:
    return run(kind="violin")


if __name__ == "__main__":
    raise SystemExit(main())
