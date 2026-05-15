#!/bin/bash

SRC_DIR="$(pwd)"
DEST_DIR="/opt/ZPM"
BIN_DIR="$DEST_DIR/bin"
SYMLINK_DIR="/usr/bin"

echo "Starting ZPM build..."

# ───────────────────────── DIRS ─────────────────────────

sudo mkdir -p "$BIN_DIR"
sudo mkdir -p "$DEST_DIR/src"

# ───────────────────────── COMMON ─────────────────────────

if [ -d "$SRC_DIR/common" ]; then
    echo "Copying common folder..."

    sudo rm -rf "$DEST_DIR/src/common"
    sudo cp -r "$SRC_DIR/common" "$DEST_DIR/src/"

    echo "Common copied."
else
    echo "No common folder found."
fi

# ───────────────────────── BUILD ALL ─────────────────────────

for file in "$SRC_DIR"/*.cpp; do
    [ -e "$file" ] || continue

    filename=$(basename -- "$file")
    name="${filename%.cpp}"

    echo ""
    echo "Compiling $filename -> $name"

    sudo g++ -O2 "$file" -I "$DEST_DIR/src/common" -o "$name"

    if [ $? -ne 0 ]; then
        echo "Compilation failed: $filename"
        continue
    fi

    echo "Build OK."

    # ───────────────────────── CLEAN OLD VERSION ─────────────────────────

    sudo rm -f "$BIN_DIR/$name"
    sudo rm -f "$SYMLINK_DIR/$name"

    # ───────────────────────── INSTALL NEW ─────────────────────────

    sudo mv -f "$name" "$BIN_DIR/"

    # ───────────────────────── SYMLINK ─────────────────────────

    sudo ln -sf "$BIN_DIR/$name" "$SYMLINK_DIR/$name"

    echo "Installed -> $SYMLINK_DIR/$name"
done

# ───────────────────────── CLEAN CACHE ─────────────────────────

hash -r 2>/dev/null

echo ""
echo "Build complete."
echo "Binaries location: $BIN_DIR"
echo "Symlinks: $SYMLINK_DIR"