#!/usr/bin/env bash
set -euo pipefail

python3 tools/clean_test_round1_data.py \
  --input-root experiment_data_collection/test_round1 \
  --output-root experiment_data_collection/test_round1_clean \
  "$@"
