#!/bin/bash
set -e

cd /opt/ZPM/src

echo " Building ZPM..."

for file in *.cpp; do
    [ -e "$file" ] || continue

    name="${file%.cpp}"

    # ───────── special case ─────────
    if [[ "$name" == "ZPM" ]]; then
        out_name="zpm"
    else
        out_name="$name"
    fi

    out="/tmp/$out_name"

    echo "⚙ compiling $file -> $out_name"

    g++ "$file" -O2 -I /opt/ZPM/src/common -o "$out"

    sudo mv -f "$out" /opt/ZPM/bin/
    sudo ln -sf /opt/ZPM/bin/$out_name /usr/bin/$out_name

    echo " installed $out_name"
done

cd ~

echo " DONE"
