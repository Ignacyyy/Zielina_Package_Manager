#include <iostream>
#include <cstdlib>
#include <string>
#include <unistd.h>

const std::string GREEN = "\033[1;32m";   // nagłówki
const std::string CYAN = "\033[1;36m";
const std::string YELLOW = "\033[1;33m";  // ASCII znak
const std::string BLUE = "\033[1;34m";
const std::string RESET = "\033[0m";
const std::string RED    = "\033[31m";

int main(int argc, char* argv[]){

    using namespace std;
    bool           showHelp       = false;
    bool           showVersion    = false;
    bool y = false;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if      (arg == "--help"    || arg == "-h") showHelp = true;
        else if (arg == "--version" || arg == "-v") showVersion = true;
        else if (arg == "--yes" || arg == "-y") y = true;
    }

    if(showVersion && showHelp){
        cout << YELLOW << "--version\n" << RESET;
        cout << RED << "zr component version: 1.1 of ZPM\n" << RESET;
        cout << "https://github.com/Ignacyyy/Zielina_Package_Manager\n";
        cout << "Copyright (c) 2026 Ignacyyy\nLicense: MIT\n";
        cout << "\n";
        cout << YELLOW << "--help\n" << RESET;
        cout << RED << "Usage: " << RESET << argv[0] << " [options]\n\n";
        cout << RED << "Options:\n" << RESET;
        cout << "  --version, -v  Show version information\n";
        cout << "  --help,    -h  Show this help message\n";
        cout << "  --yes,   -y Automatic system reboot\n";
        return 0;
    if (y && showHelp || y && showVersion){
        cout << RED << "Error: -y are mutually exclusive. " << endl; cout << "--help and --version cannot be combined with other options." << RESET << endl;
        return 0;

    }

    }
    if (showVersion) {
        cout << RED << "zr component version: 1.1 of ZPM\n" << RESET;
        cout << "https://github.com/Ignacyyy/Zielina_Package_Manager\n";
        cout << "Copyright (c) 2026 Ignacyyy\nLicense: MIT\n";
        return 0;
    }

    if (showHelp) {
        cout << RED << "Usage: " << RESET << argv[0] << " [options]\n\n";
        cout << RED << "Options:\n" << RESET;
        cout << "  --version, -v  Show version information\n";
        cout << "  --help,    -h  Show this help message\n";
        cout << "  --yes,   -y Automatic system reboot\n";
        return 0;
    }
    if (y){
        sleep (2);
        std::system("sudo reboot");
        return 0;
    }

    std::string anwser;
    std::cout << " system Reboot, You want to continue ? [y/N]" << std::endl;
    std::cin >> anwser;
     if(anwser == "y" || anwser == "Y" || anwser == "yes" || anwser == "Yes" || anwser =="YES" || anwser =="yEs" || anwser == "yeS" || anwser == "tak" || anwser == "T" || anwser == "t")
{
        std::system("sudo reboot");
        return 0;
    }
else{
    std::cout << "Canceling system reboot..." << std::endl;
    sleep(2);
    std::cout << "System reboot canceled" << std::endl;
    return 0;
}
}
