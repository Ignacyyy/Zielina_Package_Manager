#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>

int main(int argc, char* argv[]) {
    bool doFull = false;
    bool doReboot = false;
    bool doShutdown = false;
    bool doFlatpak = false;

    // Parsowanie argumentów
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-full") doFull = true;
        else if (arg == "-flatpak") doFlatpak = true;
        else if (arg == "-reboot") doReboot = true;
        else if (arg == "-shutdown") doShutdown = true;
        else {
            std::cout << "Invalid argument: " << arg << "\n";
            std::cout << "Valid: -full -flatpak -reboot -shutdown\n";
            return 1;
        }
    }

    // NORMAL update (jeśli nie full)
    if (!doFull && !doFlatpak) {
        std::cout << "Performing normal update...\n";
        sleep(2);
        std::system("sudo apt update && sudo apt upgrade -y");
        std::system("sudo apt autoremove -y");
    }

    // FULL update
    if (doFull) {
        std::cout << "Performing full update...\n";
        sleep(2);
        std::system("sudo apt update && sudo apt full-upgrade -y");
        std::system("sudo apt autoremove -y");
        std::system("sudo flatpak update -y");
    }

    // Flatpak osobno (jeśli nie było full)
    if (doFlatpak && !doFull) {
        std::cout << "Performing update + Flatpak update...\n";
        std::system("sudo flatpak update -y");
        std::system("sudo apt update && sudo apt upgrade -y");
    }

    std::cout << "Update complete.\n";

    // reboot/shutdown NA KOŃCU
    if (doReboot) {
        std::cout << "Rebooting system...\n";
        std::system("sudo systemctl reboot");
    }

    if (doShutdown) {
        std::cout << "Shutting down system...\n";
        std::system("sudo systemctl poweroff");
    }

    return 0;
}
