#!/usr/bin/env bash
# Ecossistema SisTer — MIRANTE Launcher Script
# Copyright (C) 2026 José Pedro Trindade

set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

if [[ -f .env ]]; then
  set -a
  # shellcheck source=/dev/null
  source .env
  set +a
fi

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

HOST="${MIRANTE_HOST:-0.0.0.0}"
PORT="${MIRANTE_PORT:-8088}"

exec ./build/mirante-server --host "$HOST" --port "$PORT" "$@"
