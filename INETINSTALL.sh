#!/bin/bash
set -euo pipefail

LOG="/tmp/ZMP_INETINSTALL.log"
TMP="/tmp/ZMP_INETINSTALL_$$"
TARGET="/opt/ZPM"

exec > >(tee -a "$LOG") 2>&1
echo "===== ZPM Internet Installer ====="

# ── CLEANUP TRAP ─────────────────────────────
cleanup() {
    local exit_code=$?
    rm -rf "$TMP"

    if [ "$exit_code" -ne 0 ]; then
        echo ""
        echo "Installation failed (exit code: $exit_code). See log: $LOG"
    fi

    exit "$exit_code"
}
trap cleanup EXIT

# ── ROOT CHECK ─────────────────────────────
if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: Run with sudo!"
    exit 1
fi

# ── CONFIRM INSTALL ─────────────────────────────
read -rp "Start installation of ZPM? [y/n] " odp

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
    echo "[*] Updating package lists..."
    apt-get update -y >> "$LOG" 2>&1

    echo "[*] Installing packages..."
    apt-get install -y curl git wget python3 >> "$LOG" 2>&1

    echo "[+] Dependencies installed successfully."
else
    echo "[!] Skipping dependencies."
fi

# ── GET LATEST VERSION ─────────────────────────────
echo ""
echo "[*] Fetching latest version..."

LATEST=$(curl -fsSL "https://api.github.com/repos/Ignacyyy/ZPM/releases/latest" \
    | grep '"tag_name"' \
    | cut -d '"' -f4)

if [ -z "${LATEST:-}" ]; then
    echo "ERROR: Could not fetch latest version."
    exit 1
fi

echo "Latest: $LATEST"

# ── DOWNLOAD ─────────────────────────────
mkdir -p "$TMP"
cd "$TMP"

echo "[*] Downloading ${LATEST}..."

wget -q \
    "https://github.com/Ignacyyy/ZPM/archive/refs/tags/${LATEST}.tar.gz" \
    -O "${LATEST}.tar.gz" >> "$LOG" 2>&1

echo "[*] Extracting..."
tar -xzf "${LATEST}.tar.gz"

DIR=$(find . -maxdepth 1 -type d -name "ZPM-*" | head -n 1)

if [ -z "$DIR" ]; then
    echo "ERROR: Extraction failed."
    exit 1
fi

cd "$DIR"

# ── INSTALL ─────────────────────────────
echo "[*] Installing to ${TARGET}..."

rm -rf "$TARGET"
mkdir -p "$TARGET"
cp -r . "$TARGET/"

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
# ── ARM SYSTEMS ─────────────────────────────
echo "===== ZPM ARM Compatibility ====="

read -rp "Do you want to recompile ZPM for ARM systems? [y/n] " rec

if [ "$rec" = "y" ] || [ "$rec" = "Y" ]; then

    if [ ! -f /opt/ZPM/src/build.sh ]; then
        echo "ERROR: build.sh not found."
        rm -f "$TARGET/PREVERSION.txt" 2>/dev/null || true
        exit 1
    fi

    echo "[*] Recompiling ZPM..."
    echo "[*] Recompiling Programs...(it can take a while....)"

    cd /opt/ZPM/src
    bash build.sh >> "$LOG" 2>&1

    echo "[*] Cleanup trash"

    cd ~
fi
# ── VERSION CLEAN STATE ─────────────────────────────
echo "${LATEST#v}" > "$TARGET/VERSION.txt"
rm -f "$TARGET/PREVERSION.txt" 2>/dev/null || true

echo ""
echo "Installation complete!"
echo "Version: ${LATEST#v}"
echo "Path   : ${TARGET}"
echo "Log    : ${LOG}"
