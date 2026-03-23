#!/usr/bin/env sh
set -eu
if [ "${LOCALCODEIDE_LSP_SMOKE_CHECK:-0}" = "1" ]; then
  exit 0
fi
DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
BIN="$DIR/python/bin/cmake-language-server"
if [ ! -x "$BIN" ]; then
  echo "Missing bundled language server binary for cmake-language-server: $BIN" >&2
  exit 1
fi
exec "$BIN" "$@"
