#!/bin/bash

# ============================================
# Zielina Package Manager - Installer/Updater
# ============================================
# This script installs or updates Zielina Package Manager
# using GitHub Releases for version checking

# Root check - script must run with sudo
if [ "$EUID" -ne 0 ]; then
    echo "Error: This script requires root privileges."
    echo "Please run with: sudo ./INSTALL.sh"
    exit 1
fi

# User confirmation
echo "Zielina Package Manager installer / updater"
echo "Do you want to continue? [Y/n]"
read -r answer

# ============================================
# Version checking using GitHub Releases API
# ============================================

# GitHub repository information
REPO_OWNER="Ignacyyy"
REPO_NAME="Zielina_Package_Manager"

# Fetch latest version from GitHub Releases
# Using GitHub API - returns tag_name like "v3.9" or "TEST"
get_latest_release_version() {
    local api_url="https://api.github.com/repos/${REPO_OWNER}/${REPO_NAME}/releases/latest"
    local version=$(curl -s "$api_url" | grep '"tag_name"' | head -1 | sed 's/.*"\([^"]*\)".*/\1/')
    
    # Remove leading 'v' if present (v3.9 -> 3.9)
    version=$(echo "$version" | sed 's/^v//')
    
    echo "$version"
}

# Get remote version
REMOTE_VERSION=$(get_latest_release_version)

# Check if we got a valid version
if [[ -z "$REMOTE_VERSION" || "$REMOTE_VERSION" == "null" ]]; then
    echo "Warning: Could not fetch latest version from GitHub Releases."
    echo "Falling back to branch-based version check..."
    
    # Fallback to branch-based version
    BRANCH="main,-APT(debian,ubuntu.)"
    REMOTE_VERSION=$(curl -s "https://raw.githubusercontent.com/${REPO_OWNER}/${REPO_NAME}/${BRANCH}/VERSION.txt" | grep -oP '\d+(\.\d+)*')
fi

# Get installed version
INSTALLED_VERSION=""
if [[ -f "/opt/Zielina_Package_Manager/VERSION.txt" ]]; then
    INSTALLED_VERSION=$(grep -oP '\d+(\.\d+)*' /opt/Zielina_Package_Manager/VERSION.txt)
fi

# Display version information
echo "=========================================="
echo "Installed version: ${INSTALLED_VERSION:-none}"
echo "Latest release:    ${REMOTE_VERSION:-unknown}"
echo "=========================================="

# Check if update is needed
if [[ -n "$INSTALLED_VERSION" && "$INSTALLED_VERSION" == "$REMOTE_VERSION" ]]; then
    echo "You already have the latest version ($INSTALLED_VERSION)."
    echo "No update needed."
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
    echo "Source folder: $INSTALL_FOLDER"

    # Target locations
    TARGET_FOLDER="/opt/Zielina_Package_Manager"
    PROGRAMS_PATH="$TARGET_FOLDER/programs"
    BIN_PATH="$TARGET_FOLDER/bin"
    PROFILE_SCRIPT="/etc/profile.d/Zielina_Package_Manager.sh"

    # Move folder to target location
    echo "Moving Zielina_Package_Manager to $TARGET_FOLDER..."
    rm -rf "$TARGET_FOLDER"
    mv "$INSTALL_FOLDER" "$TARGET_FOLDER"
    sleep 1

    # Set executable permissions
    echo "Setting executable permissions..."
    chmod -R +x "$PROGRAMS_PATH"
    chmod -R +x "$BIN_PATH"
    sleep 1

    # Add programs folder to PATH globally
    echo "Adding programs folder to global PATH..."
    echo "export PATH=\"\$PATH:$PROGRAMS_PATH:$BIN_PATH\"" > "$PROFILE_SCRIPT"
    chmod +x "$PROFILE_SCRIPT"
    sleep 1

    # Create symbolic links in /usr/bin
    echo "Creating symbolic links in /usr/bin..."
    rm -f /usr/bin/z*
    ln -sf "$PROGRAMS_PATH"/* /usr/bin/ 2>/dev/null || true
    
    # Clean up temporary files
    rm -rf /ztmp 2>/dev/null || true
    
    echo ""
    echo "=========================================="
    echo "Installation / update finished successfully!"
    echo "=========================================="
    echo ""
    echo "Installed version: $REMOTE_VERSION"
    echo ""
    echo "Please reboot or log out and log back in for changes to take effect."
    echo ""
    echo "Quick start: Type 'zhelp' to see available commands."
    echo ""
    
    exit 0
else
    echo "Installation / update cancelled."
    exit 0
fi
