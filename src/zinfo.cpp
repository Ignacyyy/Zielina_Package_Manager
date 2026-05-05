#include <iostream>
#include <cstdio>
#include <string>
#include <algorithm>

// kolory
const std::string GREEN  = "\033[1;32m";
const std::string CYAN   = "\033[1;36m";
const std::string YELLOW = "\033[1;33m";
const std::string BLUE   = "\033[1;34m";
const std::string RESET  = "\033[0m";
const std::string RED    = "\033[31m";

// funkcja do obsługi jednego pakietu
void showPackageInfo(const std::string& pkg) {
    std::string command = "apt show " + pkg + " 2>/dev/null";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Error running apt for " << pkg << "\n";
        return;
    }

    char buffer[512];
    std::string line;

    std::string name, version, priority, section;
    std::string depends, recommends, homepage, desc;
    bool inDescription = false;

    while (fgets(buffer, sizeof(buffer), pipe)) {
        line = buffer;
        if      (line.find("Package:")     == 0) name      = line.substr(9);
        else if (line.find("Version:")     == 0) version   = line.substr(9);
        else if (line.find("Priority:")    == 0) priority  = line.substr(10);
        else if (line.find("Section:")     == 0) section   = line.substr(9);
        else if (line.find("Depends:")     == 0) depends   = line.substr(9);
        else if (line.find("Recommends:")  == 0) recommends= line.substr(12);
        else if (line.find("Homepage:")    == 0) homepage  = line.substr(10);
        else if (line.find("Description:") == 0) {
            desc = line.substr(13);
            inDescription = true;
        }
        else if (inDescription) {
            desc += line;
        }
    }
    pclose(pipe);

    // sprawdzenie instalacji
    std::string check = "dpkg -s " + pkg + " 2>/dev/null";
    FILE* p = popen(check.c_str(), "r");
    bool installed = false;
    while (fgets(buffer, sizeof(buffer), p)) {
        std::string s(buffer);
        if (s.find("Status: install ok installed") != std::string::npos) {
            installed = true;
            break;
        }
    }
    pclose(p);

    // OUTPUT
    std::cout << YELLOW << "[P] " << GREEN << name << RESET;
    std::cout << CYAN   << " (" << version << ")" << RESET;
    if (installed) std::cout << BLUE << " [✓ Installed]"  << RESET;
    else           std::cout << BLUE << " [ ] Not installed" << RESET;
    std::cout << "\n\n";

    std::cout << YELLOW << "[I] " << GREEN << "Basic info\n" << RESET;
    std::cout << "  Priority: " << priority;
    std::cout << "  Section : " << section << "\n\n";

    std::cout << YELLOW << "[D] " << GREEN << "Dependencies\n" << RESET;
    std::cout << "  Depends    : " << depends;
    std::cout << "  Recommends : " << recommends << "\n\n";

    std::cout << YELLOW << "[H] " << GREEN << "Homepage\n" << RESET;
    std::cout << "  " << homepage << "\n\n";

    std::cout << YELLOW << "[d] " << GREEN << "Description\n" << RESET;
    std::cout << desc << "\n";

    std::cout << "----------------------------------------\n\n";
}

int main(int argc, char* argv[]) {
    using namespace std;
    bool showHelp    = false;
    bool showVersion = false;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if      (arg == "--help"    || arg == "-h") showHelp    = true;
        else if (arg == "--version" || arg == "-v") showVersion = true;
        else showPackageInfo(argv[i]);
    }

    if (showVersion && showHelp) {
        cout << YELLOW << "--version\n" << RESET;
        cout << RED << "zinfo component version: 1.1 of ZPM\n" << RESET;
        cout << "https://github.com/Ignacyyy/Zielina_Package_Manager\n";
        cout << "Copyright (c) 2026 Ignacyyy\nLicense: MIT\n\n";
        cout << YELLOW << "--help\n" << RESET;
        cout << RED << "Usage: " << RESET << argv[0] << " [packages...] [options]\n\n";
        cout << RED << "Options:\n" << RESET;
        cout << "  --version, -v  Show version information\n";
        cout << "  --help,    -h  Show this help message\n";
        return 0;
    }

    if (showVersion) {
        cout << RED << "zinfo component version: 1.1 of ZPM\n" << RESET;
        cout << "https://github.com/Ignacyyy/Zielina_Package_Manager\n";
        cout << "Copyright (c) 2026 Ignacyyy\nLicense: MIT\n";
        return 0;
    }

    if (showHelp) {
        cout << RED << "Usage: " << RESET << argv[0] << " [packages...] [options]\n\n";
        cout << RED << "Options:\n" << RESET;
        cout << "  --version, -v  Show version information\n";
        cout << "  --help,    -h  Show this help message\n";
        return 0;
    }

    return 0;
}