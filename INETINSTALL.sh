#!/bin/bash
set -euo pipefail

LOG="/tmp/ZMP_INETINSTALL.log"
TMP="/tmp/ZMP_INETINSTALL_$$"
TARGET="/opt/ZPM"

exec > >(tee -a "$LOG") 2>&1
echo "=== ZPM Internet Installer ==="

# ── CLEANUP TRAP ──────────────────────────────────────────────────────────────
cleanup() {
    local exit_code=$?
    rm -rf "$TMP"
    if [ $exit_code -ne 0 ]; then
        echo ""
        echo " Installation failed (exit code: $exit_code). See log: $LOG"
    fi
    exit $exit_code
}
trap cleanup EXIT

# ── ROOT CHECK ────────────────────────────────────────────────────────────────
if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: Run with sudo!"
    exit 1
fi

# ── CONFIRM ───────────────────────────────────────────────────────────────────
read -rp "Start installation of ZPM? [y/n] " odp
if [[ "$odp" != "y" && "$odp" != "Y" ]]; then
    echo "Cancelled."
    exit 0
fi

# ── DEPENDENCIES ─────────────────────────────────────────────────────────────
echo "[*] Installing dependencies..."
apt-get update -y >> "$LOG" 2>&1
apt-get install -y curl git wget >> "$LOG" 2>&1

# ── GET LATEST VERSION ────────────────────────────────────────────────────────
echo "[*] Fetching latest version..."
LATEST=$(curl -fsSL "https://api.github.com/repos/Ignacyyy/ZPM/releases/latest" \
    | grep '"tag_name"' \
    | cut -d '"' -f4)

if [ -z "$LATEST" ]; then
    echo "ERROR: Could not fetch latest version. Check your internet connection."
    exit 1
fi
echo "    Latest: $LATEST"

# ── DOWNLOAD ─────────────────────────────────────────────────────────────────
mkdir -p "$TMP"
cd "$TMP"

echo "[*] Downloading ${LATEST}..."
if ! wget -q \
    "https://github.com/Ignacyyy/Zielina_Package_Manager/archive/refs/tags/${LATEST}.tar.gz" 2>&1; then
    echo "ERROR: Download failed."
    exit 1
fi

echo "[*] Extracting..."
tar -xzf "${LATEST}.tar.gz"

DIR=$(find . -maxdepth 1 -type d -name "Zielina_Package_Manager-*" | head -1)
if [ -z "$DIR" ]; then
    echo "ERROR: Extraction failed — expected directory not found."
    exit 1
fi
cd "$DIR"

# ── INSTALL ───────────────────────────────────────────────────────────────────
echo "[*] Installing to ${TARGET}..."
rm -rf "$TARGET"
mkdir -p "$TARGET"
cp -r . "$TARGET/"

if [ -d "$TARGET/bin" ]; then
    find "$TARGET/bin" -type f -exec chmod +x {} +
else
    echo "WARNING: No bin/ directory found in package."
fi

echo "[*] Updating symlinks in /usr/bin/..."
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

echo "${LATEST#v}" > "$TARGET/VERSION.txt"

echo ""
echo " Installation complete!"
echo "   Version : ${LATEST#v}"
echo "   Path    : ${TARGET}"
echo "   Log     : ${LOG}"
