// Compile: g++ -O2 -pthread -o zrm zrm.cpp

#include <iostream>
#include <string>
#include <vector>
#include <atomic>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <cstdio>
#include <algorithm>
#include <csignal>
#include <cmath>
#include <fcntl.h>
#include <pthread.h>
#include <sys/wait.h>

using namespace std;

const string GREEN  = "\033[32m";
const string YELLOW = "\033[33m";
const string RED    = "\033[31m";
const string CYAN   = "\033[36m";
const string BOLD   = "\033[1m";
const string RESET  = "\033[0m";
const string LOG_PATH = "/tmp/zrm.log";

volatile sig_atomic_t g_interrupted = 0;

struct PackageResult {
    string name;
    string message;
    bool   success = false;
};

// ─── signal ──────────────────────────────────────────────────────────────────
void handleSigint(int) { g_interrupted = 1; }

// ─── progress bar ─────────────────────────────────────────────────────────────
void drawGlobalBar(float pct, const string& task) {
    int width   = 40;
    int percent = static_cast<int>(pct);
    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;
    int pos = width * percent / 100;

    cout << "\r\033[K" << YELLOW << "Install Progress: [" << RESET;
    for (int i = 0; i < width; ++i)
        cout << (i < pos ? GREEN + "#" + RESET : " ");
    cout << YELLOW << "] " << percent << "% " << RESET << "| " << task << flush;
}

static string stripAnsi(const string& in) {
    string out;
    bool inEsc = false;
    for (size_t i = 0; i < in.size(); ++i) {
        unsigned char c = (unsigned char)in[i];
        if (inEsc) {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) inEsc = false;
            continue;
        }
        if (c == '\033') { inEsc = true; continue; }
        if (c == '\r' || c == '\n') continue;
        if (c < 32 || c == 127) continue;
        out += (char)c;
    }
    return out;
}

static string trimForTask(const string& in, size_t maxLen = 48) {
    string s = stripAnsi(in);
    string collapsed;
    bool prevSpace = true;
    for (char c : s) {
        if (c == ' ' || c == '\t') {
            if (!prevSpace) { collapsed += ' '; prevSpace = true; }
        } else { collapsed += c; prevSpace = false; }
    }
    while (!collapsed.empty() && collapsed.back() == ' ') collapsed.pop_back();
    if (collapsed.size() > maxLen) collapsed = collapsed.substr(0, maxLen);
    return collapsed.empty() ? "Working..." : collapsed;
}

// ─── detection helpers ────────────────────────────────────────────────────────
bool isInstalledAPT(const string& pkg) {
    string cmd = "dpkg-query -W -f='${Status}' " + pkg + " 2>/dev/null";
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) return false;
    char buf[128]; string status;
    if (fgets(buf, sizeof(buf), p)) status = buf;
    pclose(p);
    return status.find("install ok installed") != string::npos;
}

bool isInstalledFlatpak(const string& pkg) {
    string cmd = "flatpak list --columns=application | grep -Fx \"" + pkg + "\" >/dev/null 2>&1";
    return system(cmd.c_str()) == 0;
}

bool isInstalledSnap(const string& pkg) {
    return system(("snap list " + pkg + " >/dev/null 2>&1").c_str()) == 0;
}

// ─── progress render thread ───────────────────────────────────────────────────
struct BarState {
    atomic<float> progress{0.0f};
    atomic<bool>  running{true};
    string        label;
};

static void* barThread(void* arg) {
    BarState* s = static_cast<BarState*>(arg);
    while (s->running.load()) {
        drawGlobalBar(s->progress.load(), s->label);
        usleep(80000);
    }
    return nullptr;
}

// ─── APT/pmstatus helpers ─────────────────────────────────────────────────────
static bool parsePmStatus(const string& line, float& pct, string& msg) {
    if (line.rfind("pmstatus:", 0) != 0) return false;
    size_t c1 = line.find(':');
    size_t c2 = line.find(':', c1 + 1);
    size_t c3 = line.find(':', c2 + 1);
    if (c3 == string::npos) return false;
    try { pct = stof(line.substr(c2 + 1, c3 - c2 - 1)); } catch (...) { return false; }
    msg = trimForTask(line.substr(c3 + 1));
    return true;
}

// ─── removers ─────────────────────────────────────────────────────────────────

