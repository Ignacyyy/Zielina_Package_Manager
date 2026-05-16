#!/bin/bash
set -euo pipefail

SRC_DIR="$(cd "$(dirname "$0")" && pwd)"
DEST_DIR="/opt/ZPM"
BIN_DIR="$DEST_DIR/bin"
COMMON_DIR="$DEST_DIR/src/common"
SYMLINK_DIR="/usr/bin"

echo " Starting ZPM build..."

# ───────────────────────── DIRS ─────────────────────────

sudo mkdir -p "$BIN_DIR"
sudo mkdir -p "$DEST_DIR/src"

# ───────────────────────── COMMON ─────────────────────────

if [ -d "$SRC_DIR/common" ]; then
    echo " Syncing common..."

    sudo rm -rf "$COMMON_DIR"
    sudo cp -r "$SRC_DIR/common" "$COMMON_DIR"

    echo " common updated"
else
    echo " No common folder found"
fi

# ───────────────────────── BUILD ─────────────────────────

for file in "$SRC_DIR"/*.cpp; do
    [ -e "$file" ] || continue

    filename=$(basename "$file")
    name="${filename%.cpp}"

    echo ""
    echo "⚙ Compiling $filename"

    tmp_out="/tmp/$name"

    # special case: main binary
    if [ "$name" = "ZPM" ]; then
        out_name="zpm"
    else
        out_name="$name"
    fi

    g++ -O2 "$file" -I "$COMMON_DIR" -o "$tmp_out"

    echo " build OK -> $out_name"

    # ───────────────────────── INSTALL ─────────────────────────

    sudo mv -f "$tmp_out" "$BIN_DIR/$out_name"

    sudo ln -sf "$BIN_DIR/$out_name" "$SYMLINK_DIR/$out_name"

    echo " installed -> $SYMLINK_DIR/$out_name"
done

# ───────────────────────── FINISH ─────────────────────────

echo ""
echo " Build complete"
echo " binaries: $BIN_DIR"
echo " symlinks: $SYMLINK_DIR"
