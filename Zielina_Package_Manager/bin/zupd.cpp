#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>

using namespace std;

int runCommand(const string& cmd) {
    int status = system(cmd.c_str());
    if (status != 0) {
        cout << "Command failed: " << cmd << "\n";
        return 1;
    }
    return 0;
}

int main(int argc, char* argv[]) {

    // root check
    if (geteuid() != 0) {
        cout << "Use sudo!\n";
        return 1;
    }

    bool doFull = false;
    bool doReboot = false;
    bool doShutdown = false;
    bool doFlatpak = false;

    // argument parsing
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];

        if (arg == "-full") doFull = true;
        else if (arg == "-f") doFlatpak = true;
        else if (arg == "-r") doReboot = true;
        else if (arg == "-s") doShutdown = true;
        else {
            cout << "Invalid argument: " << arg << "\n";
            cout << "Valid: -full -f -r -s\n";
            return 1;
        }
    }

    // conflict check
    if (doReboot && doShutdown) {
        cout << "Cannot use -r and -s together!\n";
        return 1;
    }

    cout << "Updating system...\n";

    // apt update
    if (doFull) {
        if (runCommand("apt update && apt full-upgrade -y")) return 1;
    } else {
        if (runCommand("apt update && apt upgrade -y")) return 1;
    }

    if (runCommand("apt autoremove -y")) return 1;

    // flatpak check + update
    if (doFlatpak || doFull) {
        cout << "Checking Flatpak...\n";

        if (system("which flatpak > /dev/null 2>&1") == 0) {
            if (runCommand("flatpak update -y")) return 1;
        } else {
            cout << "Flatpak not installed, skipping...\n";
        }
    }

    cout << "Update complete.\n";

    // reboot / shutdown
    if (doReboot) {
        cout << "Rebooting system...\n";
        system("reboot");
    }

    if (doShutdown) {
        cout << "Shutting down system...\n";
        system("shutdown");
    }

    return 0;
}