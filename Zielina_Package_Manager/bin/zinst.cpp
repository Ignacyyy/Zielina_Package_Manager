#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>

using namespace std;

int main(int argc, char* argv[]) {

    // Root
    if (geteuid() != 0) {
        cout << "Run with sudo!\n";
        return 1;
    }

    // Usage
    if (argc < 2) {
        cout << "Usage:\n";
        cout << "  zinst [package]\n";
        cout << "  zinst -f [package]\n";
        return 1;
    }

    bool useFlatpak = false;
    string command;

    // Check flags
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];

        if (arg == "-f") {
            useFlatpak = true;
        }
    }

    // Check if any package exists
    bool hasPackage = false;
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg != "-f") {
            hasPackage = true;
            break;
        }
    }

    if (!hasPackage) {
        cout << "No package specified!\n";
        return 1;
    }

    // Select install method
    if (useFlatpak)
        command = "flatpak install -y ";
    else
        command = "apt install -y ";

    // Add packages
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];

        if (arg == "-f") continue;

        // Invalid flag
        if (!arg.empty() && arg[0] == '-') {
            cout << "Invalid argument: " << arg << endl;
            return 1;
        }

        // Security check
        if (arg.find(';') != string::npos || arg.find('&') != string::npos) {
            cout << "Invalid characters in package name!\n";
            return 1;
        }

        command += arg + " ";
    }

    // Info
    cout << (useFlatpak ? "Using Flatpak\n" : "Using APT\n");
    cout << "Installing packages...\n";
    sleep(1);

    // Execute
    int status = system(command.c_str());

    if (status != 0) {
        cout << "Installation failed!\n";
        return 1;
    }

    cout << "Installation complete.\n";
    return 0;
}



