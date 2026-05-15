#include "main.h"


using namespace std;


const string LOG_PATH = "/tmp/zinst.log";

volatile sig_atomic_t g_interrupted = 0;

struct PackageResult {
    string name;
    string message;
    bool   success = false;
};

struct InstallTarget {
    string name;
    bool   useFlatpak = false;
    bool   useSnap    = false;
};

// ─── signal ──────────────────────────────────────────────────────────────────
void handleSigint(int) { g_interrupted = 1; }

// ─── progress bar ─────────────────────────────────────────────────────────────
void drawGlobalBar(float totalProgress, const string& task) {
    struct winsize w;
    int termWidth = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
        termWidth = w.ws_col;

    const int barWidth = max(10, min(40, termWidth / 3));
    const int visualPrefixLen = 28 + barWidth;
    const int taskMaxLen = max(1, termWidth - visualPrefixLen);
    

    string taskTrimmed = task;
    taskTrimmed.erase(remove(taskTrimmed.begin(), taskTrimmed.end(), '\n'), taskTrimmed.end());
    if ((int)taskTrimmed.size() > taskMaxLen)
        taskTrimmed = taskTrimmed.substr(0, taskMaxLen - 1) + "~";

    int percent = max(0, min(100, (int)totalProgress));
    int pos = barWidth * percent / 100;

    cout << "\r\033[K" << YELLOW << "Install Progress: [" << RESET;
    for (int i = 0; i < barWidth; ++i)
        cout << (i < pos ? GREEN + "#" + RESET : " ");
        cout << YELLOW << "] " << setw(3) << percent << "% " << RESET << "| " << taskTrimmed << "\033[K" << flush;
}

static string stripAnsi(const string& in) {
    string out;
    out.reserve(in.size());
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
    // collapse multiple spaces/tabs into one
    string collapsed;
    bool prevSpace = true;
    for (char c : s) {
        if (c == ' ' || c == '\t') {
            if (!prevSpace) { collapsed += ' '; prevSpace = true; }
        } else {
            collapsed += c;
            prevSpace = false;
        }
    }
    while (!collapsed.empty() && collapsed.back() == ' ') collapsed.pop_back();
    if (collapsed.size() > maxLen) collapsed = collapsed.substr(0, maxLen);
    return collapsed.empty() ? "Working..." : collapsed;
}

// ─── detection helpers ────────────────────────────────────────────────────────
bool isInstalledAPT(const string& pkg) {
    string cmd = "dpkg-query -W -f='${Status}' " + pkg + " 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return false;
    char buffer[128]; string status;
    if (fgets(buffer, sizeof(buffer), pipe)) status = buffer;
    pclose(pipe);
    return status.find("install ok installed") != string::npos;
}

bool isInstalledFlatpak(const string& pkg) {
    string cmd = "flatpak list --columns=application | grep -Fx \"" + pkg + "\" >/dev/null 2>&1";
    return system(cmd.c_str()) == 0;
}

bool isInstalledSnap(const string& pkg) {
    string cmd = "snap list " + pkg + " >/dev/null 2>&1";
    return system(cmd.c_str()) == 0;
}

bool aptPackageExists(const string& pkg) {
    // apt-cache show returns 0 even for virtual packages with no install candidate.
    // A dry-run install is the only reliable check.
    string cmd = "apt-get -y --simulate install " + pkg + " >/dev/null 2>&1";
    return system(cmd.c_str()) == 0;
}

bool snapPackageExists(const string& pkg) {
    return system(("snap info " + pkg + " >/dev/null 2>&1").c_str()) == 0;
}

// Returns all Flatpak application IDs matching query (case-insensitive substring)
vector<string> searchFlatpak(const string& query) {
    vector<string> results;
    string cmd = "flatpak search --columns=application \"" + query + "\" 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return results;

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        string line = buffer;
        line.erase(line.find_last_not_of(" \n\r\t") + 1);
        if (!line.empty()) results.push_back(line);
    }
    pclose(pipe);

    // filter by substring
    string qLower = query;
    transform(qLower.begin(), qLower.end(), qLower.begin(), ::tolower);
    vector<string> filtered;
    for (const auto& pkg : results) {
        string p = pkg;
        transform(p.begin(), p.end(), p.begin(), ::tolower);
        if (p.find(qLower) != string::npos) filtered.push_back(pkg);
    }
    sort(filtered.begin(), filtered.end());
    return filtered;
}

