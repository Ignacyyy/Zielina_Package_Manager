#include "main.h"

int main(int argc, char* argv[]) {
    zpm_update::checkForUpdates();
    using namespace std;
    bool           showHelp       = false;
    bool           showVersion    = false;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if      (arg == "--help"    || arg == "-h") showHelp = true;
        else if (arg == "--version" || arg == "-v") showVersion = true;
    }

    if(showVersion && showHelp){
        cout << YELLOW << "--version\n" << RESET;
        cout << RED << "zlist component version: 1.1 of ZPM\n" << RESET;
        cout << "https://github.com/Ignacyyy/ZPM\n";
        cout << "Copyright (c) 2026 Ignacyyy\nLicense: MIT\n\n";
        cout << YELLOW << "--help\n" << RESET;
        cout << RED << "Usage: " << RESET << argv[0] << " [options]" << " or zpm list [options]" "\n\n";
        cout << RED << "Options:\n" << RESET;
        cout << "  --version, -v  Show version information\n";
        cout << "  --help,    -h  Show this help message\n";
        return 0;
    }
    if (showVersion) {
        cout << RED << "zsearch component version: 1.1 of ZPM\n" << RESET;
        cout << "https://github.com/Ignacyyy/ZPM\n";
        cout << "Copyright (c) 2026 Ignacyyy\nLicense: MIT\n";
        return 0;
    }

    if (showHelp) {
        cout << RED << "Usage: " << RESET << argv[0] << " [options]" << " or zpm list [options]" "\n\n";
        cout << RED << "Options:\n" << RESET;
        cout << "  --version, -v  Show version information\n";
        cout << "  --help,    -h  Show this help message\n";
        return 0;
    }






    std::cout << "Listing all packages...\n";
    sleep(1);
    int status = std::system("apt list --installed");
    
    return 0;
}   