// APT remove: use Status-Fd=3 for real percent
int runAPTRemoveWithProgress(const string& pkg, bool purge,
                              float startRange, float endRange) {
    string exitFile = "/tmp/zrm_apt_exit_" + to_string(getpid());
    int pfd[2];
    if (pipe(pfd) != 0) return 1;

    pid_t pid = fork();
    if (pid == 0) {
        close(pfd[0]);
        dup2(pfd[1], 3);
        close(pfd[1]);
        int logfd = open(LOG_PATH.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (logfd >= 0) { dup2(logfd, 1); dup2(logfd, 2); close(logfd); }
        setenv("DEBIAN_FRONTEND", "noninteractive", 1);
        const char* op = purge ? "purge" : "remove";
        execlp("apt-get", "apt-get", "-y", "-o", "APT::Status-Fd=3",
               op, pkg.c_str(), nullptr);
        _exit(127);
    }
    if (pid < 0) { close(pfd[0]); close(pfd[1]); return 1; }
    close(pfd[1]);

    BarState bs;
    bs.progress = startRange;
    bs.label    = string("APT ") + (purge ? "purge" : "remove") + " " + pkg + ": preparing...";
    pthread_t tid;
    pthread_create(&tid, nullptr, barThread, &bs);

    FILE* f = fdopen(pfd[0], "r");
    if (f) setvbuf(f, nullptr, _IONBF, 0);
    char buf[512];

    while (f && fgets(buf, sizeof(buf), f)) {
        if (g_interrupted) {
            bs.running = false; pthread_join(tid, nullptr);
            fclose(f); kill(pid, SIGTERM); waitpid(pid, nullptr, 0); return 130;
        }
        string line = buf;
        float pct = 0.0f; string msg;
        if (parsePmStatus(line, pct, msg)) {
            float gp = startRange + (pct / 100.0f) * (endRange - startRange);
            if (gp > bs.progress.load()) bs.progress = gp;
            bs.label = "APT: " + msg;
        }
    }
    if (f) fclose(f);

    bs.running = false;
    pthread_join(tid, nullptr);

    int wst = 0;
    waitpid(pid, &wst, 0);
    return WIFEXITED(wst) ? WEXITSTATUS(wst) : 1;
}

// Flatpak remove: no progress output — time-based animation
int runFlatpakRemoveWithProgress(const string& pkg, float startRange, float endRange) {
    string exitFile = "/tmp/zrm_fp_exit_" + to_string(getpid());
    string cmd = "flatpak uninstall -y --delete-data " + pkg
                 + " >> " + LOG_PATH + " 2>&1; echo $? > " + exitFile;

    pid_t pid = fork();
    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        _exit(127);
    }
    if (pid < 0) return 1;

    float range = endRange - startRange;
    const float T = 10.0f; // removal is faster than install
    auto timeNow = []() -> float {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts.tv_sec + ts.tv_nsec * 1e-9f;
    };
    float t0 = timeNow();

    while (true) {
        if (g_interrupted) { kill(pid, SIGTERM); waitpid(pid, nullptr, 0); return 130; }
        if (waitpid(pid, nullptr, WNOHANG) == pid) break;
        usleep(100000);
        float t    = timeNow() - t0;
        float frac = 1.0f - 1.0f / (1.0f + t / T);
        float gp   = startRange + range * frac * 0.95f;
        drawGlobalBar(gp, "Flatpak " + pkg + ": removing...");
    }

    int exitCode = 1;
    { ifstream ef(exitFile); if (ef) ef >> exitCode; remove(exitFile.c_str()); }
    return exitCode;
}

// Snap remove: fast, just animate
int runSnapRemoveWithProgress(const string& pkg, float startRange, float endRange) {
    string exitFile = "/tmp/zrm_snap_exit_" + to_string(getpid());
    string cmd = "snap remove " + pkg + " >> " + LOG_PATH + " 2>&1; echo $? > " + exitFile;

    pid_t pid = fork();
    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        _exit(127);
    }
    if (pid < 0) return 1;

    float range = endRange - startRange;
    const float T = 8.0f;
    auto timeNow = []() -> float {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts.tv_sec + ts.tv_nsec * 1e-9f;
    };
    float t0 = timeNow();

    while (true) {
        if (g_interrupted) { kill(pid, SIGTERM); waitpid(pid, nullptr, 0); return 130; }
        if (waitpid(pid, nullptr, WNOHANG) == pid) break;
        usleep(100000);
        float t    = timeNow() - t0;
        float frac = 1.0f - 1.0f / (1.0f + t / T);
        float gp   = startRange + range * frac * 0.95f;
        drawGlobalBar(gp, "Snap " + pkg + ": removing...");
    }

    int exitCode = 1;
    { ifstream ef(exitFile); if (ef) ef >> exitCode; remove(exitFile.c_str()); }
    return exitCode;
}

