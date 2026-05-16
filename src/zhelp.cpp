#include "main.h"


int main() {


    std::cout << RED << "Zielina Package Manager (ZPM) " << RESET << "v" << zpm_version::version() << std::endl;
    std::cout << "https://github.com/Ignacyyy/ZPM\n" << std::endl;

    // commands
    std::cout << BOLD << "Commands:" << RESET << std::endl;
    std::cout << "  " << GREEN << "zhelp" << RESET << "      - Show this help message" << std::endl;
    std::cout << "  " << GREEN << "zpm" << RESET << "        - Unified wrapper (e.g., zpm install, zpm update)" << std::endl;
    std::cout << "  " << GREEN << "zupd" << RESET << "       - Update system packages" << std::endl;
    std::cout << "  " << GREEN << "zinst" << RESET << "      - Install package" << std::endl;
    std::cout << "  " << GREEN << "zrm" << RESET << "        - Remove package" << std::endl;
    std::cout << "  " << GREEN << "zlist" << RESET << "      - List installed packages" << std::endl;
    std::cout << "  " << GREEN << "zsearch" << RESET << "    - Search for package" << std::endl;
    std::cout << "  " << GREEN << "zinfo" << RESET << "      - Package information" << std::endl;
    std::cout << "  " << GREEN << "zclean" << RESET << "     - Clean package cache" << std::endl;
    std::cout << "  " << GREEN << "zr" << RESET << "         - Reboot system (sudo)" << std::endl;
    std::cout << "  " << GREEN << "zs" << RESET << "         - Shutdown system (sudo)" << std::endl;
    std::cout << "  " << GREEN << "zupgr" << RESET << "      - Update ZPM itself (sudo)" << std::endl;
    std::cout << "  " << GREEN << "zuninstall" << RESET << " - Uninstall ZPM (sudo)" << std::endl;

    // global options
    std::cout << "\n" << BOLD << "Common options:" << RESET << std::endl;
    std::cout << "  --help, -h    Show help for a specific command" << std::endl;
    std::cout << "  --version, -v Show version" << std::endl;

    // args
    std::cout << "\n" << BOLD << "Command-specific arguments:" << RESET << std::endl;
    std::cout << CYAN << "------------------------------------------------------------" << RESET << std::endl;
    std::cout << "  zupd   -full    Full system upgrade" << std::endl;
    std::cout << "  zupd   -r   Reboot after update" << std::endl;
    std::cout << "  zupd   -s   Shutdown after update" << std::endl;
    std::cout << "  zupd   --yes, -y   Automatic yes to prompts" << std::endl;
    std::cout << CYAN << "------------------------------------------------------------" << RESET << std::endl;
    std::cout << "  zrm    -p   Purge (remove config files)" << std::endl;
    std::cout << CYAN << "------------------------------------------------------------" << RESET << std::endl;
    std::cout << "  zupgr  --force, -f  Force update ZPM" << std::endl;
    std::cout << "  zupgr  --experimental, -ex  update ZPM to prerelease ver" << std::endl;
    std::cout << CYAN << "------------------------------------------------------------" << RESET << std::endl;
    std::cout << "  zlist  --apt, -apt   List only apt packages" << std::endl;
    std::cout << "  zlist  --flatpak, -f   List only flatpak packages" << std::endl;
    std::cout << "  zlist  --snap, -s   List only snap packages" << std::endl;
    std::cout << CYAN << "------------------------------------------------------------" << RESET << std::endl;
    std::cout << "  zs     --yes, -y   Automatic yes to prompts" << std::endl;
    std::cout << CYAN << "------------------------------------------------------------" << RESET << std::endl;
    std::cout << "  zr     --yes, -y   Automatic yes to prompts" << std::endl;
    std::cout << CYAN << "------------------------------------------------------------" << RESET << std::endl;

    // examples
    std::cout << "\n" << BOLD << "Examples:" << RESET << std::endl;
    std::cout << "  zinst firefox" << std::endl;
    std::cout << "  sudo zupd -full -y" << std::endl;
    std::cout << "  zrm vlc -p" << std::endl;
    std::cout << "  sudo zupgr -ex" <<std::endl;

    // zpm wrapper info
    std::cout << "\n" << RED << "New way of using ZPM:" << RESET << std::endl;
    std::cout << "  " << BOLD << "zpm" << RESET << " <command> [options]   (e.g., zpm install firefox)" << std::endl;
    std::cout << "  Run " << BOLD << "zpm --help" << RESET << " for more details." << std::endl;

    return 0;
}
