Zielina Package Manager

                               ↑
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
                                                                           ↑↖

HOW TO USE
------------
To access the main guide, type:
    zhelp
after successful installation.

HOW TO INSTALL
---------------

Method 1 (Recommended - one line):

   ```bash
sudo apt update && sudo apt install git curl wget -y && LATEST=$(curl -s https://api.github.com/repos/Ignacyyy/Zielina_Package_Manager/releases/latest | grep '"tag_name"' | head -1 | cut -d '"' -f4) && VERSION=${LATEST#v} && wget --no-cache "https://github.com/Ignacyyy/Zielina_Package_Manager/archive/refs/tags/${LATEST}.tar.gz" && tar -xzf ${LATEST}.tar.gz && cd "Zielina_Package_Manager-${VERSION}" && sudo chmod +x INSTALL.sh && sudo bash ./INSTALL.sh && cd ~
```

Method 2 (Manual):

    1. Open terminal.
    2. Download latest release:
    4. go to ZMP folder eg. ~/ZPM
    3. Run installer
       sudo chmod +x INSTALL.sh
       sudo ./INSTALL.sh
    4. Logout and back in (or reboot).

HOW TO UPDATE
---------------
    sudo zupgr

HOW TO UNINSTALL
---------------
    sudo zuninstall

COMMANDS
---------------
    zhelp       - Display help message
    zinst       - Install package
    zrm         - Remove package
    zupgr       - Update ZMP
    zupd        - Update system packages
    zlist       - List installed packages
    zsearch     - Search for package
    zclean      - Clean package cache
    zr          - Reboot system
    zs          - Shutdown system
    zuninstall  - Uninstall ZMP
    zinfo       - Package information
    
HOW IT WORKS
-------------------------------

ZMP is Package manager, that allows you to use apt, flatpacks and snaps together,
e.g.
sudo zisnt mc 
Package: mc
  1. APT:     exist (mc)
  2. Flatpak: exist (29 results)
  0. Skip
Choose: 1

Auto mode: APT/Flatpak/Snap per package
Installing packages...

Install Progress: [########################################] 100% | Done!

Package mc is already installed.
Installation complete!
or
sudo zupd
[SYS] Debian GNU/Linux 13 (trixie)

[D] APT Repositories:
- deb http://deb.debian.org/debian/ trixie main contrib non-free non-free-firmware
- deb http://security.debian.org/debian-security trixie-security main contrib non-free non-free-firmware
- deb http://deb.debian.org/debian/ trixie-updates main contrib non-free non-free-firmware
- deb http://deb.debian.org/debian/ trixie-backports main contrib non-free non-free-firmware
- deb [arch=amd64,arm64,armhf signed-by=/usr/share/keyrings/microsoft-prod.gpg] https://packages.microsoft.com/debian/13/prod trixie main
- deb [signed-by=/usr/share/keyrings/waydroid.gpg] https://repo.waydro.id/ trixie main

[F] Flatpak Remotes:
- flathub

Packages to update (APT):
[+] zpm-core
[+] zpm-ui
[+] apt-wrapper
[+] zpm-notifier
[+] zpm-plugins

Packages to update (Flatpak):
[+] org.mozilla.firefox
[+] org.videolan.VLC
[+] com.visualstudio.code

Proceed with update? [y/n]: y
Update Progress: [########################################] 100% | Done!

 finished successfully!

[Report]
- refresh: OK (code 0)
- apt: OK (code 0)
- flatpak: OK (code 0)


INFO
---------------
    Works on: Debian/Ubuntu (APT) distros ONLY
    Installation path: /opt/ZPM
    Commands location: /usr/bin/z* (symbolic links)
    Current version: 1.1

DEPENDENCIES
---------------
    git
    curl
    wget
    sudo