// ─── installed Flatpak list ───────────────────────────────────────────────────
vector<string> getInstalledFlatpaks(const string& query = "") {
    vector<string> results;
    string cmd = "flatpak list --columns=application 2>/dev/null";
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) return results;
    char buf[256];
    while (fgets(buf, sizeof(buf), p)) {
        string line = buf;
        line.erase(line.find_last_not_of(" \n\r\t") + 1);
        if (!line.empty()) results.push_back(line);
    }
    pclose(p);
    if (!query.empty()) {
        string q = query;
        transform(q.begin(), q.end(), q.begin(), ::tolower);
        vector<string> filtered;
        for (auto& r : results) {
            string l = r; transform(l.begin(), l.end(), l.begin(), ::tolower);
            if (l.find(q) != string::npos) filtered.push_back(r);
        }
        results = filtered;
    }
    sort(results.begin(), results.end());
    return results;
}

// ─── removal source menu ──────────────────────────────────────────────────────
//
//  Package: obs-studio
//    1. APT:     installed
//    2. Snap:    none
//    3. Flatpak: installed
//    0. Skip
//  Choose: _
//
string chooseRemoveMenu(const string& pkg,
                        bool aptInstalled,
                        bool snapInstalled,
                        bool flatpakInstalled,
                        bool snapSystemAvail,
                        bool flatpakSystemAvail) {

    struct Option { int num; string key; };
    vector<Option> options;
    int idx = 1;
    int aptNum = -1, snapNum = -1, flatpakNum = -1;

    cout << "\n" << BOLD << "Package: " << CYAN << pkg << RESET << "\n";

    // APT
    cout << "  " << BOLD << idx << ". APT:     " << RESET;
    if (aptInstalled) {
        cout << GREEN << "installed" << RESET << "\n";
        aptNum = idx;
        options.push_back({aptNum, "apt"});
    } else {
        cout << RED << "none" << RESET << "\n";
    }
    idx++;

    // Snap
    if (snapSystemAvail) {
        cout << "  " << BOLD << idx << ". Snap:    " << RESET;
        if (snapInstalled) {
            cout << GREEN << "installed" << RESET << "\n";
            snapNum = idx;
            options.push_back({snapNum, "snap"});
        } else {
            cout << RED << "none" << RESET << "\n";
        }
        idx++;
    }

    // Flatpak
    if (flatpakSystemAvail) {
        cout << "  " << BOLD << idx << ". Flatpak: " << RESET;
        if (flatpakInstalled) {
            cout << GREEN << "installed" << RESET << "\n";
            flatpakNum = idx;
            options.push_back({flatpakNum, "flatpak"});
        } else {
            cout << RED << "none" << RESET << "\n";
        }
        idx++;
    }

    cout << "  " << BOLD << "0. Skip" << RESET << "\n";

    if (options.empty()) {
        cout << YELLOW << "Package '" << pkg << "' is not installed anywhere.\n" << RESET;
        return "";
    }

    while (true) {
        cout << BOLD << "Choose: " << RESET;
        string input;
        if (!getline(cin, input)) return "";
        int choice = -1;
        try { choice = stoi(input); } catch (...) {}
        if (choice == 0) return "";
        for (const auto& o : options)
            if (o.num == choice) return o.key;
        cout << RED << "Invalid choice, try again.\n" << RESET;
    }
}

