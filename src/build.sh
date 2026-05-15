#!/bin/bash

SRC_DIR="$(pwd)"
DEST_DIR="/opt/ZPM"
INCLUDE_DIR="/opt/ZPM/src/common"
# create destination directory if it does not exist
if [ ! -d "$DEST_DIR" ]; then
    sudo mkdir -p "$DEST_DIR"
fi

# compile all .cpp files
for file in "$SRC_DIR"/*.cpp; do
    [ -e "$file" ] || continue

    filename=$(basename -- "$file")
    name="${filename%.cpp}"

    echo "Compiling $filename -> $name"

    g++ -O2 -I "$INCLUDE_DIR" "$file" -o "$name"

    if [ $? -eq 0 ]; then

        # move binary to /opt/ZPM
        sudo mv -f "$name" "$DEST_DIR/"

        # create symlink in /usr/bin
        sudo ln -sf "$DEST_DIR/$name" "/usr/bin/$name"

        echo "Installed: /usr/bin/$name"

    else
        echo "Compilation failed: $filename"
    fi
done

echo "Done."
