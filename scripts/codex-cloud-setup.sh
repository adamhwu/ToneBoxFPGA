#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

if ! command -v python3 >/dev/null 2>&1; then
  echo "Python 3 is required" >&2
  exit 1
fi
if ! command -v clang++ >/dev/null 2>&1 && ! command -v g++ >/dev/null 2>&1; then
  echo "A C++17 compiler (clang++ or g++) is required" >&2
  exit 1
fi

if [[ ! -x custom_nam/.venv/bin/python ]]; then
  python3 -m venv custom_nam/.venv
fi

custom_nam/.venv/bin/python custom_nam/verify_headless.py
