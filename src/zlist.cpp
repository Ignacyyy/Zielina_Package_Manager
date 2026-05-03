#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>

const std::string GREEN = "\033[1;32m";   // nagłówki
const std::string CYAN = "\033[1;36m";
const std::string YELLOW = "\033[1;33m";  // ASCII znak
const std::string BLUE = "\033[1;34m";
const std::string RESET = "\033[0m";
const std::string RED    = "\033[31m";

int main(int argc, char* argv[]) {

    using namespace std;
    bool           showHelp       = false;
    bool           showVersion    = false;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if      (arg == "--help"    || arg == "-h") showHelp = true;
        else if (arg == "--version" || arg == "-v") showVersion = true;
    }

    if(showVersion && showHelp){
        cout << RED << "Error: Invalid arguments, can't use -r and -s together, or can't use --help and --version with any other options!" << RESET << endl;
        return 1;
    }
    if (showVersion) {
        cout << RED << "zsearch component version: 1.0 of ZPM\n" << RESET;
        cout << "https://github.com/Ignacyyy/Zielina_Package_Manager\n";
        cout << "Copyright (c) 2026 Ignacyyy\nLicense: MIT\n";
        return 0;
    }

    if (showHelp) {
        cout << RED << "Usage: " << RESET << argv[0] << " [options]\n\n";
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