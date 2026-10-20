#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

if [[ ! -x ./nbody3d ]]; then
  echo "3D simulasyon derleniyor..."
  ./scripts/build_macos_3d.sh
fi

exec ./nbody3d
