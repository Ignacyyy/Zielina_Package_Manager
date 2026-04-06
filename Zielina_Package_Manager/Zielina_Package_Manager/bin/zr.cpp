#include <iostream>
#include <cstdlib>
#include <string>
#include <unistd.h>

int main(){
    std::string anwser;
    std::cout << " System reboot, You want to continue ? [y/N]" << std::endl;
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
