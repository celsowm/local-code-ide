#!/usr/bin/env bash
set -euo pipefail

# LocalCodeIDE - AppImage packaging script
# Usage: scripts/build-appimage.sh <version>
# Prerequisites: built binary at build/LocalCodeIDE

VERSION="${1:?Usage: build-appimage.sh <version>}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
APPDIR="$BUILD_DIR/AppDir"
APPIMAGE_NAME="LocalCodeIDE-${VERSION}.AppImage"

cd "$PROJECT_DIR"

echo "=== Building AppImage v${VERSION} ==="

# Check binary exists
BINARY_PATH="$BUILD_DIR/bin/LocalCodeIDE"
if [ ! -f "$BINARY_PATH" ]; then
    # Fallback to old location (Windows-style or no RUNTIME_OUTPUT_DIRECTORY)
    BINARY_PATH="$BUILD_DIR/LocalCodeIDE"
    if [ ! -f "$BINARY_PATH" ]; then
        echo "ERROR: LocalCodeIDE binary not found. Build the project first."
        exit 1
    fi
fi

# Download linuxdeploy if not present
LINUXDEPLOY="$BUILD_DIR/linuxdeploy-x86_64.AppImage"
if [ ! -f "$LINUXDEPLOY" ]; then
    echo "Downloading linuxdeploy..."
    curl -sSL -o "$LINUXDEPLOY" \
        "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    chmod +x "$LINUXDEPLOY"
fi

# Download linuxdeploy-plugin-qt if not present
LINUXDEPLOY_QT="$BUILD_DIR/linuxdeploy-plugin-qt-x86_64.AppImage"
if [ ! -f "$LINUXDEPLOY_QT" ]; then
    echo "Downloading linuxdeploy-plugin-qt..."
    curl -sSL -o "$LINUXDEPLOY_QT" \
        "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
    chmod +x "$LINUXDEPLOY_QT"
fi

# Clean previous AppDir
rm -rf "$APPDIR"

# Create AppDir structure
echo "Creating AppDir structure..."
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"
mkdir -p "$APPDIR/usr/share/local-code-ide/language-packs"

# Copy binary
cp "$BINARY_PATH" "$APPDIR/usr/bin/"
chmod +x "$APPDIR/usr/bin/LocalCodeIDE"

# Copy language packs
if [ -d "resources/language-packs" ]; then
    cp -r resources/language-packs/* "$APPDIR/usr/share/local-code-ide/language-packs/"
fi
if [ -d "language-packs" ]; then
    cp -r language-packs/* "$APPDIR/usr/share/local-code-ide/language-packs/" 2>/dev/null || true
fi
# Make linux launcher scripts executable
find "$APPDIR/usr/share/local-code-ide/language-packs/linux" -name "*.sh" -exec chmod +x {} \; 2>/dev/null || true

# Create desktop entry
cat > "$APPDIR/usr/share/applications/local-code-ide.desktop" << 'EOF'
[Desktop Entry]
Type=Application
Name=LocalCodeIDE
Exec=LocalCodeIDE
Icon=local-code-ide
Categories=Development;IDE;
Comment=Local-first IDE with AI assistance
Terminal=false
EOF

# Copy icon
if [ -f "assets/local-code-ide-logo-normalized.png" ]; then
    cp "assets/local-code-ide-logo-normalized.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/local-code-ide.png"
fi

# Use linuxdeploy to bundle Qt libraries
echo "Bundling Qt libraries with linuxdeploy..."
export QML_SOURCES_PATHS="$PROJECT_DIR/qml"
export EXTRA_QT_PLUGINS=""

"$LINUXDEPLOY" \
    --appdir "$APPDIR" \
    --desktop-file "$APPDIR/usr/share/applications/local-code-ide.desktop" \
    --icon-file "$APPDIR/usr/share/icons/hicolor/256x256/apps/local-code-ide.png" \
    --plugin qt \
    --output appimage

# Rename the output AppImage
GENERATED_APPIMAGE=$(find "$BUILD_DIR" -maxdepth 1 -name "LocalCodeIDE*.AppImage" | head -1)
if [ -n "$GENERATED_APPIMAGE" ]; then
    mv "$GENERATED_APPIMAGE" "$BUILD_DIR/$APPIMAGE_NAME"
fi

if [ -f "$BUILD_DIR/$APPIMAGE_NAME" ]; then
    echo ""
    echo "=== AppImage built successfully ==="
    echo "Output: $BUILD_DIR/$APPIMAGE_NAME"
    echo "Size: $(du -h "$BUILD_DIR/$APPIMAGE_NAME" | cut -f1)"
else
    echo "WARNING: AppImage may have been created with a different name. Check $BUILD_DIR/"
fi
