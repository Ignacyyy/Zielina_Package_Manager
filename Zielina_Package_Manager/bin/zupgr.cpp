#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <string>

using namespace std;

int main()
{
    if (geteuid() != 0) {
        cout << "Run with sudo!\n";
        return 1;
    }

    string answer;
    cout << "Zielina Package Manager Updater. Do you want to continue? [y/N]\n";
    cin >> answer;

    if(answer == "y" || answer == "Y" || answer == "yes" || answer == "Yes" || answer =="YES" ||
       answer =="yEs" || answer == "yeS" || answer == "tak" || answer == "T" || answer == "t")
    {
        cout << "Downloading latest version...\n";
        system("rm -rf /tmp/Zielina_Package_Manager");
        system("git clone https://github.com/Ignacyyy/Zielina_Package_Manager.git /tmp/Zielina_Package_Manager");

        cout << "Updating Zielina Package Manager folder...\n";
        // Nadpisuje wszystkie pliki i foldery, zachowując istniejące niezmienione pliki
        system("rsync -a /tmp/Zielina_Package_Manager/ /usr/local/bin/Zielina_Package_Manager/");

        cout << "Cleaning up temporary files...\n";
        system("rm -rf /tmp/Zielina_Package_Manager");

        cout << "Update finished!\n";
    }

    return 0;
}
