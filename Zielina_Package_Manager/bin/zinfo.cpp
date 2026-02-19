#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>

int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cout << "Usage: zinfo [package name]\n";
        return 1;
    }

    std::string command = "apt show ";

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        
        if (arg[0] == '-') {
            std::cout << "Invalid argument: " << arg << std::endl;
            std::cout << "Valid usage: zinfo [package name]\n";
            return 1;
        }

        command += arg + " ";
    }

    std::cout << "Fetching package info...\n";
    sleep(1);

    std::system(command.c_str());

    std::cout << "\nPackage info displayed.\n";

    return 0;
}
