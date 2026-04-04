#!/bin/bash

# root check
if [ "$EUID" -ne 0 ]; then
    echo "Use sudo!"
    exit 1
fi

echo "Zielina Package Manager installer, do you want to continue? [Y,n]"
read -r answer

if [[ "$answer" =~ ^[Yy]$ ]]; then
    echo "Starting installation..."
    sleep 2

    # script folder
    INSTALL_FOLDER="$(cd "$(dirname "$0")" && pwd)"
    echo "Source Folder: $INSTALL_FOLDER"

    # localisation
    TARGET_FOLDER="/opt/Zielina_Package_Manager"
    PROGRAMS_PATH="$TARGET_FOLDER/programs"
    PROFILE_SCRIPT="/etc/profile.d/Zielina_Package_Manager.sh"

    # Moving folder
    echo "Moving Zielina_Package_Manager to $TARGET_FOLDER"
    mv "$INSTALL_FOLDER" "$TARGET_FOLDER"
    sleep 1

    # chmod +x
    echo "Setting executable permissions..."
    chmod -R +x "$PROGRAMS_PATH"
    chmod -R +x "$TARGET_FOLDER/bin"
    sleep 1

    # add to PATH globally
    echo "Adding programs folder to PATH..."
    echo "export PATH=\"\$PATH:$PROGRAMS_PATH\"" > "$PROFILE_SCRIPT"
    chmod +x "$PROFILE_SCRIPT"
    sleep 1

    echo "Zielina finished installing :)"
    echo " reboot or logout"
    exit 0
else
    echo "Installation cancelled."
    exit 0
fi
