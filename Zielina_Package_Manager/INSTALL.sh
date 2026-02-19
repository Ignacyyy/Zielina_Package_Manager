#!/bin/bash

# root
if [ "$EUID" -ne 0 ]; then
    echo "Use sudo!"
    exit 1
fi

echo "Starting Instalation / Update of Zielina_Package_Manager..."
sleep 2

# Folder  INSTALL.sh
INSTALL_FOLDER="$(cd "$(dirname "$0")" && pwd)"

echo "Source Folder: $INSTALL_FOLDER"

# Localisation
TARGET_FOLDER="/usr/local/bin/Zielina_Package_Manager"
PROGRAMS_PATH="$TARGET_FOLDER/programs"
PROFILE_SCRIPT="/etc/profile.d/zielina.sh"

# Backup 
if [ -d "$TARGET_FOLDER" ]; then
    sudo rm -rf "$TARGET_FOLDER"
fi
# Moving Folder
echo "Moving Zielina to $TARGET_FOLDER..."
sudo mv "$INSTALL_FOLDER" "$TARGET_FOLDER"

# chmod +x
echo "chmod +x..."
sudo chmod -R +x "$PROGRAMS_PATH"
sudo chmod -R +x "$TARGET_FOLDER/bin"

sleep 1

# PATH
echo "Add to PATH..."
echo "Export PATH=\"\$PATH:$PROGRAMS_PATH\"" | sudo tee "$PROFILE_SCRIPT" > /dev/null
sudo chmod +x "$PROFILE_SCRIPT"

sleep 1

echo "Linking commands to /usr/local/bin..."
sudo ln -sf "$PROGRAMS_PATH"/* /usr/local/bin/
sleep 2

# done
echo "Zielina Finished installing / updating :)"
echo "Reboot or logout"

exit 0
