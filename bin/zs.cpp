#include <iostream>
#include <cstdlib>
#include <string>
#include <unistd.h>

int main(){
    std::string anwser;
    std::cout << " system Shutdown, You want to continue ? [y/N]" << std::endl;
    std::cin >> anwser;
     if(anwser == "y" || anwser == "Y" || anwser == "yes" || anwser == "Yes" || anwser =="YES" || anwser =="yEs" || anwser == "yeS" || anwser == "tak" || anwser == "T" || anwser == "t")
{
        std::system("sudo shutdown now");
        return 0;
    }
else{
    std::cout << "Canceling system shutdown..." << std::endl;
    sleep(2);
    std::cout << "System shutdown canceled" << std::endl;
    return 0;
}
}
