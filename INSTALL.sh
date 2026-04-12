
#!/bin/bash

# Root check
if [ "$EUID" -ne 0 ]; then
    echo "Use sudo!"
    exit 1
fi

echo "Zielina Package Manager installer / updater, do you want to continue? [Y,n]"
read -r answer

if [[ "$answer" =~ ^[Yy]$ ]]; then
    echo "Starting installation..."
    sleep 1
    
    # Get latest release tag
    LATEST=$(curl -s https://api.github.com/repos/Ignacyyy/Zielina_Package_Manager/releases/latest | grep '"tag_name"' | sed 's/.*"\(v[0-9.]*\)".*/\1/')
    
    echo "Latest release: $LATEST"
    
    # Download and extract
    wget --no-cache "https://github.com/Ignacyyy/Zielina_Package_Manager/archive/refs/tags/${LATEST}.tar.gz"
    tar -xzf ${LATEST}.tar.gz
    cd Zielina_Package_Manager-*
    
    # Target locations
    TARGET_FOLDER="/opt/ZPM"
    PROGRAMS_PATH="$TARGET_FOLDER/bin"
    
    # Clean and install
    rm -f /usr/bin/z*
    rm -f ~/bin/z* 2>/dev/null
    rm -rf "$TARGET_FOLDER"
    
    # Copy files
    mkdir -p "$TARGET_FOLDER"
    cp -r . "$TARGET_FOLDER/"
    
    # Set permissions
    chmod -R +x "$PROGRAMS_PATH"
    
    # Create symlinks
    ln -sf "$PROGRAMS_PATH"/* /usr/bin/
    
    # Save version
    echo "${LATEST#v}" > "$TARGET_FOLDER/VERSION.txt"
    
    echo ""
    echo "Installation finished successfully!"
    echo "Installed version: ${LATEST#v}"
    echo ""
    echo "Quick start: Type 'zhelp' to see available commands."
    echo ""
    
    exit 0
else
    echo "Installation cancelled."
    exit 0
fi
