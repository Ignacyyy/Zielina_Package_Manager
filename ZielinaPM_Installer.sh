#!/bin/bash

# Zielina PM Installer – Bash version with symlinks and correct location
# Jeden krok: wget -qO- <link> | bash

# Kolory
GREEN='\033[0;32m'
NC='\033[0m' # no color

# Banner ASCII
echo -e "${GREEN}ZielinaPM_Installer${NC}"
cat <<EOF
${GREEN}                              ↑
                               ↑↑
                               ↖↑↙←                       ↑
                               ↑↖↙↖↘↖                     ↑
                               ↗↙↑↑↙↑↑←                 ↖↑←
                               →↖↖↓↑↖↑↓↑              →↑↑↑
                               ↙↑↖↖↑↖←↙↓↑           ↑↑↖ ↖←
                               ↑↑↖↖↗→↖↑↖↑→         ↙↖↑↑→↖↑
                                ↙↙↑↙↗↘↖↓↖↑       ↑↙←↑↓↖↘↑   ↑
                                ↑↘↖↖←↑↗↓↖↑      →↑↖↑←→↖↖↑  ←↑
                                 ↑←↑←↖↖←↓↘↖    ←↑ ↑↖↗↖↗↙↑ ↙←←
                                   ↑↗→↙↑↑↓    ↓↖↖↓↖↑→↖→→ ↖↖↖↖↙
                                     ↑↙←↘→    ↙↗↖↑↑↓↑↖↖↑ ↓↖↑↘←
               ↑↑↑↓↑↖↖↖↖↑↙↙↑↙→↙↘↖     →→↑↓←  ↑←↗↖→↖↖↖↑↑ ↖↑↖↗↖→
                 ↗←↖↖↑↑↘→↘↗↖↖↗↖↖↑↙↖↘    ↑↙↗  ↗↖↙↖↙←↑↓↑ ↓↖←→←↙←
                   ↙↑↙↖↖↖↙↑↑→↘↖→↖↑↖↖↑↑↖   ↑  ↙↘↖↑↖↙↑  ↖→↑↖↑←←↑
                      ↑↑↖↖↖↓←↑↗↗↑↖↑↓↖↙↖↘←    ↙↖↑↖→↑  ↓↙↑↖↖↑↖↖↓
                        ↑↑→←↖↙↖↖↑↓→↓↑↑↑←↙↙   ↖↑→↑    ↑↓↖↑↖↖↘↖→
                            →↙→↙↙↖→↖↖↖↖↓→↑↑ ↑↖↖     ↓↖↑↖→↑↖←↓↘
                                  ↑↘→↑↓→↑    ↑      →↖↖→←↖↘←↑
                                              ↑    ←→↗↖←↘↖↘←↓
                                               ↑   ←↖↖←↗↖↖↗↘
                                                ↘  ←→←↖↓↗↙→
                          ↓↘↑↑→↑↑↙↑↑←↓↑↙↖↓←↓     ↖ ↙↙↖↑↖↓→
                     ↓↖↓↑↓←↖↘↖↖↓↖↖↖↖↖↑↖↗↑↗↑↓↙↑←←    ↖↓↑←↑
                  ↑↘→↑↑↙↑↙↗↑↑↘ ↑↑→↘→↗↖↓↖↖↖↖↖←↖↙↖↖↖ ↑ ↖↗
                        ↙↙↓↑←↙↓↑↖↓↘↖→↘↓↑↑↑↑↗↑↑↑↖↑←→ ↑↖
                            →↑↖↓↙←↑←←→↓↖↙↗↖←↗↖←↖↙↑   ↑
                                  →→↙↓↘↓←↙↘↗↙          ↖
                                                        ↑
                                                           ↑
                                                            ↑
                                                              ↑
                                                                ↑↖
                                                                   ↑
                                                                     ↑
                                                                        ↑
                                                                           ↑↖${NC}
EOF

sleep 2
echo
read -p "Do you want to install Zielina PM? [y/N]: " answer
if [[ "$answer" =~ ^[Yy]$ || -z "$answer" ]]; then
    echo -e "${GREEN}Downloading Zielina PM...${NC}"

    if [ -d "/usr/local/bin/Zielina_Package_Manager" ]; then
    echo -e "${GREEN}Old installation found. Removing...${NC}"
    sudo rm -rf /usr/local/bin/Zielina_Package_Manager
fi

if [ -f "/etc/profile.d/zielina.sh" ]; then
    sudo rm -f /etc/profile.d/zielina.sh
fi


    sleep 1


    INSTALL_DIR="/usr/local/bin/Zielina_Package_Manager"
    sudo mkdir -p "$INSTALL_DIR"

    #  ZIP, GitHub
    sudo curl -L -o /tmp/Zielina_Package_Manager.zip https://github.com/Ignacyyy/Zielina_Package_Manager/archive/refs/heads/main.zip


    sudo unzip -o /tmp/Zielina_Package_Manager.zip -d /tmp


    sudo mv /tmp/Zielina_Package_Manager-main/bin/* "$INSTALL_DIR/"

    # chmod
    sudo chmod +x "$INSTALL_DIR/"*

    #  /usr/bin,
    for cmd in "$INSTALL_DIR"/*; do
    sudo ln -sf "$cmd" "/usr/bin/$(basename $cmd)"
done


    #  PATH
    PROFILE_SCRIPT="/etc/profile.d/zielina.sh"
    sudo bash -c "echo 'export PATH=\$PATH:$INSTALL_DIR' > $PROFILE_SCRIPT"
    sudo chmod +x "$PROFILE_SCRIPT"

    sleep 2
    echo -e "${GREEN}Installation Finished!${NC}"
    sleep 1
    echo "You can now use Zielina PM commands like: zhelp, zrm, zupd, etc."
    echo "Reboot or logout"
else
    echo "Installation Canceled."
fi
