#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>

int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cout << "Usage: zrm [package name]\n";
        return 1;
    }

    std::string command = "sudo apt remove -y ";

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        // blokujemy niepoprawne argumenty
        if (arg[0] == '-') {
            std::cout << "Invalid argument: " << arg << std::endl;
            return 1;
        }

        command += arg + " ";
    }

    std::cout << "Removing packages...\n";
    sleep(1);

    int status = std::system(command.c_str());

    if (status != 0) {
        std::cout << "Package removal failed! Check package names.\n";
        return 1;
    }

    std::cout << "Package removal complete.\n";

    return 0;
}
