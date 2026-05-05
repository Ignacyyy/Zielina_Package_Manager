#include <iostream>
#include <cstdio>
#include <string>
#include <algorithm>
#include <unistd.h>

// colors
const std::string GREEN = "\033[1;32m";
const std::string YELLOW = "\033[1;33m";
const std::string RED = "\033[1;31m";
const std::string RESET = "\033[0m";

// lowercase
std::string toLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

// highlight
std::string highlight(const std::string& text, const std::string& query) {
    std::string lowerText = toLower(text);
    std::string lowerQuery = toLower(query);

    size_t pos = lowerText.find(lowerQuery);
    if (pos == std::string::npos) return text;

    return text.substr(0, pos) +
    YELLOW + text.substr(pos, query.length()) + RESET +
    text.substr(pos + query.length());
}

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
        cout << YELLOW << "--version\n" << RESET;
        cout << RED << "zsearch component version: 1.2 of ZPM\n" << RESET;
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
        cout << RED << "zsearch component version: 1.2 of ZPM\n" << RESET;
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

    std::string query;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg.find(';') != std::string::npos ||
            arg.find('&') != std::string::npos ||
            arg.find('|') != std::string::npos) {
            std::cout << RED << "Invalid characters!\n" << RESET;
        return 1;
            }

            query += arg + " ";
    }

    std::string command = "apt-cache search " + query;

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << RED << "Error running apt\n" << RESET;
        return 1;
    }

    std::cout << GREEN << " Searching: " << RESET << query << "\n\n";

    char buffer[256];
    int count = 0;

    while (fgets(buffer, sizeof(buffer), pipe)) {
        std::string line(buffer);

        size_t dash = line.find(" - ");
        if (dash != std::string::npos) {

            std::string name = line.substr(0, dash);
            std::string desc = line.substr(dash + 3);

            name = highlight(name, query);
            desc = highlight(desc, query);

            std::cout << GREEN << "[+]" << RESET << " " << name << "\n";
            std::cout << "    " << desc;

            count++;
        }
    }

    pclose(pipe);

    std::cout << "\n" << GREEN << " Found: " << count << " packages\n" << RESET;

    return 0;
}
