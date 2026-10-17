#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

if [[ ! -x ./nbody ]]; then
  echo "Simulasyon derleniyor..."
  ./scripts/build_macos.sh
fi

exec ./nbody
