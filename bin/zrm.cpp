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
        { lock_guard<mutex> lock(consoleMutex); progressPercent++; }
        this_thread::sleep_for(chrono::milliseconds(30));
    }
    while (progressPercent < 70 && !done) {
        { lock_guard<mutex> lock(consoleMutex); progressPercent++; }
        this_thread::sleep_for(chrono::milliseconds(80));
    }
    while (progressPercent < 90 && !done) {
        { lock_guard<mutex> lock(consoleMutex); progressPercent++; }
        this_thread::sleep_for(chrono::milliseconds(200));
    }
    while (!done) {
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    { lock_guard<mutex> lock(consoleMutex); progressPercent = 100; }
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

// Get installed Flatpaks
vector<string> getInstalledFlatpaks(const string& query = "") {
    vector<string> results;
    string cmd = "flatpak list --columns=application 2>/dev/null";
    if (!query.empty()) cmd += " | grep -i \"" + query + "\"";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return results;

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        string line = buffer;
        line.erase(line.find_last_not_of(" \n\r\t") + 1);
        if (!line.empty()) results.push_back(line);
    }
    pclose(pipe);
    sort(results.begin(), results.end());
    return results;
}

vector<string> chooseFlatpakPackages(const vector<string>& packages, const string& query) {
    if (packages.empty()) {
        cout << YELLOW << "No installed Flatpak packages found";
        if (!query.empty()) cout << " for '" << query << "'";
        cout << "." << RESET << endl;
        return {};
    }

    cout << GREEN << "\nInstalled Flatpak packages";
    if (!query.empty()) cout << " for '" << query << "'";
    cout << ":\n" << RESET << endl;

    for (size_t i = 0; i < packages.size(); ++i) {
        cout << "  " << (i + 1) << ". " << packages[i] << endl;
    }

    cout << "\n  0. Skip this list" << endl << endl;
    cout << "Enter number(s) to remove (e.g. 1 3 5): ";

    string input;
    getline(cin, input);

    vector<string> selected;
    size_t pos = 0;
    while (pos < input.length()) {
        while (pos < input.length() && isspace(input[pos])) pos++;
        if (pos >= input.length()) break;

        size_t end = pos;
        while (end < input.length() && !isspace(input[end])) end++;

        string token = input.substr(pos, end - pos);
        try {
            int num = stoi(token);
            if (num == 0) return {};
            if (num > 0 && num <= static_cast<int>(packages.size())) {
                selected.push_back(packages[num - 1]);
            }
        } catch (...) {}
        pos = end;
    }
    return selected;
}

int main(int argc, char* argv[]) {
    if (geteuid() != 0) {
        cout << RED << "Run with sudo!\n" << RESET;
        return 1;
    }

    bool useFlatpak = false;
    bool purge = false;
    vector<string> packages;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-f") useFlatpak = true;
        else if (arg == "-p") purge = true;
        else packages.push_back(arg);
    }

    if (useFlatpak) {
        if (purge) cout << YELLOW << "Note: -p is ignored for Flatpak.\n" << RESET;

        vector<string> toRemove;
        if (packages.empty()) {
            auto installed = getInstalledFlatpaks();
            auto selected = chooseFlatpakPackages(installed, "");
            toRemove.insert(toRemove.end(), selected.begin(), selected.end());
        } else {
            for (const auto& q : packages) {
                auto found = getInstalledFlatpaks(q);
                auto selected = chooseFlatpakPackages(found, q);
                toRemove.insert(toRemove.end(), selected.begin(), selected.end());
            }
        }

        if (toRemove.empty()) {
            cout << YELLOW << "No packages selected.\n" << RESET;
            return 0;
        }
        packages = toRemove;
    }

    if (packages.empty()) {
        cout << YELLOW << "No package specified!\n" << RESET;
        return 1;
    }

    cout << (useFlatpak ? RED + "Using Flatpak\n" : RED + "Using APT\n") << RESET;
    cout << "Removing packages...\n" << endl;

    int totalPackages = packages.size();
    int completedPackages = 0;
    vector<PackageResult> results;
    bool anyFailed = false;

    thread progressThread(showProgress, totalPackages, ref(completedPackages));

    for (auto& pkg : packages) {
        PackageResult res;
        res.name = pkg;

        if (useFlatpak) {
            if (!isInstalledFlatpak(pkg)) {
                res.message = YELLOW + "Package " + pkg + " not found." + RESET;
                res.success = false;
                anyFailed = true;
            } else {
                int status = runWithFakeProgress("flatpak uninstall --force-remove --delete-data -y " + pkg);

                if (!isInstalledFlatpak(pkg)) {
                    res.message = "Package " + pkg + " removed successfully." + RESET;
                    res.success = true;
                } else {
                    runWithFakeProgress("flatpak uninstall -y " + pkg);
                    if (!isInstalledFlatpak(pkg)) {
                        res.message = "Package " + pkg + " removed successfully." + RESET;
                        res.success = true;
                    } else {
                        res.message = YELLOW + "Package " + pkg + " removal may have failed." + RESET;
                        res.success = false;
                        anyFailed = true;
                    }
                }
            }
        } else {
            if (!isInstalledAPT(pkg)) {
                res.message = YELLOW + "Package " + pkg + " is not installed." + RESET;
                res.success = false;
                anyFailed = true;
            } else {
                string cmd = purge ?
                    "DEBIAN_FRONTEND=noninteractive apt-get -qq -y purge " + pkg :
                    "DEBIAN_FRONTEND=noninteractive apt-get -qq -y remove " + pkg;

                int status = runWithFakeProgress(cmd);
                if (status == 0) {
                    res.message = "Package " + pkg +
                                  (purge ? " purged successfully." : " removed successfully.") + RESET;
                    res.success = true;
                } else {
                    res.message = YELLOW + "Package " + pkg + " removal failed." + RESET;
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
        cout << RED << "Removal finished with errors!" << RESET << endl;
    else
        cout << GREEN << "Removal complete!" << RESET << endl;

    return 0;
}