// Flatpak: pick which installed app(s) to remove
vector<string> chooseFlatpakToRemove(const vector<string>& installed, const string& query) {
    if (installed.empty()) {
        cout << YELLOW << "No installed Flatpak packages";
        if (!query.empty()) cout << " matching '" << query << "'";
        cout << ".\n" << RESET;
        return {};
    }
    cout << GREEN << "\nInstalled Flatpak packages";
    if (!query.empty()) cout << " matching '" << query << "'";
    cout << ":\n" << RESET;
    for (size_t i = 0; i < installed.size(); ++i)
        cout << "  " << (i + 1) << ". " << installed[i] << "\n";
    cout << "  0. Cancel\n" << BOLD << "Enter number(s) to remove (e.g. 1 3): " << RESET;

    string input;
    if (!getline(cin, input)) return {};
    vector<string> selected;
    size_t pos = 0;
    while (pos < input.size()) {
        while (pos < input.size() && isspace((unsigned char)input[pos])) pos++;
        if (pos >= input.size()) break;
        size_t end = pos;
        while (end < input.size() && !isspace((unsigned char)input[end])) end++;
        string tok = input.substr(pos, end - pos);
        try {
            int n = stoi(tok);
            if (n == 0) return {};
            if (n >= 1 && n <= (int)installed.size()) selected.push_back(installed[n - 1]);
        } catch (...) {}
        pos = end;
    }
    return selected;
}

