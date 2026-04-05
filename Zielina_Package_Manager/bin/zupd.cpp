#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <vector>
#include <cstdio>
#include <atomic>
#include <mutex>
#include <algorithm>

using namespace std;

// Color codes
const string GREEN = "\033[32m";
const string RED = "\033[31m";
const string YELLOW = "\033[33m";
const string RESET = "\033[0m";
mutex progressMutex;

// Run a system command and handle errors
int runCommand(const string& cmd) {
    int status = system(cmd.c_str());
    if (status != 0) {
        lock_guard<mutex> lock(progressMutex);
        cerr << RED << "Command failed: " << cmd << RESET << "\n";
        return 1;
    }
    return 0;
}

// Smart progress bar
void showProgress(atomic<bool>& updateDone) {
    int progress = 0;
    char spinner[] = {'|','/','-','\\'};
    while (!updateDone.load()) {
        {
            lock_guard<mutex> lock(progressMutex);
            int width = 50;
            int pos = (progress * width) / 100;
            cout << "\r\033[2K" << GREEN
                 << "Progress: [" << string(pos,'=') << string(width-pos,' ')
                 << "] " << progress << "% " << spinner[progress % 4]
                 << RESET << flush;
        }
        this_thread::sleep_for(chrono::milliseconds(120));
        if (progress < 70) progress += 2;
        else if (progress < 90) progress += 1;
        else if (progress < 97) progress += 0;
    }
    {
        lock_guard<mutex> lock(progressMutex);
        cout << "\r\033[2K" << GREEN
             << "Progress: [" << string(50,'=') << "] 100%"
             << RESET << endl;
    }
}