// ─── NEW: pretty source-selection menu ────────────────────────────────────────
//
//  Package: mc
//    1. APT:     exist (mc)
//    2. Snap:    none
//    3. Flatpak: exist (29 results)
//    0. Skip
//  Choose: _
//
// Returns one of: "apt", "flatpak", "snap", or "" (skip / no source)
string chooseSourceMenu(const string& pkg,
                        bool aptAvail,
                        bool snapAvail,
                        const vector<string>& flatpakResults,
                        bool snapSystemAvail,
                        bool flatpakSystemAvail) {

    bool flatpakAvail = !flatpakResults.empty();
    int  flatpakCount = static_cast<int>(flatpakResults.size());

    // Build option list in a fixed display order: APT → Snap → Flatpak
    struct Option { int num; string key; };
    vector<Option> options;
    int idx = 1;

    int aptNum     = -1;
    int snapNum    = -1;
    int flatpakNum = -1;

    // ── header ────────────────────────────────────────────────────────────────
    cout << "\n" << BOLD << "Package: " << CYAN << pkg << RESET << "\n";

    // APT
    cout << "  " << BOLD << idx << ". APT:     " << RESET;
    if (aptAvail) {
        cout << GREEN << "exist (" << pkg << ")" << RESET << "\n";
        aptNum = idx++;
        options.push_back({aptNum, "apt"});
    } else {
        cout << RED << "none" << RESET << "\n";
        idx++; // keep numbering stable (shown but not selectable)
    }

    // Snap — only show row if snap is installed on the system
    if (snapSystemAvail) {
        cout << "  " << BOLD << idx << ". Snap:    " << RESET;
        if (snapAvail) {
            cout << GREEN << "exist (" << pkg << ")" << RESET << "\n";
            snapNum = idx;
            options.push_back({snapNum, "snap"});
        } else {
            cout << RED << "none" << RESET << "\n";
        }
        idx++;
    }

    // Flatpak — only show row if flatpak is installed on the system
    if (flatpakSystemAvail) {
        cout << "  " << BOLD << idx << ". Flatpak: " << RESET;
        if (flatpakAvail) {
            cout << GREEN << "exist (" << flatpakCount << " result"
                 << (flatpakCount != 1 ? "s" : "") << ")" << RESET << "\n";
            flatpakNum = idx;
            options.push_back({flatpakNum, "flatpak"});
        } else {
            cout << RED << "none" << RESET << "\n";
        }
        idx++;
    }

    cout << "  " << BOLD << "0. Skip" << RESET << "\n";

    if (options.empty()) {
        cout << YELLOW << "No source available for '" << pkg << "'." << RESET << "\n";
        return "";
    }

    // ── prompt ────────────────────────────────────────────────────────────────
    while (true) {
        cout << BOLD << "Choose: " << RESET;
        string input;
        if (!getline(cin, input)) return "";   // EOF
        int choice = -1;
        try { choice = stoi(input); } catch (...) {}

        if (choice == 0) return "";

        for (const auto& o : options)
            if (o.num == choice) return o.key;

        cout << RED << "Invalid choice, try again.\n" << RESET;
    }
}

// ─── sub-menu: pick one Flatpak app-id from list ──────────────────────────────
string chooseFlatpakPackage(const vector<string>& packages, const string& query) {
    if (packages.empty()) {
        cout << YELLOW << "No Flatpak packages found for '" << query << "'.\n" << RESET;
        return "";
    }
    cout << GREEN << "\nFlatpak results for '" << query << "':\n" << RESET;
    for (size_t i = 0; i < packages.size(); ++i)
        cout << "  " << (i + 1) << ". " << packages[i] << "\n";
    cout << "  0. Cancel\n" << BOLD << "Choose: " << RESET;

    string input;
    if (!getline(cin, input)) return "";
    int choice = -1;
    try { choice = stoi(input); } catch (...) {}
    if (choice == 0) return "";
    if (choice >= 1 && choice <= static_cast<int>(packages.size()))
        return packages[choice - 1];

    cout << RED << "Invalid choice!\n" << RESET;
    return "";
}


// ─── progress renderer thread ────────────────────────────────────────────────
// Redraws the bar every 80ms so it appears immediately even when the pipe
// has no data yet (e.g. APT's "Reading package lists..." phase).
struct BarState {
    atomic<float>  progress{0.0f};
    atomic<bool>   running{true};
    string         label;          // written once before thread starts — no race
};

static void* barThread(void* arg) {
    BarState* s = static_cast<BarState*>(arg);
    while (s->running.load()) {
        drawGlobalBar(s->progress.load(), s->label);
        usleep(80000); // 80ms
    }
    return nullptr;
}

// ─── installers ───────────────────────────────────────────────────────────────

