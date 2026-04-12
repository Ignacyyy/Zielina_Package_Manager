#!/bin/bash

# ============================================
# Zielina Package Manager - Installer/Updater
# ============================================
# This script installs or updates Zielina Package Manager
# using GitHub Releases for version checking

# Root check
if [ "$EUID" -ne 0 ]; then
    echo "Use sudo!"
    exit 1
fi

# User confirmation
echo "Zielina Package Manager installer / updater, do you want to continue? [Y,n]"
read -r answer

# ============================================
# Version checking using GitHub Releases API
# ============================================

# GitHub repository
REPO_OWNER="Ignacyyy"
REPO_NAME="Zielina_Package_Manager"

# Fetch latest version from GitHub Releases
get_latest_release_version() {
    local api_url="https://api.github.com/repos/${REPO_OWNER}/${REPO_NAME}/releases/latest"
    local version=$(curl -s "$api_url" | grep '"tag_name"' | head -1 | sed 's/.*"\([^"]*\)".*/\1/')
    # Remove leading 'v' if present (v3.9 -> 3.9)
    version=$(echo "$version" | sed 's/^v//')
    echo "$version"
}

# Get remote version
REMOTE_VERSION=$(get_latest_release_version)

# Check if we got a valid version (fallback to branch if API fails)
if [[ -z "$REMOTE_VERSION" || "$REMOTE_VERSION" == "null" ]]; then
    echo "Warning: Could not fetch latest release, falling back to branch..."
    BRANCH="main,-APT(debian,ubuntu.)"
    REMOTE_VERSION=$(curl -s "https://raw.githubusercontent.com/${REPO_OWNER}/${REPO_NAME}/${BRANCH}/VERSION.txt" | grep -oP '\d+(\.\d+)*')
fi

# Get installed version
INSTALLED_VERSION=""
if [[ -f "/opt/ZPM/VERSION.txt" ]]; then
    INSTALLED_VERSION=$(grep -oP '\d+(\.\d+)*' /opt/ZPM/VERSION.txt)
fi

# Display version information
echo "=========================================="
echo "Installed version: ${INSTALLED_VERSION:-none}"
echo "Latest version:    ${REMOTE_VERSION:-unknown}"
echo "=========================================="

# Check if update is needed
if [[ -n "$INSTALLED_VERSION" && "$INSTALLED_VERSION" == "$REMOTE_VERSION" ]]; then
    echo "You already have the latest version ($INSTALLED_VERSION)."
    exit 0
else
    echo "Installing / updating from ${INSTALLED_VERSION:-none} to ${REMOTE_VERSION}..."
fi

# ============================================
# Installation process
# ============================================

if [[ "$answer" =~ ^[Yy]$ ]]; then
    echo "Starting installation..."
    sleep 1

    # Current script folder (source)
    INSTALL_FOLDER="$(cd "$(dirname "$0")" && pwd)"
    echo "Source Folder: $INSTALL_FOLDER"

    # Target locations
    TARGET_FOLDER="/opt/ZPM"
    PROGRAMS_PATH="$TARGET_FOLDER/bin"
    SRC_PATH="$TARGET_FOLDER/src"

    # Clean up old files before installation
    echo "Cleaning up old files..."
    rm -f /usr/bin/z*
    rm -f ~/bin/z* 2>/dev/null

    # Move folder to target location
    echo "Moving Zielina Package Manager to $TARGET_FOLDER..."
    rm -rf "$TARGET_FOLDER"
    mv "$INSTALL_FOLDER" "$TARGET_FOLDER"
    sleep 1

    # Set executable permissions
    echo "Setting executable permissions..."
    chmod -R +x "$PROGRAMS_PATH"
    sleep 1

    # Create symbolic links in /usr/bin
    echo "Creating symbolic links in /usr/bin..."
    ln -sf "$PROGRAMS_PATH"/* /usr/bin/

    echo ""
    echo "Installation / update finished successfully!"
    echo "Installed version: $REMOTE_VERSION"
    echo ""
    echo "Quick start: Type 'zhelp' to see available commands."
    echo ""

    exit 0
else
    echo "Installation / update cancelled."
    exit 0
fi
