// Compile: g++ -O2 -o zuninstall zuninstall.cpp

#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <vector>

using namespace std;

const string GREEN  = "\033[1;32m";
const string YELLOW = "\033[33m";
const string RESET  = "\033[0m";
const string RED    = "\033[31m";

void print_banner() {
    cout << GREEN;
    cout << R"(


                               ↑
                               ↑↑
                               ↖↑↙←                       ↑
                               ↑↖↙↖↘↖                     ↑
                               ↗↙↑↑↙↑↑←                 ↖↑←
                               →↖↖↓↑↖↑↓↑              →↑↑↑
                               ↙↑↖↖↑↖←↙↓↑           ↑↑↖ ↖←
                               ↑↑↖↖↗→↖↑↖↑→         ↙↖↑↑→↖↑
                                ↙↙↑↙↗↘↖↓↖↑       ↑↙←↑↓↖↘↑   ↑
                                ↑↘↖↖←↑↗↓↖↑      →↑↖↑←→↖↖↑  ←↑
                                 ↑←↑←↖↖←↓↘↖    ←↑ ↑↖↗↖↗↙↑ ↙←←
                                   ↑↗→↙↑↑↓    ↓↖↖↓↖↑→↖→→ ↖↖↖↖↙
                                     ↑↙←↘→    ↙↗↖↑↑↓↑↖↖↑ ↓↖↑↘←
               ↑↑↑↓↑↖↖↖↖↑↙↙↑↙→↙↘↖     →→↑↓←  ↑←↗↖→↖↖↖↑↑ ↖↑↖↗↖→
                 ↗←↖↖↑↑↘→↘↗↖↖↗↖↖↑↙↖↘    ↑↙↗  ↗↖↙↖↙←↑↓↑ ↓↖←→←↙←
                   ↙↑↙↖↖↖↙↑↑→↘↖→↖↑↖↖↑↑↖   ↑  ↙↘↖↑↖↙↑  ↖→↑↖↑←←↑
                      ↑↑↖↖↖↓←↑↗↗↑↖↑↓↖↙↖↘←    ↙↖↑↖→↑  ↓↙↑↖↖↑↖↖↓
                        ↑↑→←↖↙↖↖↑↓→↓↑↑↑←↙↙   ↖↑→↑    ↑↓↖↑↖↖↘↖→
                            →↙→↙↙↖→↖↖↖↖↓→↑↑ ↑↖↖     ↓↖↑↖→↑↖←↓↘
                                  ↑↘→↑↓→↑    ↑      →↖↖→←↖↘←↑
                                              ↑    ←→↗↖←↘↖↘←↓
                                               ↑   ←↖↖←↗↖↖↗↘
                                                ↘  ←→←↖↓↗↙→
                          ↓↘↑↑→↑↑↙↑↑←↓↑↙↖↓←↓     ↖ ↙↙↖↑↖↓→
                     ↓↖↓↑↓←↖↘↖↖↓↖↖↖↖↖↑↖↗↑↗↑↓↙↑←←    ↖↓↑←↑
                  ↑↘→↑↑↙↑↙↗↑↑↘ ↑↑→↘→↗↖↓↖↖↖↖↖←↖↙↖↖↖ ↑ ↖↗
                        ↙↙↓↑←↙↓↑↖↓↘↖→↘↓↑↑↑↑↗↑↑↑↖↑←→ ↑↖
                            →↑↖↓↙←↑←←→↓↖↙↗↖←↗↖←↖↙↑   ↑
                                  →→↙↓↘↓←↙↘↗↙          ↖
                                                        ↑

                                                           ↑
                                                            ↑
                                                              ↑
                                                                ↑↖
                                                                   ↑
                                                                     ↑
                                                                        ↑
                                                                           ↑↖
)" << RESET << "\n";
}

static void run(const string& cmd) {
    system(cmd.c_str());
}

int main(int argc, char* argv[]) {

    bool showHelp    = false;
    bool showVersion = false;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if      (arg == "--version" || arg == "-v") showVersion = true;
        else if (arg == "--help"    || arg == "-h") showHelp    = true;
    }

    if (showVersion && showHelp) {
        cout << YELLOW << "--version\n" << RESET;
        cout << RED << "zuninstall component version: 1.1 of ZPM\n" << RESET;
        cout << "https://github.com/Ignacyyy/Zielina_Package_Manager\n";
        cout << "Copyright (c) 2026 Ignacyyy\nLicense: MIT\n\n";
        cout << YELLOW << "--help\n" << RESET;
        cout << RED << "Usage: " << RESET << argv[0] << " [options]\n\n";
        cout << RED << "Options:\n" << RESET;
        cout << "  --version, -v  Show version information\n";
        cout << "  --help,    -h  Show this help message\n\n";
        cout << "Removes all ZPM binaries and data from the system.\n";
        return 0;
    }

    if (showVersion) {
        cout << RED << "zuninstall component version: 1.1 of ZPM\n" << RESET;
        cout << "https://github.com/Ignacyyy/Zielina_Package_Manager\n";
        cout << "Copyright (c) 2026 Ignacyyy\nLicense: MIT\n";
        return 0;
    }

    if (showHelp) {
        cout << RED << "Usage: " << RESET << argv[0] << " [options]\n\n";
        cout << RED << "Options:\n" << RESET;
        cout << "  --version, -v  Show version information\n";
        cout << "  --help,    -h  Show this help message\n\n";
        cout << "Removes all ZPM binaries and data from the system.\n";
        return 0;
    }

    if (geteuid() != 0) {
        cout << RED << "Run with sudo!\n" << RESET;
        return 1;
    }

    print_banner();

    cout << RED << "Zielina Package Manager Uninstall program\n" << RESET;
    cout << "Do you want to continue? [y/N]: ";

    string answer;
    getline(cin, answer);

    if (answer != "y" && answer != "Y") {
        cout << YELLOW << "Uninstall canceled.\n" << RESET;
        return 0;
    }

    cout << "Uninstalling...\n";

    vector<string> files = {
        "zhelp",
        "zinst",
        "zr",
        "zs",
        "zuninstall",
        "zupgr",
        "zclean",
        "zinfo",
        "zlist",
        "zrm",
        "zsearch",
        "zupd"
    };

    vector<string> dirs = {
        "/usr/bin/",
        "/usr/local/bin/",
        "/opt/ZPM/"
    };

    for (const auto& dir : dirs) {
        for (const auto& file : files) {
            string path = dir + file;
            string cmd = "[ -f " + path + " ] && rm -f " + path + " 2>/dev/null";
            run(cmd);
        }
    }

    run("rm -rf /opt/ZPM 2>/dev/null");
    run("rm -rf /opt/Zielina_Package_Manager 2>/dev/null");
    run("rm -f /etc/profile.d/ZPM.sh /etc/profile.d/Zielina_Package_Manager.sh 2>/dev/null");

    cout << GREEN << "Done. ZPM has been removed.\n" << RESET;
    return 0;
}