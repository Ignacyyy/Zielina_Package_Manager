#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>

int main() {
    std::cout << "Listing all packages...\n";
    sleep(1);
    int status = std::system("apt list --installed");
    
    return 0;
}   