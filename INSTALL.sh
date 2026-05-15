#!/bin/bash
set -euo pipefail

LOG="/tmp/ZPM_INSTALL.log"
TARGET="/opt/ZPM"

exec > >(tee -a "$LOG") 2>&1

echo "====== ZPM MANUAL INSTALL ======"

# ── ROOT CHECK ─────────────────────────────
if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: Run with sudo!"
    exit 1
fi

# ── CONFIRM ─────────────────────────────
read -rp "Do you want to continue? [y/N] " odp

if [ "$odp" != "y" ] && [ "$odp" != "Y" ]; then
    echo "Cancelled."
    exit 0
fi

# ── DEPENDENCIES ─────────────────────────────
echo ""
echo "[*] Dependencies list:"
echo "- curl"
echo "- git"
echo "- wget"
echo "- python3"
echo ""

read -rp "Do you want to install dependencies? [y/n] " dep

if [ "$dep" = "y" ] || [ "$dep" = "Y" ]; then
    echo "[*] Installing dependencies..."
    apt-get update -y >> "$LOG" 2>&1
    apt-get install -y curl git wget python3 >> "$LOG" 2>&1
    echo "[+] Dependencies installed."
else
    echo "[!] Skipping dependencies."
fi

# ── SOURCE DIR ─────────────────────────────
SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo ""
echo "[*] Source directory: $SRC_DIR"

# ── INSTALL ─────────────────────────────
echo "[*] Installing to ${TARGET}..."

rm -rf "$TARGET"
mkdir -p "$TARGET"


cp -r "$SRC_DIR/." "$TARGET/"

# ── BIN FIX ─────────────────────────────
if [ -d "$TARGET/bin" ]; then
    find "$TARGET/bin" -type f -exec chmod +x {} +
else
    echo "WARNING: No bin/ directory found."
fi

echo "[*] Updating symlinks in /usr/bin..."

find /usr/bin -maxdepth 1 -type l | while read -r link; do
    if readlink "$link" | grep -q "^${TARGET}/bin/"; then
        rm -f "$link"
    fi
done

if [ -d "$TARGET/bin" ] && [ -n "$(ls -A "$TARGET/bin" 2>/dev/null)" ]; then
    for bin_file in "$TARGET/bin"/*; do
        [ -f "$bin_file" ] || continue
        ln -sf "$bin_file" /usr/bin/"$(basename "$bin_file")"
    done
else
    echo "WARNING: bin/ is empty — no symlinks created."
fi

# ── CLEAN PREVERSION ONLY ─────────────────────────────
echo "[*] Cleaning PREVERSION state..."
rm -f "$TARGET/PREVERSION.txt" 2>/dev/null || true

echo ""
echo " Installation complete!"
echo "Path: $TARGET"
echo "Log : $LOG"
