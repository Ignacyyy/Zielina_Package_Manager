#!/bin/bash
set -euo pipefail

LOG="/tmp/ZPM_INSTALL.log"
TARGET="/opt/ZPM"

if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: Run with sudo!"
    exit 1
fi

echo "====== ZPM manual install ======"
read -rp "Do you want to continue? [y/N] " odp

if [ "$odp" != "y" ] && [ "$odp" != "Y" ]; then
    echo "Cancelled."
    exit 0
fi

echo "[*] Installing dependencies..."
apt-get update -y >> "$LOG" 2>&1
apt-get install -y curl git wget python3 >> "$LOG" 2>&1

echo "[*] Installing to ${TARGET}..."

rm -rf "$TARGET"
mkdir -p "$TARGET"
cp -r . "$TARGET/"

echo "[*] Updating symlinks in /usr/bin..."
find /usr/bin -maxdepth 1 -type l | while read -r link; do
    if readlink "$link" | grep -q "^${TARGET}/bin/"; then
        rm -f "$link"
    fi
done

if [ -d "$TARGET/bin" ] && [ -n "$(ls -A "$TARGET/bin" 2>/dev/null)" ]; then
    find "$TARGET/bin" -type f -exec chmod +x {} +

    for bin_file in "$TARGET/bin"/*; do
        [ -f "$bin_file" ] || continue
        ln -sf "$bin_file" /usr/bin/"$(basename "$bin_file")"
    done
else
    echo "WARNING: bin/ is empty — no symlinks created."
fi

rm -f "$TARGET/PREVERSION.txt" 2>/dev/null || true

echo ""
echo "Installation complete! 🎉"
echo "Path: $TARGET"
echo "Log : $LOG"
