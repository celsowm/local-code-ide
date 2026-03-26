#!/usr/bin/env bash
set -euo pipefail

# LocalCodeIDE - Quick build on Linux (auto-detects Qt)

cd "$(dirname "$0")"

# Auto-detect aqt Qt and export if found
for ver_dir in "$HOME/Qt"/*/gcc_*; do
    if [ -d "$ver_dir/lib/cmake/Qt6" ]; then
        export QT_DIR="$ver_dir"
        export LD_LIBRARY_PATH="$ver_dir/lib:${LD_LIBRARY_PATH:-}"
        export QML_IMPORT_PATH="$ver_dir/qml"
        break
    fi
done

exec python3 setup.py
