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
echo "=== Preflight ==="
export LOCALCODEIDE_DOCTOR_SKIP_STARTUP=1
export LOCALCODEIDE_LOG_LEVEL="${LOCALCODEIDE_LOG_LEVEL:-info}"
export LOCALCODEIDE_KEEP_LOG_FILES="${LOCALCODEIDE_KEEP_LOG_FILES:-20}"
python3 setup.py doctor
unset LOCALCODEIDE_DOCTOR_SKIP_STARTUP

echo ""
echo "=== Launching ==="
set +e
python3 setup.py run
exit_code=$?
set -e

if [ "$exit_code" -ne 0 ]; then
    echo
    echo "============================================================"
    echo "LocalCodeIDE crashed or exited with error code $exit_code."
    echo "============================================================"
    latest_log_dir="$(ls -1dt build/logs/*/ 2>/dev/null | head -n 1 || true)"
    if [ -n "$latest_log_dir" ] && [ -f "${latest_log_dir}runtime.log" ]; then
        echo "Latest session: ${latest_log_dir%/}"
        echo
        echo "Last log lines:"
        tail -n 30 "${latest_log_dir}runtime.log" || true
    else
        echo "No run session logs found in build/logs."
    fi
    echo
    read -r -p "Press Enter to close..."
fi

exit "$exit_code"
