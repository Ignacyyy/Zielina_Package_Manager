#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>

int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cout << "Usage:\n";
        std::cout << "  zinst [package]\n";
        std::cout << "  zinst -flatpak [package]\n";
        return 1;
    }

    bool useFlatpak = false;
    std::string command;

    int startIndex = 1;

    // sprawdzenie flagi
    std::string firstArg = argv[1];
    if (firstArg == "-flatpak") {
        useFlatpak = true;
        startIndex = 2;

        if (argc < 3) {
            std::cout << "Flatpak package name required.\n";
            return 1;
        }
    }

    // budowanie komendy
    if (useFlatpak)
        command = "sudo flatpak install -y ";
    else
        command = "sudo apt install -y ";

    for (int i = startIndex; i < argc; i++) {
        std::string arg = argv[i];

        if (arg[0] == '-') {
            std::cout << "Invalid argument: " << arg << std::endl;
            return 1;
        }

        command += arg + " ";
    }

    std::cout << "Installing packages...\n";
    sleep(1);

    int status = std::system(command.c_str());

    if (status != 0) {
        std::cout << "Installation failed! Check package names.\n";
        return 1;
    }

    std::cout << "Installation complete.\n";
    return 0;
}
