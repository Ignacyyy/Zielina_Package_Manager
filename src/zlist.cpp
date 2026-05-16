#include "main.h"

int main(int argc, char* argv[]) {
    zpm_update::checkForUpdates();
    using namespace std;

    bool showHelp    = false;
    bool showVersion = false;
    bool apt = false;
    bool flatpak = false;
    bool snap = false;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if      (arg == "--help"    || arg == "-h") showHelp    = true;
        else if (arg == "--version" || arg == "-v") showVersion = true;
        else if (arg == "--apt" || arg == "-apt") apt = true;
        else if (arg == "--flatpak" || arg == "-f") flatpak = true;
        else if (arg == "--snap" || arg == "-s") snap = true;
    }

    if (showVersion && showHelp) {
        cout << YELLOW << "--version\n" << RESET;
        cout << RED << "zlist component version: " << zpm_version::version() << " of ZPM\n" << RESET;
        cout << "https://github.com/Ignacyyy/ZPM\n";
        cout << "Copyright (c) 2026 Ignacyyy\nLicense: MIT\n\n";
        cout << YELLOW << "--help\n" << RESET;
        cout << RED << "Usage: " << RESET << argv[0] << " [options] or zpm list [options]\n\n";
        cout << RED << "Options:\n" << RESET;
        cout << "  --version, -v    Show version information\n";
        cout << "  --help, -h   Show this help message\n";
        cout << "  --apt, -apt  List only apt\n";
        cout << "  --flatpak. -f    List only flatpak\n";
        cout << "  --snap, -s   List only snap\n";
        return 0;
    }
    if (showVersion) {
        cout << RED << "zlist component version: " << zpm_version::version() << " of ZPM\n" << RESET;
        cout << "https://github.com/Ignacyyy/ZPM\n";
        cout << "Copyright (c) 2026 Ignacyyy\nLicense: MIT\n";
        return 0;
    }
    if (showHelp) {
        cout << RED << "Usage: " << RESET << argv[0] << " [options] or zpm list [options]\n\n";
        cout << RED << "Options:\n" << RESET;
        cout << "  --version, -v    Show version information\n";
        cout << "  --help, -h   Show this help message\n";
        cout << "  --apt, -apt  List only apt\n";
        cout << "  --flatpak. -f    List only flatpak\n";
        cout << "  --snap, -s   List only snap\n";
        return 0;
    }

 // ─── arguments ─────────────────────────────────────────────────────────────────
    if (apt){
        cout << YELLOW << "=== APT packages ===\n" << RESET;
        system("apt list --installed 2>/dev/null");
        return 0;
    }
    if (flatpak){
        bool hasFlatpak = (access("/usr/bin/flatpak", X_OK) == 0 ||
        access("/bin/flatpak",     X_OK) == 0);
        if (hasFlatpak) {
         cout << "\n" << YELLOW << "=== Flatpak packages ===\n" << RESET;
     
         FILE* p = popen("flatpak list --columns=application,name,version 2>/dev/null", "r");
         char buf[512];
         while (fgets(buf, sizeof(buf), p)) {
             std::string line(buf);
             line.erase(line.find_last_not_of(" \n\r\t") + 1);
             cout << line << RESET << "\n";
         }
         pclose(p);
     }
     return 0;
    }

    if (snap){
        bool hasSnap = (access("/usr/bin/snap", X_OK) == 0 ||
        access("/bin/snap",     X_OK) == 0 ||
        access("/snap/bin/snap",X_OK) == 0);
    if (hasSnap) {
    cout << "\n" << YELLOW << "=== Snap packages ===\n" << RESET;
    system("snap list 2>/dev/null");
        }
    if (!hasSnap){
        cout << RED << "Snap not installed\n" << RESET;
    }   
        return 0;
    }

    // ─── APT ─────────────────────────────────────────────────────────────────
    cout << YELLOW << "=== APT packages ===\n" << RESET;
    system("apt list --installed 2>/dev/null");

    // ─── FLATPAK ─────────────────────────────────────────────────────────────
    bool hasFlatpak = (access("/usr/bin/flatpak", X_OK) == 0 ||
                       access("/bin/flatpak",     X_OK) == 0);
                       if (hasFlatpak) {
                        cout << "\n" << YELLOW << "=== Flatpak packages ===\n" << RESET;
                    
                        FILE* p = popen("flatpak list --columns=application,name,version 2>/dev/null", "r");
                        char buf[512];
                        while (fgets(buf, sizeof(buf), p)) {
                            std::string line(buf);
                            line.erase(line.find_last_not_of(" \n\r\t") + 1);
                            cout << line << RESET << "\n";
                        }
                        pclose(p);
                    }
    // ─── SNAP ────────────────────────────────────────────────────────────────
    bool hasSnap = (access("/usr/bin/snap", X_OK) == 0 ||
                    access("/bin/snap",     X_OK) == 0 ||
                    access("/snap/bin/snap",X_OK) == 0);
    if (hasSnap) {
        cout << "\n" << YELLOW << "=== Snap packages ===\n" << RESET;
        system("snap list 2>/dev/null");
    }

    return 0;
}