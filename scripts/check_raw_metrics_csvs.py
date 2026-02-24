#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path

import pandas as pd


def main() -> None:
    parser = argparse.ArgumentParser(description="Verify raw_metrics CSVs are readable and print columns")
    parser.add_argument("--input", type=Path, default=Path("artifacts/raw_metrics"))
    args = parser.parse_args()

    root = args.input
    if not root.exists() or not root.is_dir():
        raise SystemExit(f"ERROR: input directory does not exist or is not a directory: {root}")

    csvs = sorted(root.glob("*.csv"))
    if not csvs:
        raise SystemExit(f"ERROR: no CSV files found in {root}")

    print(f"Found {len(csvs)} CSV files in {root}:")
    for csv_path in csvs:
        print(f"- {csv_path.name}")

    for csv_path in csvs:
        try:
            df = pd.read_csv(csv_path)
        except Exception as exc:
            raise SystemExit(f"ERROR: failed to read {csv_path}: {exc}") from exc
        print(f"{csv_path.name}: columns={list(df.columns)}")

    print("OK: all CSVs are readable.")


if __name__ == "__main__":
    main()
