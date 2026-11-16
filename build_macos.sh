#!/usr/bin/env bash
set -euo pipefail
exec bash "$(dirname "$0")/tools/build_unix.sh" macos "$@"
