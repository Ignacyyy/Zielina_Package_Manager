#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <cstdio>
#include <mutex>

using namespace std;

mutex consoleMutex;
int progressPercent = 0;

const string GREEN  = "\033[32m";
const string YELLOW = "\033[33m";
const string RED    = "\033[31m";
const string RESET  = "\033[0m";

void showProgress(int totalPackages, int& completedPackages) {
    char spinner[] = {'|', '/', '-', '\\'};
    int spinIndex = 0;
    int width = 50;

    while (completedPackages < totalPackages) {
        {
            lock_guard<mutex> lock(consoleMutex);
            int pos = (progressPercent * width) / 100;
            cout << "\r" << GREEN
                 << "Progress: ["
                 << string(pos, '=')
                 << string(width - pos, ' ')
                 << "] " << progressPercent << "% "
                 << spinner[spinIndex]
                 << RESET << flush;
        }
        spinIndex = (spinIndex + 1) % 4;
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    {
        lock_guard<mutex> lock(consoleMutex);
        cout << "\r" << GREEN
             << "Progress: [" << string(width, '=') << "] 100%"
             << RESET << endl;
    }
}

// APT helpers
bool isAptInstalled(const string& pkg) {
    string cmd = "dpkg-query -W -f='${Status}' " + pkg + " 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return false;
    char buffer[128];
    string status = "";
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        status = buffer;
    pclose(pipe);
    return status.find("install ok installed") != string::npos;
}

bool isAptAvailable(const string& pkg) {
    string cmd = "apt-cache show " + pkg + " > /dev/null 2>&1";
    return system(cmd.c_str()) == 0;
}

// Flatpak helpers
bool isFlatpakInstalled(const string& pkg) {
    string cmd = "flatpak list | grep -F \"" + pkg + "\" > /dev/null";
    return system(cmd.c_str()) == 0;
}

bool isFlatpakAvailable(const string& pkg) {
    string cmd = "flatpak search " + pkg + " | grep -F \"" + pkg + "\" > /dev/null";
    return system(cmd.c_str()) == 0;
}

// Run command silently
int runCommand(const string& cmd) {
    string fullCmd = cmd + " > /dev/null 2>&1";
    int ret = system(fullCmd.c_str());
    return WEXITSTATUS(ret);
}

struct PackageResult {
    string name;
    string message;
    bool success;
};

int main(int argc, char* argv[]) {
    if (geteuid() != 0) {
        cout << RED << "Run with sudo!\n" << RESET;
        return 1;
    }

    if (argc < 2) {
        cout << "Usage:\n  zinst [package]\n  zinst -f [package]\n";
        return 1;
    }

    bool useFlatpak = false;
    vector<string> packages;
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-f") useFlatpak = true;
        else packages.push_back(arg);
    }

    if (packages.empty()) {
        cout << RED << "No package specified!\n" << RESET;
        return 1;
    }

    cout << (useFlatpak ? GREEN + "Using Flatpak\n" : GREEN + "Using APT\n") << RESET;
    cout << "Installing packages...\n" << endl;

    int totalPackages = packages.size();
    int completedPackages = 0;
    vector<PackageResult> results;
    bool anyFailed = false;

    thread progressThread(showProgress, totalPackages, ref(completedPackages));

    for (auto& p : packages) {
        PackageResult res;
        res.name = p;

        if (useFlatpak) {
            if (!isFlatpakAvailable(p)) {
                res.message = YELLOW + "Package " + p + " not found." + RESET;
                res.success = false;
                anyFailed = true;
            } else if (isFlatpakInstalled(p)) {
                res.message = YELLOW + "Package " + p + " is already installed." + RESET;
                res.success = false;
            } else {
                string cmd = "flatpak install -y -q " + p;
                int status = runCommand(cmd);
                if (status == 0) {
                    res.message = GREEN + "Package " + p + " installed successfully." + RESET;
                    res.success = true;
                } else {
                    res.message = YELLOW + "Package " + p + " installation failed." + RESET;
                    res.success = false;
                    anyFailed = true;
                }
            }
        } else {
            if (!isAptAvailable(p)) {
                res.message = YELLOW + "Package " + p + " not found." + RESET;
                res.success = false;
                anyFailed = true;
            } else if (isAptInstalled(p)) {
                res.message = YELLOW + "Package " + p + " is already installed." + RESET;
                res.success = false;
            } else {
                string cmd = "DEBIAN_FRONTEND=noninteractive apt-get -qq -y install " + p;
                int status = runCommand(cmd);
                if (status == 0) {
                    res.message = GREEN + "Package " + p + " installed successfully." + RESET;
                    res.success = true;
                } else {
                    res.message = YELLOW + "Package " + p + " installation failed." + RESET;
                    res.success = false;
                    anyFailed = true;
                }
            }
        }

        results.push_back(res);

        completedPackages++;
        progressPercent = (completedPackages * 100) / totalPackages;
    }

    progressPercent = 100;
    progressThread.join();

    cout << endl;
    for (auto& r : results)
        cout << r.message << endl;

    if (anyFailed)
        cout << RED << "Installation finished with errors!" << RESET << endl;
    else
        cout << RED << "Installation complete!" << RESET << endl;

    return 0;
}