// APT: use Status-Fd=3 — real percent from dlstatus/pmstatus lines
int runAPTInstallWithProgress(const string& pkg, float startRange, float endRange) {
    string exitFile = "/tmp/zinst_apt_exit_" + to_string(getpid());

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
        execlp("apt-get", "apt-get", "-y", "-o", "APT::Status-Fd=3",
               "install", pkg.c_str(), nullptr);
        _exit(127);
    }
    if (pid < 0) { close(pfd[0]); close(pfd[1]); return 1; }
    close(pfd[1]);

    // Start render thread — shows bar immediately while pipe is silent
    BarState bs;
    bs.progress = startRange;
    bs.label    = "APT " + pkg + ": preparing...";
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
        if (line.rfind("dlstatus:", 0) == 0 || line.rfind("pmstatus:", 0) == 0) {
            size_t c1 = line.find(':');
            size_t c2 = line.find(':', c1 + 1);
            size_t c3 = line.find(':', c2 + 1);
            if (c3 != string::npos) {
                float pct = 0.0f;
                try { pct = stof(line.substr(c2 + 1, c3 - c2 - 1)); } catch (...) {}
                float gp = startRange + (pct / 100.0f) * (endRange - startRange);
                if (gp > bs.progress.load()) bs.progress = gp;
                bs.label = "APT: " + trimForTask(line.substr(c3 + 1));
            }
        }
    }
    if (f) fclose(f);

    bs.running = false;
    pthread_join(tid, nullptr);

    int wst = 0;
    waitpid(pid, &wst, 0);
    return WIFEXITED(wst) ? WEXITSTATUS(wst) : 1;
}

// Flatpak: parse "XX%" from output lines
// Flatpak outputs NO progress lines during download — only start/end messages.
// We run it in background and animate the bar with a time-based curve.
// Bar grows fast at start, slows down, caps at 95% until process finishes.
int runFlatpakInstallWithProgress(const string& pkg, float startRange, float endRange) {
    string exitFile = "/tmp/zinst_fp_exit_" + to_string(getpid());
    string cmd = "flatpak install -y --noninteractive flathub "
                 + pkg + " >> " + LOG_PATH + " 2>&1; echo $? > " + exitFile;

    pid_t pid = fork();
    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        _exit(127);
    }
    if (pid < 0) return 1;

    float range = endRange - startRange;
    // Animate: progress = startRange + range * (1 - 1/(1 + t/T))
    // where T is "half-life" in seconds. At T seconds: 50%, at 3T: 75%, etc.
    // We never reach endRange — only jump to it when process finishes.
    // With T=20s: 10s→33%, 20s→50%, 40s→67%, 60s→75%, 120s→86%
    const float T = 20.0f; // tune this — larger = slower fill
    auto timeNow = []() -> float {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts.tv_sec + ts.tv_nsec * 1e-9f;
    };
    float t0 = timeNow();

    while (true) {
        if (g_interrupted) { kill(pid, SIGTERM); waitpid(pid, nullptr, 0); return 130; }
        if (waitpid(pid, nullptr, WNOHANG) == pid) break;
        usleep(100000); // 100ms
        float t = timeNow() - t0;
        float frac = 1.0f - 1.0f / (1.0f + t / T); // hyperbolic, never reaches 1
        float gp = startRange + range * frac * 0.95f; // cap at 95% of range
        drawGlobalBar(gp, "Flatpak " + pkg + ": installing...");
    }

    int exitCode = 1;
    { ifstream ef(exitFile); if (ef) ef >> exitCode; remove(exitFile.c_str()); }
    return exitCode;
}

