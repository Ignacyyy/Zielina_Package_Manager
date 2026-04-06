#!/bin/bash

# Root check
if [ "$EUID" -ne 0 ]; then
    echo "Use sudo!"
    exit 1
fi

echo "Zielina Package Manager installer / Updater, do you want to continue? [Y,n]"
read -r answer

if [[ "$answer" =~ ^[Yy]$ ]]; then
    echo "Starting installation..."
    sleep 1

    # Current script folder (source)
    INSTALL_FOLDER="$(cd "$(dirname "$0")" && pwd)"
    echo "Source Folder: $INSTALL_FOLDER"

    # Target locations
    TARGET_FOLDER="/opt/Zielina_Package_Manager"
    PROGRAMS_PATH="$TARGET_FOLDER/programs"
    BIN_PATH="$TARGET_FOLDER/bin"
    PROFILE_SCRIPT="/etc/profile.d/Zielina_Package_Manager.sh"

    # Move folder
    echo "Moving Zielina_Package_Manager to $TARGET_FOLDER..."
    rm -rf "$TARGET_FOLDER"          # usuń jeśli istnieje
    mv "$INSTALL_FOLDER" "$TARGET_FOLDER"
    sleep 1

    # Set executable permissions
    echo "Setting executable permissions..."
    chmod -R +x "$PROGRAMS_PATH"
    chmod -R +x "$BIN_PATH"
    sleep 1

    # Add programs folder to PATH globally for all users including sudo
    echo "Adding programs folder to global PATH..."
    echo "export PATH=\"\$PATH:$PROGRAMS_PATH:$BIN_PATH\"" > "$PROFILE_SCRIPT"
    chmod +x "$PROFILE_SCRIPT"
    sleep 1

    echo "Linking commands..."
    ln -sf "$PROGRAMS_PATH"/* /usr/bin/
    rm -rf /ztmp
    echo ""
    echo "Installation / Update finished :)"
    echo "Please reboot or log out and log back in for changes to take effect."
    echo ""
    exit 0
else
    echo "Installation / Update cancelled."
    exit 0
fi
