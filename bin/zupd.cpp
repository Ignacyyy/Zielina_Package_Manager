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

const string GREEN = "\033[32m";
const string RED = "\033[31m";
const string YELLOW = "\033[33m";
const string RESET = "\033[0m";
mutex progressMutex;

vector<string> getFlatpakUpdates() {
    vector<string> updates;
    FILE* pipe = popen("flatpak remote-ls --updates --columns=application 2>/dev/null", "r");
    if (!pipe) return updates;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        string line = buffer;
        line.erase(line.find_last_not_of(" \n\r\t")+1);
        if (!line.empty()) updates.push_back(line);
    }
    pclose(pipe);
    return updates;
}

void fakeProgressThread(atomic<bool>& done, int& progressPercent) {
    while (progressPercent < 50 && !done) {
        { lock_guard<mutex> lock(progressMutex); progressPercent++; }
        this_thread::sleep_for(chrono::milliseconds(30));
    }
    while (progressPercent < 70 && !done) {
        { lock_guard<mutex> lock(progressMutex); progressPercent++; }
        this_thread::sleep_for(chrono::milliseconds(80));
    }
    while (progressPercent < 90 && !done) {
        { lock_guard<mutex> lock(progressMutex); progressPercent++; }
        this_thread::sleep_for(chrono::milliseconds(200));
    }
    while (!done) {
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

void showProgress(atomic<bool>& updateDone, int& progressPercent) {
    char spinner[] = {'|','/','-','\\'};
    int spinIndex = 0;
    int width = 50;
    while (!updateDone.load()) {
        {
            lock_guard<mutex> lock(progressMutex);
            int pos = (progressPercent * width) / 100;
            cout << "\r\033[2K" << GREEN
                 << "Progress: [" << string(pos,'=') << string(width-pos,' ')
                 << "] " << progressPercent << "% " << spinner[spinIndex]
                 << RESET << flush;
        }
        spinIndex = (spinIndex + 1) % 4;
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    {
        lock_guard<mutex> lock(progressMutex);
        cout << "\r\033[2K" << GREEN
             << "Progress: [" << string(50,'=') << "] 100%"
             << RESET << endl;
    }
}

int runCommand(const string& cmd) {
    string fullCmd = cmd + " > /dev/null 2>&1";
    int status = system(fullCmd.c_str());
    return WEXITSTATUS(status);
}

int main(int argc, char* argv[]) {

    if (geteuid() != 0) {
        cout << RED << "Run with sudo!\n" << RESET;
        return 1;
    }

    bool doFull = false;
    bool doReboot = false;
    bool doShutdown = false;
    bool doFlatpak = false;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-full") { doFull = true; doFlatpak = true; }
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

    // Upgradable APT packages
    char buffer[128];
    string cmd = "apt list --upgradable 2>/dev/null | tail -n +2 | cut -d'/' -f1";
    FILE* pipe = popen(cmd.c_str(), "r");
    vector<string> aptPackages;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        string line = buffer;
        line.erase(line.find_last_not_of(" \n\r\t")+1);
        if (!line.empty()) aptPackages.push_back(line);
    }
    pclose(pipe);

    bool hasAptUpdates = !aptPackages.empty();
    vector<string> flatpakPackages = getFlatpakUpdates();
    bool hasFlatpakUpdates = !flatpakPackages.empty();
    bool hasAnyUpdates = hasAptUpdates || (doFlatpak && hasFlatpakUpdates);

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

    // Flatpak remotes
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

    cout << endl;

    // List APT
    if (hasAptUpdates) {
        cout << GREEN << YELLOW << "[+]" << GREEN << " APT packages to update: (" << aptPackages.size() << ")" << RESET << endl;
        for (auto &pkg : aptPackages) cout << "  [+] " << pkg << endl;
        cout << endl;
    }

    // List Flatpak
    if (doFlatpak && hasFlatpakUpdates) {
        cout << GREEN << YELLOW << "[+]" << GREEN << " Flatpak packages to update: (" << flatpakPackages.size() << ")" << RESET << endl;
        for (auto &pkg : flatpakPackages) cout << "  [+] " << pkg << endl;
        cout << endl;
    }

    // System up to date?
    if (!hasAnyUpdates) {
        cout << RED << "System is up to date." << RESET << endl;
        if (!doFlatpak && hasFlatpakUpdates)
            cout << YELLOW << "Flatpak updates available, run with -f to update." << RESET << endl;
        cout << endl;
    } else {
        cout << "Updating system..." << endl;
        if (hasAptUpdates && doFlatpak && hasFlatpakUpdates)
            cout << RED << "Using APT + Flatpak" << RESET << endl;
        else if (hasAptUpdates)
            cout << RED << "Using APT" << RESET << endl;
        else
            cout << RED << "Using Flatpak" << RESET << endl;
        cout << endl;
    }

    // Progress + update
    int progressPercent = 0;
    atomic<bool> updateDone(false);
    thread progressThread(showProgress, ref(updateDone), ref(progressPercent));

    bool updateSuccess = true;

    if (hasAptUpdates) {
        atomic<bool> done(false);
        thread fakeThread(fakeProgressThread, ref(done), ref(progressPercent));

        int status;
        if (doFull) status = runCommand("apt update -qq && apt full-upgrade -y -qq");
        else        status = runCommand("apt update -qq && apt upgrade -y -qq");
        if (status != 0) updateSuccess = false;

        runCommand("apt autoremove -y -qq");

        done = true;
        fakeThread.join();
        progressPercent = 100;
    }

    if (doFlatpak && hasFlatpakUpdates) {
        progressPercent = 0;
        atomic<bool> done(false);
        thread fakeThread(fakeProgressThread, ref(done), ref(progressPercent));

        int status = runCommand("flatpak update -y --noninteractive");
        if (status != 0) updateSuccess = false;

        done = true;
        fakeThread.join();
        progressPercent = 100;
    }

    updateDone = true;
    progressThread.join();

    if (hasAnyUpdates) {
        if (updateSuccess)
            cout << GREEN << "Update complete successfully." << RESET << "\n";
        else
            cout << RED << "Update finished with errors." << RESET << "\n";
    }

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