// Snap: parse "CUR MB / TOT MB" → percent; fallback to line-tick
int runSnapInstallWithProgress(const string& pkg, float startRange, float endRange) {
    string exitFile = "/tmp/zinst_snap_exit_" + to_string(getpid());
    string cmd = "snap install " + pkg + " 2>&1; echo $? > " + exitFile;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return 1;
    setvbuf(pipe, nullptr, _IONBF, 0);

    BarState bs;
    bs.progress = startRange;
    bs.label    = "Snap " + pkg + ": preparing...";
    pthread_t tid;
    pthread_create(&tid, nullptr, barThread, &bs);

    char  buf[512];
    int   ticks = 0;
    while (fgets(buf, sizeof(buf), pipe)) {
        if (g_interrupted) {
            bs.running = false; pthread_join(tid, nullptr);
            pclose(pipe); return 130;
        }
        string line  = buf;
        string clean = stripAnsi(line);
        { ofstream log(LOG_PATH, ios::app); log << clean << "\n"; }

        size_t sep = clean.find(" / ");
        float  cur = 0.0f, tot = 0.0f;
        if (sep != string::npos) {
            int k = (int)sep - 1;
            while (k >= 0 && !isdigit((unsigned char)clean[k])) k--;
            string n1;
            while (k >= 0 && (isdigit((unsigned char)clean[k]) || clean[k] == '.'))
                n1 = clean[k--] + n1;
            size_t m = sep + 3;
            while (m < clean.size() && !isdigit((unsigned char)clean[m])) m++;
            string n2;
            while (m < clean.size() && (isdigit((unsigned char)clean[m]) || clean[m] == '.'))
                n2 += clean[m++];
            if (!n1.empty() && !n2.empty()) {
                try { cur = stof(n1); tot = stof(n2); } catch (...) {}
            }
        }
        float gp;
        if (tot > 0.0f) {
            gp = startRange + (cur / tot) * (endRange - startRange);
        } else {
            ticks++;
            gp = startRange + (endRange - startRange)
                 * (1.0f - expf(-(float)ticks / 8.0f)) * 0.9f;
        }
        if (gp > bs.progress.load()) bs.progress = gp;
        bs.label = "Snap: " + trimForTask(clean);
    }
    pclose(pipe);

    bs.running = false;
    pthread_join(tid, nullptr);

    int exitCode = 1;
    { ifstream ef(exitFile); if (ef) ef >> exitCode; remove(exitFile.c_str()); }
    return exitCode;
}

