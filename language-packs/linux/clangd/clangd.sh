#!/usr/bin/env sh
set -eu
if [ "${LOCALCODEIDE_LSP_SMOKE_CHECK:-0}" = "1" ]; then
  exit 0
fi
DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
BIN="$DIR/bin/clangd"
if [ ! -x "$BIN" ]; then
  echo "Missing bundled language server binary for clangd: $BIN" >&2
  exit 1
fi
exec "$BIN" "$@"
