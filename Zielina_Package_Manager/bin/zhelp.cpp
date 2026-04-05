
#include <iostream>
#include <string>
#include <cstdlib>

int main() 
{std::cout << "\033[1;31mZielina Package Manager\033[0m Ver 2.8" << std::endl;
// Zielony kolor dla tekstu
const std::string GREEN = "\033[1;32m"; // jasna zieleń
const std::string RESET = "\033[0m";    // reset koloru

std::cout << GREEN;
    
 std::cout << R"(    
  

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
)" << std::endl;
std::cout << RESET; // reset koloru do domyślnego
std::cout << "\033[1;31mOptions:\033[0m" << std::endl;
std::cout << "\033[1mzhelp\033[0m - Display this help message, Usage: zhelp" << std::endl;
std::cout << "\033[1mzupd\033[0m - Update package, Usage: zupd or zupd -full...(etc.), Agr -full, -r, -s -f" << std::endl;
std::cout << "\033[1mzinst\033[0m - Install package, Usage: zinst  or zinst -f [package name], Agr -f" << std::endl;
std::cout << "\033[1mzrm\033[0m - Remove package, Usage: zrm [package name]" << std::endl;
std::cout << "\033[1mzlist\033[0m - List all packages, Usage: zlist" << std::endl;
std::cout << "\033[1mzsearch\033[0m - Search for a package, Usage: zsearch [package name]" << std::endl;
std::cout << "\033[1mzinfo\033[0m - Get package information, Usage: zinfo [package name]" << std::endl;
std::cout << "\033[1mzuninstall\033[0m - Uninstall Zielina_Package_Manager, Usage: sudo zuninstall" << std::endl;
std::cout << "\033[1mzr\033[0m - System reboot, Usage: sudo zr" << std::endl;
std::cout << "\033[1mzs\033[0m - System Shutdown, Usage sudo zs" <<std::endl;
std::cout << "\033[1mzclean\033[0m - System package/cache cleaning, Usage: zclean" << std::endl;
std::cout << "\033[1mzupgr\033[0m - Zielina Package Manager update, Usage: sudo zupgr" <<std::endl;
std::cout << "" << std::endl;
std::cout << "" << std::endl;
std::cout << "\033[1;31mAdditional Info:\033[0m" << std::endl;
std::cout << "zupd -f (flatpak) -r (reboot) -s (shutdown)" << std::endl;
std::cout << "zinst -f (flatpak)" <<std::endl;
return 0;
}
