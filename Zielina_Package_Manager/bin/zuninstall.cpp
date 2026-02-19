#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>

int main() {
    // Sprawdzenie, czy uruchomiono jako root
    if (geteuid() != 0) {
        std::cerr << "This script must be run as root! Use sudo.\n";
        return 1;
    }

    std::string answer;
    std::system("clear");
    std::cout << "Zielina Package Manager UNINSTALLER (root mode)\n";
    std::cout << "This will remove Zielina and all backup folders.\n\n";
    sleep(2);

    std::cout << "Continue? [y/N]: ";
    std::cin >> answer;

    if (answer == "y" || answer == "Y") {
        std::cout << "\nRemoving main folder...\n";
        std::system("rm -rf /usr/local/bin/Zielina_Package_Manager");

        std::cout << "Removing all backup folders...\n";
        std::system("rm -rf /usr/local/bin/Zielina_Package_Manager_backup_*");

        std::cout << "Removing PATH script...\n";
        std::system("rm -f /etc/profile.d/zielina.sh");

        sleep(1);
        std::cout << "\nUninstall complete.\n";
        std::cout << "Logout or reboot recommended.\n";
    } else {
        std::cout << "Uninstallation cancelled.\n";
    }

    return 0;
}