int main(int argc, char* argv[]) {

    // Root check
    if (geteuid() != 0) {
        cout << RED << "run with sudo!\n" << RESET;
        return 1;
    }

    bool doFull = false;
    bool doReboot = false;
    bool doShutdown = false;
    bool doFlatpak = false;

    // Argument parsing
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-full") doFull = true;
        else if (arg == "-f") doFlatpak = true;
        else if (arg == "-r") doReboot = true;
        else if (arg == "-s") doShutdown = true;
        else {
            cout << "Invalid argument: " << arg << "\n";
            cout << "Valid: -full -f -r -s\n";
            return 1;
        }
    }

    if (doReboot && doShutdown) {
        cout << "Cannot use -r and -s together!\n";
        return 1;
    }

    // System info
    FILE* distro_pipe = popen("lsb_release -d 2>/dev/null | cut -f2", "r");
    char distro_buffer[256];
    string distro_info = "Unknown Linux Distribution";
    if (fgets(distro_buffer, sizeof(distro_buffer), distro_pipe) != nullptr) {
        distro_info = distro_buffer;
        distro_info.erase(distro_info.find_last_not_of(" \n\r\t")+1);
    }
    pclose(distro_pipe);

    cout << GREEN << YELLOW << "[SYS]" << GREEN << " " << distro_info << RESET << endl << endl;

    // Upgradable packages
    string cmd = "apt list --upgradable 2>/dev/null | tail -n +2 | cut -d'/' -f1";
    FILE* pipe = popen(cmd.c_str(), "r");
    vector<string> packages;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        string line = buffer;
        line.erase(line.find_last_not_of(" \n\r\t")+1);
        if (!line.empty()) packages.push_back(line);
    }
    pclose(pipe);
    bool hasPackages = !packages.empty();

    // Repositories
    cout << GREEN << YELLOW << "[D]" << GREEN << " Active Repositories:" << RESET << endl;
    FILE* repo_pipe = popen("grep -h '^deb ' /etc/apt/sources.list /etc/apt/sources.list.d/*.list 2>/dev/null | sort -u", "r");
    bool has_repos = false;
    while (fgets(buffer, sizeof(buffer), repo_pipe) != nullptr) {
        string line = buffer;
        line.erase(line.find_last_not_of(" \n\r\t")+1);
        if (!line.empty()) {
            cout << "  " << YELLOW << "-" << RESET << " " << line << endl;
            has_repos = true;
        }
    }
    pclose(repo_pipe);
    if (!has_repos) cout << "  No repositories found" << endl;

    // Flatpak
    if (system("which flatpak >/dev/null 2>&1") == 0) {
        cout << endl << GREEN << YELLOW << "[F]" << GREEN << " Flatpak:" << RESET << endl;
        FILE* flatpak_pipe = popen("flatpak remotes --columns=name 2>/dev/null", "r");
        bool has_flatpak_repos = false;
        while (fgets(buffer, sizeof(buffer), flatpak_pipe) != nullptr) {
            string line = buffer;
            line.erase(line.find_last_not_of(" \n\r\t")+1);
            if (!line.empty() && line != "Name") {
                cout << "    " << YELLOW << "-" << RESET << " " << line << endl;
                has_flatpak_repos = true;
            }
        }
        pclose(flatpak_pipe);
        if (!has_flatpak_repos) cout << "    No Flatpak repositories found" << endl;
    }

    // Snap
    if (system("which snap >/dev/null 2>&1") == 0) {
       cout << endl << GREEN << YELLOW << "[S]" << GREEN << " Snap:" << RESET << endl;
        FILE* snap_pipe = popen("snap list 2>/dev/null | tail -n +2 | awk '{print $1}' | head -5", "r");
        bool has_snap_installed = false;
        while (fgets(buffer, sizeof(buffer), snap_pipe) != nullptr) {
            string line = buffer;
            line.erase(line.find_last_not_of(" \n\r\t")+1);
            if (!line.empty()) {
                cout << "    " << YELLOW << "-" << RESET << " " << line << endl;
                has_snap_installed = true;
            }
        }
        pclose(snap_pipe);
        if (!has_snap_installed) cout << "    " << YELLOW << "-" << RESET << " Snap Store (default)" << endl;
    }

    cout << endl;

    // Progress bar
    cout << "Updating system...\n" << endl;
    atomic<bool> updateDone(false);
    thread progressThread(showProgress, ref(updateDone));

    // List packages
    if (hasPackages) {
        cout << GREEN << YELLOW << "[+]" << GREEN << " Packages to update: (" << packages.size() << ")" << RESET << endl;
        for (auto &pkg : packages) cout << "  [+] " << pkg << endl;
        cout << endl;
    } else {
        cout << RED << "System is up to date." << RESET << endl << endl;
    }

    // Update with error checking
    bool updateSuccess = true; // flaga sukcesu

    if (!doFlatpak && hasPackages) {
        int status;
        if (doFull) status = runCommand("apt update -qq >/dev/null 2>&1 && apt full-upgrade -y -qq >/dev/null 2>&1");
        else status = runCommand("apt update -qq >/dev/null 2>&1 && apt upgrade -y -qq >/dev/null 2>&1");
        if (status != 0) updateSuccess = false;

        status = runCommand("apt autoremove -y -qq >/dev/null 2>&1");
        if (status != 0) updateSuccess = false;
    }

    if ((doFlatpak || doFull) && hasPackages) {
        int status = runCommand("flatpak update -y --noninteractive >/dev/null 2>&1");
        if (status != 0) updateSuccess = false;
    }

    // Stop progress bar
    updateDone = true;
    progressThread.join();

    // Show final message based on success
    if (hasPackages) {
        if (updateSuccess)
            cout << GREEN << "Update complete successfully." << RESET << "\n";
        else
            cout << RED << "Update finished with errors. Check above messages." << RESET << "\n";
    }

    // Reboot/shutdown
    if (doReboot) {
        cout << RED << "System reboot..." << RESET << endl;
        this_thread::sleep_for(chrono::seconds(3));
        system("reboot");
    }
    if (doShutdown) {
        cout << RED << "System shutdown..." << RESET << endl;
        this_thread::sleep_for(chrono::seconds(3));
        system("shutdown now");
    }

    return 0;
}
