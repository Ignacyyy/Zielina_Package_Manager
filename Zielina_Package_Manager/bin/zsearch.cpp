#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>

int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cout << "Usage: zsearch [package name or keyword]\n";
        return 1;
    }

    std::string command = "apt search ";

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        
        if (arg[0] == '-') {
            std::cout << "Invalid argument: " << arg << std::endl;
            std::cout << "Valid usage: zsearch [package name or keyword]\n";
            return 1;
        }

        command += arg + " ";
    }

    std::cout << "Searching packages...\n";
    sleep(1);

    
    std::system(command.c_str());

    std::cout << "\nSearch complete.\n";

    return 0;
}