// ─── main ─────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    zpm_update::checkForUpdates();
    signal(SIGINT, handleSigint);
    // Disable stdout buffering so progress bar updates appear immediately
    setvbuf(stdout, nullptr, _IONBF, 0);

    bool           showHelp       = false;
    bool           showVersion    = false;
    bool           useTestPackage = false;
    bool           y = false;
    vector<string> packages;

    bool hasFlatpak = (system("command -v flatpak >/dev/null 2>&1") == 0);
    bool hasSnap    = (system("command -v snap    >/dev/null 2>&1") == 0);

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if      (arg == "--help"    || arg == "-h") showHelp = true;
        else if (arg == "--version" || arg == "-v") showVersion = true;
        else if (arg == "--dry-run")                useTestPackage = true;
        else if (arg == "--yes" || arg =="-y") y = true;
        else packages.push_back(arg);
    }

    if(showVersion && showHelp){
        cout << YELLOW << "--version" << RESET << endl;
        cout << RED << "zinst component version: 1.2 of ZPM\n" << RESET;
        cout << "https://github.com/Ignacyyy/ZPM\n";
        cout << "Copyright (c) 2026 Ignacyyy\nLicense: MIT\n";
        cout << "" << endl;
        cout << YELLOW <<"--help" << RESET << endl;
        cout << RED << "Usage: " << RESET << argv[0] << " [options] [packages...]" << " or zpm inst/install [options] [packages...]"  "\n\n";
        cout << RED << "Options:\n" << RESET;
        cout << "  (auto)         Picks APT / Flatpak / Snap per package\n";
        cout << "  --version, -v  Show version information\n";
        cout << "  --help,    -h  Show this help message\n";
        return 0;
       
    }

    if (showVersion) {
        cout << RED << "zinst component version: 1.2 of ZPM\n" << RESET;
        cout << "https://github.com/Ignacyyy/ZPM\n";
        cout << "Copyright (c) 2026 Ignacyyy\nLicense: MIT\n";
        return 0;
    }

    if (showHelp) {
        cout << RED << "Usage: " << RESET << argv[0] << " [options] [packages...]" << " or zpm inst/install [options] [packages...]"  "\n\n";
        cout << RED << "Options:\n" << RESET;
        cout << "  (auto)         Picks APT / Flatpak / Snap per package\n";
        cout << "  --version, -v  Show version information\n";
        cout << "  --help,    -h  Show this help message\n";
        return 0;
    }

    if (geteuid() != 0) {
        cout << RED << "Run with sudo!\n" << RESET;
        return 1;
    }

    if (useTestPackage) packages.push_back("sl");

    if (packages.empty()) {
        cout << YELLOW << "No package specified!\n" << RESET;
        return 1;
    }

    // ── resolve each package to an InstallTarget ──────────────────────────────
    vector<InstallTarget> targets;

    for (const string& pkg : packages) {
        bool           aptAvail     = aptPackageExists(pkg);
        bool           snapAvail    = hasSnap    && snapPackageExists(pkg);
        vector<string> flatpakFound = hasFlatpak ? searchFlatpak(pkg) : vector<string>{};

        string source = chooseSourceMenu(pkg, aptAvail, snapAvail, flatpakFound, hasSnap, hasFlatpak);

        if (source == "apt") {
            targets.push_back({pkg, false, false});
        } else if (source == "snap") {
            targets.push_back({pkg, false, true});
        } else if (source == "flatpak") {
            // If there's an exact match use it directly, else let user pick
            bool exactMatch = false;
            for (const auto& c : flatpakFound)
                if (c == pkg) { exactMatch = true; break; }

            string selected = exactMatch ? pkg : chooseFlatpakPackage(flatpakFound, pkg);
            if (!selected.empty()) targets.push_back({selected, true, false});
        }
        // source == "" → skip
    }

    if (targets.empty()) {
        cout << YELLOW << "No packages selected.\n" << RESET;
        return 0;
    }

    cout << "\n" << RED << "Auto mode: APT/Flatpak/Snap per package\n" << RESET;
    cout << "Installing packages...\n\n";

    // ── install loop ──────────────────────────────────────────────────────────
    vector<PackageResult> results;
    bool anyFailed    = false;
    int  totalPkgs    = static_cast<int>(targets.size());

    for (int i = 0; i < totalPkgs; ++i) {
        if (g_interrupted) {
            cout << "\n" << YELLOW << "Cancelled by user (Ctrl+C).\n" << RESET;
            return 130;
        }

        const string& p         = targets[i].name;
        float         startRange = (100.0f * i)       / totalPkgs;
        float         endRange   = (100.0f * (i + 1)) / totalPkgs;

        PackageResult res; res.name = p;

        // ── Flatpak ───────────────────────────────────────────────────────────
        if (targets[i].useFlatpak) {
            if (isInstalledFlatpak(p)) {
                drawGlobalBar(endRange, "Flatpak " + p + ": already installed");
                res.message = YELLOW + "Package " + p + " is already installed." + RESET;
                res.success = true;
            } else {
                int st = runFlatpakInstallWithProgress(p, startRange, endRange);
                if (st == 130) { cout << "\n" << YELLOW << "Cancelled.\n" << RESET; return 130; }
                if (st == 0) {
                    drawGlobalBar(endRange, "Flatpak " + p + ": done");
                    res.message = GREEN + "Package " + p + " installed successfully." + RESET;
                    res.success = true;
                } else {
                    res.message = RED + "Package " + p + " installation failed." + RESET;
                    anyFailed   = true;
                }
            }
        }
        // ── Snap ──────────────────────────────────────────────────────────────
        else if (targets[i].useSnap) {
            if (isInstalledSnap(p)) {
                drawGlobalBar(endRange, "Snap " + p + ": already installed");
                res.message = YELLOW + "Package " + p + " is already installed." + RESET;
                res.success = true;
            } else {
                int st = runSnapInstallWithProgress(p, startRange, endRange);
                if (st == 130) { cout << "\n" << YELLOW << "Cancelled.\n" << RESET; return 130; }
                if (st == 0) {
                    drawGlobalBar(endRange, "Snap " + p + ": done");
                    res.message = GREEN + "Package " + p + " installed successfully." + RESET;
                    res.success = true;
                } else {
                    res.message = RED + "Package " + p + " installation failed." + RESET;
                    anyFailed   = true;
                }
            }
        }
        // ── APT ───────────────────────────────────────────────────────────────
        else {
            if (isInstalledAPT(p)) {
                drawGlobalBar(endRange, "APT " + p + ": already installed");
                res.message = YELLOW + "Package " + p + " is already installed." + RESET;
                res.success = true;
            } else {
                int st = runAPTInstallWithProgress(p, startRange, endRange);
                if (st == 130) { cout << "\n" << YELLOW << "Cancelled.\n" << RESET; return 130; }
                if (st == 0) {
                    drawGlobalBar(endRange, "APT " + p + ": done");
                    res.message = GREEN + "Package " + p + " installed successfully." + RESET;
                    res.success = true;
                } else {
                    res.message = RED + "Package " + p + " installation failed." + RESET;
                    anyFailed   = true;
                }
            }
        }

        results.push_back(res);
    }

    drawGlobalBar(100, "Done!");
    cout << "\n\n";

    for (const auto& r : results) cout << r.message << "\n";

    if (anyFailed) {
        cout << RED    << "Installation finished with errors!\n" << RESET;
        cout << YELLOW << "Check " << LOG_PATH << " for details.\n" << RESET;
        return 1;
    }

    cout << GREEN << "Installation complete!\n" << RESET;
    return 0;
}