// ─── main ─────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    signal(SIGINT, handleSigint);
    setvbuf(stdout, nullptr, _IONBF, 0);

    bool showHelp    = false;
    bool showVersion = false;
    bool purge       = false;
    vector<string> packages;

    bool hasFlatpak = (system("command -v flatpak >/dev/null 2>&1") == 0);
    bool hasSnap    = (system("command -v snap    >/dev/null 2>&1") == 0);

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if      (arg == "--help"    || arg == "-h") showHelp = true;
        else if (arg == "--version" || arg == "-v") showVersion = true;
        else if (arg == "--purge"   || arg == "-p") purge = true;
        else packages.push_back(arg);
    }

    if(showVersion && showHelp){
        cout << YELLOW << "--help" << RESET << endl;
        cout << RED << "zrm component version: 1.2 of ZPM\n" << RESET;
        cout << "https://github.com/Ignacyyy/ZPM\n";
        cout << "Copyright (c) 2026 Ignacyyy\nLicense: MIT\n";
        cout << "" << endl;
        cout << YELLOW << "--version" << RESET << endl;
        cout << RED << "Usage: " << RESET << argv[0] << " [options] [packages...]" << " or zpm rm/remove [options] [packages...]"  "\n\n";
        cout << RED << "Options:\n" << RESET;
        cout << "  (auto)         Picks APT / Flatpak / Snap per package\n";
        cout << "  --purge, -p    APT purge instead of remove\n";
        cout << "  --version, -v  Show version information\n";
        cout << "  --help,    -h  Show this help message\n";
        return 0;
    }
    if (purge && showVersion || purge && showHelp)
    {
        cout << RED << "Error: -p are mutually exclusive. " << endl; cout << "--help and --version cannot be combined with other options." << RESET << endl;
        return 0;
    }

    if (showVersion) {
        cout << RED << "zrm component version: 1.2 of ZPM\n" << RESET;
        cout << "https://github.com/Ignacyyy/ZPM\n";
        cout << "Copyright (c) 2026 Ignacyyy\nLicense: MIT\n";
        return 0;
    }

    if (showHelp) {
        cout << RED << "Usage: " << RESET << argv[0] << " [options] [packages...]" << " or zpm rm/remove [options] [packages...]"  "\n\n";
        cout << RED << "Options:\n" << RESET;
        cout << "  (auto)         Picks APT / Flatpak / Snap per package\n";
        cout << "  --purge, -p    APT purge instead of remove\n";
        cout << "  --version, -v  Show version information\n";
        cout << "  --help,    -h  Show this help message\n";
        return 0;
    }

    // sudo required only for actual removal operations
    if (geteuid() != 0) {
        cout << RED << "Run with sudo!\n" << RESET;
        return 1;
    }

    if (packages.empty()) {
        cout << YELLOW << "No package specified!\n" << RESET;
        return 1;
    }

    // ── resolve each package ──────────────────────────────────────────────────
    struct RemoveTarget {
        string name;
        bool   useFlatpak = false;
        bool   useSnap    = false;
        bool   purge      = false;
    };
    vector<RemoveTarget> targets;

    for (const string& pkg : packages) {
        bool aptInst     = isInstalledAPT(pkg);
        bool snapInst    = hasSnap    && isInstalledSnap(pkg);

        // For Flatpak: if pkg looks like an app-id use it directly,
        // otherwise search installed apps
        bool flatpakInst = false;
        vector<string> flatpakMatches;
        if (hasFlatpak) {
            if (isInstalledFlatpak(pkg)) {
                flatpakInst = true;
                flatpakMatches = {pkg};
            } else {
                flatpakMatches = getInstalledFlatpaks(pkg);
                flatpakInst    = !flatpakMatches.empty();
            }
        }

        string source = chooseRemoveMenu(pkg, aptInst, snapInst, flatpakInst,
                                         hasSnap, hasFlatpak);

        if (source == "apt") {
            targets.push_back({pkg, false, false, purge});
        } else if (source == "snap") {
            targets.push_back({pkg, false, true, false});
        } else if (source == "flatpak") {
            // May need to pick from multiple matching app-ids
            vector<string> toRemove;
            if (flatpakMatches.size() == 1) {
                toRemove = flatpakMatches;
            } else {
                toRemove = chooseFlatpakToRemove(flatpakMatches, pkg);
            }
            for (const auto& fp : toRemove)
                targets.push_back({fp, true, false, false});
        }
        // source == "" → skip
    }

    if (targets.empty()) {
        cout << YELLOW << "No packages selected.\n" << RESET;
        return 0;
    }

    cout << "\n" << RED << "Auto mode: APT/Flatpak/Snap per package\n" << RESET;
    cout << "Removing packages...\n\n";

    // ── removal loop ──────────────────────────────────────────────────────────
    vector<PackageResult> results;
    bool anyFailed = false;
    int  total     = static_cast<int>(targets.size());

    for (int i = 0; i < total; ++i) {
        if (g_interrupted) {
            cout << "\n" << YELLOW << "Cancelled by user (Ctrl+C).\n" << RESET;
            return 130;
        }

        const string& p         = targets[i].name;
        float         startRange = (100.0f * i)       / total;
        float         endRange   = (100.0f * (i + 1)) / total;

        PackageResult res; res.name = p;

        if (targets[i].useFlatpak) {
            int st = runFlatpakRemoveWithProgress(p, startRange, endRange);
            if (st == 130) { cout << "\n" << YELLOW << "Cancelled.\n" << RESET; return 130; }
            // Don't trust exit code — flatpak exits 1 on dbus-launch errors even
            // when removal succeeded. Verify by checking installation status.
            bool stillInstalled = isInstalledFlatpak(p);
            drawGlobalBar(endRange, "Flatpak " + p + (!stillInstalled ? ": done" : ": failed"));
            if (!stillInstalled) {
                res.message = GREEN + "Package " + p + " removed successfully." + RESET;
                res.success = true;
            } else {
                res.message = RED + "Package " + p + " removal failed." + RESET;
                anyFailed   = true;
            }
        } else if (targets[i].useSnap) {
            int st = runSnapRemoveWithProgress(p, startRange, endRange);
            if (st == 130) { cout << "\n" << YELLOW << "Cancelled.\n" << RESET; return 130; }
            drawGlobalBar(endRange, "Snap " + p + (st == 0 ? ": done" : ": failed"));
            if (st == 0) {
                res.message = GREEN + "Package " + p + " removed successfully." + RESET;
                res.success = true;
            } else {
                res.message = RED + "Package " + p + " removal failed." + RESET;
                anyFailed   = true;
            }
        } else {
            int st = runAPTRemoveWithProgress(p, targets[i].purge, startRange, endRange);
            if (st == 130) { cout << "\n" << YELLOW << "Cancelled.\n" << RESET; return 130; }
            drawGlobalBar(endRange, "APT " + p + (st == 0 ? ": done" : ": failed"));
            if (st == 0) {
                string op = targets[i].purge ? "purged" : "removed";
                res.message = GREEN + "Package " + p + " " + op + " successfully." + RESET;
                res.success = true;
            } else {
                res.message = RED + "Package " + p + " removal failed." + RESET;
                anyFailed   = true;
            }
        }

        results.push_back(res);
    }

    drawGlobalBar(100, "Done!");
    cout << "\n\n";

    for (const auto& r : results) cout << r.message << "\n";

    if (anyFailed) {
        cout << RED    << "Removal finished with errors!\n" << RESET;
        cout << YELLOW << "Check " << LOG_PATH << " for details.\n" << RESET;
        return 1;
    }

    cout << GREEN << "Removal complete!\n" << RESET;
    return 0;
}