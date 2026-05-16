#!/bin/bash
set -euo pipefail

# ───────────────────────── CONFIG ─────────────────────────

DEST_DIR="/opt/ZPM"
BIN_DIR="$DEST_DIR/bin"
COMMON_DEST="$DEST_DIR/src/common"
SYMLINK_DIR="/usr/bin"

# ───────────────────────── RESOLVE SOURCE ─────────────────────────

SRC_DIR="$(pwd)"
SRC_REAL="$(realpath "$SRC_DIR")"
DEST_REAL="$(realpath "$DEST_DIR")"

echo " ZPM bulletproof build starting..."
echo " SRC : $SRC_REAL"
echo " DEST: $DEST_REAL"

# ───────────────────────── SAFETY LAYER 0 ─────────────────────────
# never allow source == system install root

if [[ "$SRC_REAL" == "$DEST_REAL" ]]; then
    echo " CRITICAL: Source equals install root!"
    echo " Move project outside /opt/ZPM/src or reinstall cleanly"
    exit 1
fi

# ───────────────────────── SAFETY LAYER 1 ─────────────────────────
# block DEST inside source tree (recursive destruction risk)

if [[ "$DEST_REAL" == "$SRC_REAL"* ]]; then
    echo " CRITICAL: DEST is inside SOURCE tree"
    exit 1
fi

# ───────────────────────── SAFETY LAYER 2 ─────────────────────────
# ensure we actually have a C++ project

if ! compgen -G "$SRC_DIR/*.cpp" > /dev/null; then
    echo " ERROR: No .cpp files found in source directory"
    exit 1
fi

# ───────────────────────── CREATE INSTALL STRUCTURE ─────────────────────────

sudo mkdir -p "$BIN_DIR"
sudo mkdir -p "$DEST_DIR/src"

# ───────────────────────── SYNC COMMON (SAFE COPY) ─────────────────────────

if [ -d "$SRC_DIR/common" ]; then
    echo " Syncing common..."

    sudo rm -rf "$COMMON_DEST"
    sudo mkdir -p "$COMMON_DEST"
    sudo cp -r "$SRC_DIR/common/." "$COMMON_DEST/"

    echo " common synced safely"
else
    echo " No common folder found in source"
fi

# ───────────────────────── BUILD STAGE (NO INSTALL YET) ─────────────────────────

BUILD_TMP="$(mktemp -d)"
echo " Build staging: $BUILD_TMP"

trap 'echo " build failed, cleaning staging..."; rm -rf "$BUILD_TMP"' EXIT

for file in "$SRC_DIR"/*.cpp; do
    [ -e "$file" ] || continue

    filename="$(basename "$file")"
    name="${filename%.cpp}"

    echo ""
    echo "⚙ compiling $filename"

    g++ -O2 "$file" \
        -I "$COMMON_DEST" \
        -o "$BUILD_TMP/$name"

    echo " built -> $name"
done

# ───────────────────────── INSTALL PHASE ─────────────────────────

echo ""
echo " Installing binaries..."

for bin in "$BUILD_TMP"/*; do
    [ -e "$bin" ] || continue

    name="$(basename "$bin")"

    # rename main binary
    if [[ "$name" == "ZPM" ]]; then
        name="zpm"
    fi

    sudo mv -f "$bin" "$BIN_DIR/$name"
    sudo ln -sf "$BIN_DIR/$name" "$SYMLINK_DIR/$name"

    echo " installed -> $SYMLINK_DIR/$name"
done

# cleanup staging
rm -rf "$BUILD_TMP"

# ───────────────────────── FINAL CHECK ─────────────────────────

if [ ! -d "$BIN_DIR" ]; then
    echo " CRITICAL: install failed (bin missing)"
    exit 1
fi

echo ""
echo " ZPM BULLETPROOF BUILD COMPLETE"
echo " $BIN_DIR"
echo " $SYMLINK_DIR"
