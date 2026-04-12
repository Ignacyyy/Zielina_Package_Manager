#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <cstdlib>
#include <unistd.h>
#include <cstdio>
#include <algorithm>
#include <atomic>
using namespace std;

mutex consoleMutex;
int progressPercent = 0;

const string GREEN = "\033[32m";
const string YELLOW = "\033[33m";
const string RED = "\033[31m";
const string RESET = "\033[0m";

struct PackageResult {
    string name;
    string message;
    bool success;
};

//Helpers
bool isInstalledAPT(const string& pkg) { 
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

bool isInstalledFlatpak(const string& pkg) {
    string cmd = "flatpak list | grep -F \"" + pkg + "\" > /dev/null 2>&1";
    return system(cmd.c_str()) == 0;
}

void fakeProgressThread(atomic<bool>& done) {
    
    while (progressPercent < 50 && !done) {
        {
            lock_guard<mutex> lock(consoleMutex);
            progressPercent++;
        }
        this_thread::sleep_for(chrono::milliseconds(30));
    }
    
    while (progressPercent < 70 && !done) {
        {
            lock_guard<mutex> lock(consoleMutex);
            progressPercent++;
        }
        this_thread::sleep_for(chrono::milliseconds(80));
    }
    
    while (progressPercent < 90 && !done) {
        {
            lock_guard<mutex> lock(consoleMutex);
            progressPercent++;
        }
        this_thread::sleep_for(chrono::milliseconds(200));
    }
   
    while (!done) {
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    
    {
        lock_guard<mutex> lock(consoleMutex);
        progressPercent = 100;
    }
}

int runWithFakeProgress(const string& cmd) {
    progressPercent = 0;
    atomic<bool> done(false);

    thread fakeThread(fakeProgressThread, ref(done));

    string fullCmd = cmd + " > /dev/null 2>&1";
    int ret = system(fullCmd.c_str());

    done = true;
    fakeThread.join();

    return WEXITSTATUS(ret);
}

// APT progress
int runAPTWithProgress(const string& pkg) {
    progressPercent = 0;
    atomic<bool> done(false);

    thread fakeThread(fakeProgressThread, ref(done));

    string cmd = "DEBIAN_FRONTEND=noninteractive apt-get -y "
                 "-o APT::Status-Fd=1 install " + pkg + " 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        done = true;
        fakeThread.join();
        return 1;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        string line = buffer;
        if (line.find("pmstatus") != string::npos) {
            size_t p1 = line.find(':');
            size_t p2 = line.find(':', p1 + 1);
            size_t p3 = line.find(':', p2 + 1);
            if (p1 != string::npos && p2 != string::npos && p3 != string::npos) {
                try {
                    float percent = stof(line.substr(p2 + 1, p3 - p2 - 1));
                    lock_guard<mutex> lock(consoleMutex);
                    progressPercent = (int)percent;
                } catch (...) {}
            }
        }
    }

    int ret = pclose(pipe);
    done = true;
    fakeThread.join();

    return WEXITSTATUS(ret);
}

void showProgress(int totalPackages, int& completedPackages) {
    char spinner[] = {'|', '/', '-', '\\'};
    int spinIndex = 0;
    int width = 50;
    while (completedPackages < totalPackages) {
        {
            lock_guard<mutex> lock(consoleMutex);
            int pos = (progressPercent * width) / 100;
            cout << "\r" << GREEN << "Progress: ["
                 << string(pos, '=') << string(width - pos, ' ')
                 << "] " << progressPercent << "% " << spinner[spinIndex] << RESET << flush;
        }
        spinIndex = (spinIndex + 1) % 4;
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    {
        lock_guard<mutex> lock(consoleMutex);
        cout << "\r" << GREEN << "Progress: [" << string(width, '=') << "] 100%" << RESET << endl;
    }
}

// -f arg and search
vector<string> searchFlatpak(const string& query) {
    vector<string> results;
    string cmd = "flatpak search --columns=application \"" + query + "\" 2>/dev/null";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return results;

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        string line = buffer;
        line.erase(line.find_last_not_of(" \n\r\t") + 1);
        if (!line.empty()) results.push_back(line);
    }
    pclose(pipe);

    vector<string> filtered;
    string q_lower = query;
    transform(q_lower.begin(), q_lower.end(), q_lower.begin(), ::tolower);
    for (const auto& pkg : results) {
        string pkg_lower = pkg;
        transform(pkg_lower.begin(), pkg_lower.end(), pkg_lower.begin(), ::tolower);
        if (pkg_lower.find(q_lower) != string::npos) {
            filtered.push_back(pkg);
        }
    }
    sort(filtered.begin(), filtered.end());
    return filtered;
}

string chooseFlatpakPackage(const vector<string>& packages, const string& query) {
    if (packages.empty()) {
        cout << YELLOW << "No packages found for '" << query << "'." << RESET << endl;
        return "";
    }

    cout << GREEN << "\nFound Flatpak packages for '" << query << "':\n" << RESET << endl;
    for (size_t i = 0; i < packages.size(); ++i) {
        cout << "  " << (i + 1) << ". " << packages[i] << endl;
    }
    cout << "\n  0. Cancel" << endl << endl;
    cout << "Enter number to install: ";

    string input;
    getline(cin, input);
    try {
        int choice = stoi(input);
        if (choice == 0) return "";
        if (choice > 0 && choice <= static_cast<int>(packages.size())) {
            return packages[choice - 1];
        }
    } catch (...) {}
    cout << RED << "Invalid choice!" << RESET << endl;
    return "";
}


int main(int argc, char* argv[]) {
    if (geteuid() != 0) {
        cout << RED << "Run with sudo!\n" << RESET;
        return 1;
    }

    bool useFlatpak = false;
    vector<string> packages;
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-f") useFlatpak = true;
        else packages.push_back(arg);
    }

    if (useFlatpak) {
        vector<string> toInstall;
        if (packages.empty()) {
            string query;
            cout << "Enter search term: ";
            getline(cin, query);
            if (!query.empty()) {
                auto found = searchFlatpak(query);
                string selected = chooseFlatpakPackage(found, query);
                if (!selected.empty()) toInstall.push_back(selected);
            }
        } else {
            for (const auto& q : packages) {
                auto found = searchFlatpak(q);
                string selected = chooseFlatpakPackage(found, q);
                if (!selected.empty()) toInstall.push_back(selected);
            }
        }
        if (toInstall.empty()) {
            cout << YELLOW << "No packages selected.\n" << RESET;
            return 0;
        }
        packages = toInstall;
    }

    if (packages.empty()) {
        cout << YELLOW << "No package specified!\n" << RESET;
        return 1;
    }

    cout << (useFlatpak ? RED + "Using Flatpak\n" : RED + "Using APT\n") << RESET;
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
            if (!isInstalledFlatpak(p)) {
                string cmd = "flatpak install -y flathub " + p;
                int status = runWithFakeProgress(cmd);

                if (status == 0) {
                    res.message = "Package " + p + " installed successfully." + RESET;
                    res.success = true;
                } else {
                    res.message = YELLOW + "Package " + p + " installation failed." + RESET;
                    res.success = false;
                    anyFailed = true;
                }
            } else {
                res.message = YELLOW + "Package " + p + " is already installed." + RESET;
                res.success = false;
            }
        } else {
            if (isInstalledAPT(p)) {
                res.message = YELLOW + "Package " + p + " is already installed." + RESET;
                res.success = false;
            } else {
                int status = runAPTWithProgress(p);
                if (status == 0) {
                    res.message = "Package " + p + " installed successfully." + RESET;
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
        progressPercent = 100;
    }

    progressPercent = 100;
    progressThread.join();
    cout << endl;

    for (auto& r : results)
        cout << r.message << endl;

    if (anyFailed)
        cout << RED << "Installation finished with errors!" << RESET << endl;
    else
        cout << GREEN << "Installation complete!" << RESET << endl;

    return 0;
}