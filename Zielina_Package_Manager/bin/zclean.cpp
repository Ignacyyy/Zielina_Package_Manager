#include <iostream>
#include <cstdlib>
#include <unistd.h>

int main() {
    std::cout << "Cleaning system cache and unused packages..." << std::endl;
    sleep(1);

    std::system("sudo apt autoremove -y");
    std::system("sudo apt clean");
    std::system("sudo apt autoclean");

    sleep(2);
    std::cout << "System cleanup complete!" << std::endl;
    return 0;
}
