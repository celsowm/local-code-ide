#!/usr/bin/env bash
set -euo pipefail

# LocalCodeIDE - One-command build + run on Linux
# Detects Qt from aqt ($HOME/Qt) or system packages automatically

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

echo "=== Building ==="
python3 setup.py

echo ""
echo "=== Launching ==="
python3 setup.py run
