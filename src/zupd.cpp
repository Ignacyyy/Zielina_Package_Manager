#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <csignal>
#include <sys/wait.h>

//colors
#include "/opt/ZPM/src/common/colors.h"

using namespace std;

const string LOG_PATH = "/tmp/zupd.log";
string ans;

struct StageStatus {
    bool ok = false;
    int exitCode = -1;
    string details;
};

struct AptRunResult {
    bool ok = false;
    int exitCode = -1;
    bool sawStructuredProgress = false;
    string lastTask = "Working...";
};

struct StageRange {
    float start = 0.0f;
    float end = 0.0f;
};

volatile sig_atomic_t g_interrupted = 0;
bool g_cancelledByUser = false;

static string trimForTask(const string& line, size_t maxLen = 44);

void handleSigint(int) {
    g_interrupted = 1;
}

bool wasInterruptedBySigint(int status) {
    if (status == 130) return true;
    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT) return true;
    if (WIFEXITED(status) && WEXITSTATUS(status) == 130) return true;
    return false;
}

void drawGlobalBar(float totalProgress, string task) {
    int width = 40;
    int pos = width * (totalProgress / 100.0);

    cout << "\r\033[K" << YELLOW << "Update Progress: [" << RESET;
    for (int i = 0; i < width; ++i) {
        if (i < pos) cout << GREEN << "#" << RESET;
        else cout << " ";
    }

    int percent = (int)totalProgress;
    if (percent > 100) percent = 100;
    if (percent < 0) percent = 0;
    cout << YELLOW << "] " << percent << "% " << RESET << "| " << task << flush;
}

int countCommandLines(const string& cmd) {
    FILE* pipe = popen((cmd + " 2>/dev/null").c_str(), "r");
    if (!pipe) return 0;
    int count = 0;
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        string line = buffer;
        if (!trimForTask(line).empty()) count++;
    }
    pclose(pipe);
    return count;
}

vector<string> collectCommandLines(const string& cmd) {
    vector<string> lines;
    FILE* pipe = popen((cmd + " 2>/dev/null").c_str(), "r");
    if (!pipe) return lines;
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        string line = trimForTask(buffer);
        if (!line.empty()) lines.push_back(line);
    }
    pclose(pipe);
    return lines;
}

vector<StageRange> buildStageRanges(bool hasFlatpak, bool hasSnap, int aptCount, int flatpakCount, int snapCount) {
    const float globalStart = 1.0f;
    const float globalEnd = 98.0f;
    const float refreshWeight = 8.0f;
    const float aptWeight = max(24.0f, static_cast<float>(aptCount) * 6.0f);
    const float flatpakWeight = hasFlatpak ? max(8.0f, static_cast<float>(flatpakCount) * 4.0f) : 0.0f;
    const float snapWeight = hasSnap ? max(8.0f, static_cast<float>(snapCount) * 4.0f) : 0.0f;
    const float totalWeight = refreshWeight + aptWeight + flatpakWeight + snapWeight;

    float cursor = globalStart;
    vector<StageRange> ranges(4);
    auto advance = [&](float weight) {
        float span = (weight / totalWeight) * (globalEnd - globalStart);
        StageRange r{cursor, cursor + span};
        cursor += span;
        return r;
    };

    ranges[0] = advance(refreshWeight);          // refresh
    ranges[1] = advance(aptWeight);              // apt
    ranges[2] = hasFlatpak ? advance(flatpakWeight) : StageRange{ranges[1].end, ranges[1].end};
    ranges[3] = hasSnap ? advance(snapWeight) : StageRange{ranges[2].end, ranges[2].end};
    return ranges;
}

bool runCommandWithEstimatedProgress(const string& cmd, float startRange, float endRange, const string& stageLabel,
                                     StageStatus& stage, int expectedItems) {
    FILE* pipe = popen((cmd + " 2>&1").c_str(), "r");
    if (!pipe) {
        stage.ok = false;
        stage.details = "Unable to start command: " + stageLabel;
        return false;
    }

    char buffer[512];
    int steps = 0;
    int stepGoal = max(1, expectedItems * 2);
    float lastProgress = startRange;
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        if (g_interrupted) {
            stage.ok = false;
            stage.exitCode = 130;
            stage.details = stageLabel + " interrupted by user.";
            pclose(pipe);
            return false;
        }

        string line = buffer;
        ofstream log(LOG_PATH, ios::app);
        log << line;

        string t = trimForTask(line);
        bool meaningful = (t.find("error") != string::npos || t.find("Error") != string::npos ||
                           t.find("Updating") != string::npos || t.find("Installing") != string::npos ||
                           t.find("refresh") != string::npos || t.find("available") != string::npos ||
                           t.find("updating") != string::npos || t.find("done") != string::npos);
        if (!meaningful && !t.empty()) {
            // Still move slowly for unrecognized but real output.
            meaningful = true;
        }
        if (meaningful) {
            steps++;
            float ratio = min(0.97f, static_cast<float>(steps) / static_cast<float>(stepGoal));
            float gp = startRange + ratio * (endRange - startRange);
            if (gp < lastProgress) gp = lastProgress;
            lastProgress = gp;
            drawGlobalBar(gp, stageLabel + ": " + t);
        }
    }

    int rc = pclose(pipe);
    stage.exitCode = rc;
    stage.ok = (rc == 0);
    stage.details = stage.ok ? (stageLabel + " completed.") : (stageLabel + " failed.");
    if (stage.ok) drawGlobalBar(endRange, stageLabel + " done.");
    return stage.ok;
}

static vector<string> splitByColon(const string& line) {
    vector<string> parts;
    string part;
    stringstream ss(line);
    while (getline(ss, part, ':')) {
        parts.push_back(part);
    }
    return parts;
}

static bool parsePmStatus(const string& line, float& aptPercent, string& pkgMsg) {
    if (line.rfind("pmstatus:", 0) != 0) return false;
    vector<string> parts = splitByColon(line);
    if (parts.size() < 4) return false;
    try {
        aptPercent = stof(parts[2]);
        if (aptPercent < 0.0f) aptPercent = 0.0f;
        if (aptPercent > 100.0f) aptPercent = 100.0f;
    } catch (...) {
        return false;
    }
    pkgMsg = parts[3];
    for (size_t i = 4; i < parts.size(); ++i) pkgMsg += ":" + parts[i];
    pkgMsg.erase(remove(pkgMsg.begin(), pkgMsg.end(), '\n'), pkgMsg.end());
    if (pkgMsg.empty()) pkgMsg = "Applying packages...";
    return true;
}

static float fallbackAptProgressByMessage(const string& line) {
    if (line.find("Unpacking") != string::npos) return 45.0f;
    if (line.find("Setting up") != string::npos) return 75.0f;
    if (line.find("Processing triggers for") != string::npos) return 92.0f;
    if (line.find("Reading package lists") != string::npos) return 12.0f;
    if (line.find("Building dependency tree") != string::npos) return 22.0f;
    if (line.find("Calculating upgrade") != string::npos) return 30.0f;
    return -1.0f;
}

static string trimForTask(const string& line, size_t maxLen) {
    string out = line;
    out.erase(remove(out.begin(), out.end(), '\n'), out.end());
    if (out.size() > maxLen) out = out.substr(0, maxLen);
    if (out.empty()) out = "Working...";
    return out;
}

AptRunResult executeAptWithGlobalProgress(const string& aptCmd, float startRange, float endRange) {
    string cmd = "export DEBIAN_FRONTEND=noninteractive; apt-get " + aptCmd + " -y -o APT::Status-Fd=1 -o APT::Cmd::Show-Update-Stats=0 -o APT::Update::Post-Invoke-Success=\"\" 2>&1";

    AptRunResult result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        result.lastTask = "Unable to start APT process.";
        return result;
    }

    char buffer[512];
    float lastGlobal = startRange;
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        if (g_interrupted) {
            result.exitCode = 130;
            result.ok = false;
            result.lastTask = "Interrupted by user.";
            pclose(pipe);
            return result;
        }

        string line = buffer;
        ofstream log(LOG_PATH, ios::app);
        log << line;

        float aptPercent = 0.0f;
        string pkgMsg;
        if (parsePmStatus(line, aptPercent, pkgMsg)) {
            result.sawStructuredProgress = true;
            result.lastTask = trimForTask(pkgMsg);
            float globalPercent = startRange + (aptPercent / 100.0f) * (endRange - startRange);
            if (globalPercent < lastGlobal) globalPercent = lastGlobal;
            lastGlobal = globalPercent;
            drawGlobalBar(globalPercent, result.lastTask);
            continue;
        }

        float fallbackStage = fallbackAptProgressByMessage(line);
        if (fallbackStage >= 0.0f) {
            float globalPercent = startRange + (fallbackStage / 100.0f) * (endRange - startRange);
            if (globalPercent < lastGlobal) globalPercent = lastGlobal;
            lastGlobal = globalPercent;
            result.lastTask = trimForTask(line);
            drawGlobalBar(globalPercent, result.lastTask);
        }
    }
    int rc = pclose(pipe);
    result.exitCode = rc;
    result.ok = (rc == 0);
    return result;
}

bool runDryRunPreview(const string& mode, float startRange, float endRange, StageStatus& stage) {
    string cmd = "apt-get -s " + mode + " 2>&1";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        stage.ok = false;
        stage.details = "Unable to start apt-get -s.";
        return false;
    }

    vector<string> instLines;
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        if (g_interrupted) {
            stage.ok = false;
            stage.exitCode = 130;
            stage.details = "Dry-run interrupted by user.";
            pclose(pipe);
            return false;
        }

        string line = buffer;
        ofstream log(LOG_PATH, ios::app);
        log << line;
        if (line.rfind("Inst ", 0) == 0) {
            instLines.push_back(trimForTask(line));
        }
    }

    int rc = pclose(pipe);
    stage.exitCode = rc;
    if (rc != 0) {
        stage.ok = false;
        stage.details = "apt-get -s failed.";
        return false;
    }

    if (instLines.empty()) {
        drawGlobalBar(endRange, "Dry-run complete: no packages to upgrade.");
    } else {
        for (size_t i = 0; i < instLines.size(); ++i) {
            float ratio = static_cast<float>(i + 1) / static_cast<float>(instLines.size());
            float globalPercent = startRange + ratio * (endRange - startRange);
            drawGlobalBar(globalPercent, "DRY-RUN " + instLines[i]);
            usleep(60000);
        }
    }
    stage.ok = true;
    stage.details = "Dry-run simulation finished.";
    return true;
}

bool runMockDryRunPreview(float startRange, float endRange, StageStatus& stage) {
    vector<string> mockPkgs = {
        "Inst zpm-core [1.8.2] (1.9.0 Debian:stable [amd64])",
        "Inst zpm-ui [0.14.7] (0.15.1 Debian:stable [amd64])",
        "Inst apt-wrapper [2.3.0] (2.3.2 Debian:stable [amd64])",
        "Inst zpm-notifier [1.1.9] (1.2.0 Debian:stable [amd64])",
        "Inst zpm-plugins [3.0.4] (3.1.0 Debian:stable [amd64])"
    };

    for (size_t i = 0; i < mockPkgs.size(); ++i) {
        if (g_interrupted) {
            stage.ok = false;
            stage.exitCode = 130;
            stage.details = "Dry-run interrupted by user.";
            return false;
        }

        float ratio = static_cast<float>(i + 1) / static_cast<float>(mockPkgs.size());
        float globalPercent = startRange + ratio * (endRange - startRange);
        drawGlobalBar(globalPercent, "DRY-RUN " + trimForTask(mockPkgs[i]));
        ofstream log(LOG_PATH, ios::app);
        log << mockPkgs[i] << "\n";
        usleep(110000);
    }
    stage.ok = true;
    stage.exitCode = 0;
    stage.details = "Mock dry-run simulation finished.";
    return true;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, handleSigint);

    bool Reboot = false, FullUpdate = false, Shutdown = false, updateSuccessful = false; bool version = false; bool y = false;
    bool Help = false, DryRun = false;
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-full") FullUpdate = true;
        else if (arg == "-r") Reboot = true;
        else if (arg == "-s") Shutdown = true;
        else if (arg == "--help" || arg == "-h") Help = true;
        else if (arg == "--dry-run") DryRun = true; // hidden developer option
        else if (arg == "--version" || arg == "-v") version = true;
        else if (arg == "--yes" || arg == "-y") y = true;
    }

    if (Reboot && Shutdown || Help && Reboot || Help && Shutdown || version && y || Help && y || version && Reboot || version && Shutdown || FullUpdate && version || FullUpdate && Help) {
        cout << RED << "Error: -r and -s are mutually exclusive. " << endl; cout<< "--help and --version cannot be combined with other options." << RESET << endl;
        return 0;
    }

    if (version && Help){
        cout << YELLOW <<"--version" << RESET << endl;
        cout << RED << "zupd component version: 1.2 of ZPM" << RESET << endl;
        cout << "https://github.com/Ignacyyy/ZPM" << endl;
        cout << "Copyright (c) 2026 Ignacyyy" << endl;
        cout << "License: MIT" << endl;
        cout << "" << endl;
        cout << YELLOW << "--help" << RESET << endl;
        cout << RED << "Usage: " << RESET << argv[0] << " [options]" << " or zpm upd/update [options]"  "\n\n";
        cout << RED << "Options:" << RESET << endl;
        cout << "  -full     Perform a full system upgrade (dist-upgrade)" << endl;
        cout << "  -r        Reboot the system after update" << endl;
        cout << "  -s        Shutdown the system after update" << endl;
        cout << "  --yes, -y    Automatic system update" << endl;
        cout << "  --help, -h    Show this help message" << endl;
        cout << "  --version, -v    Show version information" << endl;
        return 0;
    }


    if (version) {
        cout << RED << "zupd component version: 1.2 of ZPM" << RESET << endl;
        cout << "https://github.com/Ignacyyy/ZPM" << endl;
        cout << "Copyright (c) 2026 Ignacyyy" << endl;
        cout << "License: MIT" << endl;
        return 0;
    }

    if (Help) {
        cout << RED << "Usage: " << RESET << argv[0] << " [options]" << " or zpm upd/update [options]"  "\n\n";
        cout << RED << "Options:" << RESET << endl;
        cout << "  -full     Perform a full system upgrade (dist-upgrade)" << endl;
        cout << "  -r        Reboot the system after update" << endl;
        cout << "  -s        Shutdown the system after update" << endl;
        cout << "  --yes, -y    automatic system update" << endl;
        cout << "  --help, -h    Show this help message" << endl;
        cout << "  --version, -v    Show version information" << endl;
        return 0;
    }

    if (geteuid() != 0) {
        cout << RED << "Run with sudo!\n" << RESET;
        return 1;
    }

    cout << YELLOW << "[SYS] " << RESET << flush;
    system("lsb_release -ds 2>/dev/null || cat /etc/debian_version");

    cout << "\n" << YELLOW << "[D]" << RESET << GREEN << " APT Repositories:\n" << RESET;
    string repoCmd =
    "{ "
    "grep -rh '^deb ' /etc/apt/sources.list /etc/apt/sources.list.d/ 2>/dev/null; "
    "grep -rh '^URIs:' /etc/apt/sources.list.d/ /usr/lib/apt/sources.list.d/ 2>/dev/null"
    "  | sed 's/^URIs:[[:space:]]*/deb /'; "
    "} | sort -u | grep -v '^[[:space:]]*$'"
    "  | sed 's|^|" + YELLOW + "- " + RESET + "|'";
system(repoCmd.c_str());
{
    string checkCmd =
        "{ grep -rh '^deb ' /etc/apt/sources.list /etc/apt/sources.list.d/ 2>/dev/null; "
        "grep -rh '^URIs:' /etc/apt/sources.list.d/ 2>/dev/null; }"
        " | grep -qv '^[[:space:]]*$'";
    if (system(checkCmd.c_str()) != 0)
        cout << YELLOW << "- (no repos found in standard locations)" << RESET << "\n";
}

    bool hasFlatpak = (system("command -v flatpak >/dev/null 2>&1") == 0);
    bool hasSnap = (system("command -v snap >/dev/null 2>&1") == 0);
    if (hasFlatpak) {
        cout << "\n" << YELLOW << "[F]" << RESET << GREEN << " Flatpak Remotes:\n" << RESET;
        system("flatpak remotes --columns=name 2>/dev/null | sed 's/^/\033[33m- \033[0m/'");
    }
    if (hasSnap) {
        cout << "\n" << YELLOW << "[S]" << RESET << GREEN << " Snap is available.\n" << RESET;
    }

    vector<string> aptUpgradable = DryRun ? vector<string>{
        "zpm-core", "zpm-ui", "apt-wrapper", "zpm-notifier", "zpm-plugins"
    } : collectCommandLines("apt-get -s upgrade | awk '/^Inst /{print $2}'");

    vector<string> flatpakUpgradable;
    if (hasFlatpak) {
        flatpakUpgradable = DryRun ? vector<string>{
            "org.mozilla.firefox", "org.videolan.VLC", "com.visualstudio.code"
        } : collectCommandLines("flatpak remote-ls --updates --columns=application");
    }

    vector<string> snapUpgradable;
    if (hasSnap) {
        snapUpgradable = DryRun ? vector<string>{
            "firefox", "snap-store", "chromium"
        } : collectCommandLines("snap refresh --list | sed -n '2,$p' | awk '{print $1}'");
    }

    int hasUpdates = aptUpgradable.empty() ? 1 : 0;

    if (hasUpdates != 0 && !FullUpdate && !DryRun) {
        cout << "\n" << RED << "System is up to date!" << RESET << endl;
    }
    else {
        cout << RED << "\nPackages to update (APT):\n" << RESET;
        for (const string& pkg : aptUpgradable) {
            cout << YELLOW << "[+] " << RESET << pkg << "\n";
        }
        if (aptUpgradable.empty()) {
            cout << YELLOW << "[i] No APT packages available for update.\n" << RESET;
        }

        if (hasFlatpak) {
            cout << RED << "\nPackages to update (Flatpak):\n" << RESET;
            if (DryRun) {
                for (const string& pkg : flatpakUpgradable) {
                    cout << YELLOW << "[+] " << RESET << pkg << "\n";
                }
            } else {
                for (const string& pkg : flatpakUpgradable) {
                    cout << YELLOW << "[+] " << RESET << pkg << "\n";
                }
            }
        }
        if (hasSnap) {
            cout << RED << "\nPackages to update (Snap):\n" << RESET;
            for (const string& pkg : snapUpgradable) {
                cout << YELLOW << "[+] " << RESET << pkg << "\n";
            }
            if (snapUpgradable.empty()) {
                cout << YELLOW << "[i] No Snap packages available for update.\n" << RESET;
            }
        }
        if (y)
        {
            cout << "\n";
            cout << "\n";
            cout << RED << "auto mode" << RESET; cout << " (-y/--yes)" << endl;
            StageStatus refreshStage, aptStage, flatpakStage, snapStage;

            vector<StageRange> ranges = buildStageRanges(hasFlatpak, hasSnap,
                                                        static_cast<int>(aptUpgradable.size()),
                                                        static_cast<int>(flatpakUpgradable.size()),
                                                        static_cast<int>(snapUpgradable.size()));
            const float refreshStart = ranges[0].start;
            const float refreshEnd = ranges[0].end;
            const float aptStart = ranges[1].start;
            const float aptEnd = ranges[1].end;
            const float flatpakStart = ranges[2].start;
            const float flatpakEnd = ranges[2].end;
            const float snapStart = ranges[3].start;
            const float snapEnd = ranges[3].end;

            // --- STAGE 1: REFRESH ---
            drawGlobalBar(refreshStart, DryRun ? "Dry-run: validating metadata..." : "Refreshing package lists...");
            int updateStatus = system("apt-get update -o APT::Update::Post-Invoke-Success=\"\" -o APT::Cmd::Show-Update-Stats=0 -qq >/dev/null 2>&1");
            refreshStage.exitCode = updateStatus;
            refreshStage.ok = (updateStatus == 0);
            refreshStage.details = refreshStage.ok ? "Package lists refreshed." : "apt-get update failed.";

            if (g_interrupted || wasInterruptedBySigint(updateStatus)) {
                g_cancelledByUser = true;
                cout << "\n" << YELLOW << "Operation cancelled by user (Ctrl+C)." << RESET << endl;
                return 130;
            }

            if (!refreshStage.ok) {
                cout << endl << RED << "CRITICAL: apt update failed! Operation aborted." << RESET << endl;
                cout << YELLOW << "Check " << LOG_PATH << " for details." << RESET << endl;
                return 1;
            }
            drawGlobalBar(refreshEnd, DryRun ? "Dry-run: metadata ready." : "Package lists refreshed.");

            // --- STAGE 2: APT UPGRADE ---
            string mode = FullUpdate ? "dist-upgrade" : "upgrade";
            bool aptOk = false;
            if (DryRun) {
                aptOk = runMockDryRunPreview(aptStart, aptEnd, aptStage);
            } else {
                AptRunResult aptResult = executeAptWithGlobalProgress(mode, aptStart, aptEnd);
                aptStage.ok = aptResult.ok;
                aptStage.exitCode = aptResult.exitCode;
                aptStage.details = aptResult.ok
                    ? (aptResult.sawStructuredProgress ? "Upgrade completed with structured progress." : "Upgrade completed using fallback parser.")
                    : ("Upgrade failed near: " + aptResult.lastTask);
                aptOk = aptResult.ok;
            }

            if (g_interrupted || aptStage.exitCode == 130) {
                g_cancelledByUser = true;
                cout << "\n" << YELLOW << "Operation cancelled by user (Ctrl+C)." << RESET << endl;
                return 130;
            }

            if (aptOk) {

                // --- STAGE 3: FLATPAK ---
                if (hasFlatpak) {
                    drawGlobalBar(flatpakStart, DryRun ? "Dry-run: simulating Flatpak updates..." : "Updating Flatpaks...");
                    bool flatpakOk = true;
                    if (DryRun) {
                        flatpakOk = runCommandWithEstimatedProgress("flatpak remote-ls --updates --columns=application",
                                                                    flatpakStart, flatpakEnd, "Flatpak dry-run",
                                                                    flatpakStage, static_cast<int>(flatpakUpgradable.size()));
                    } else {
                        flatpakOk = runCommandWithEstimatedProgress("flatpak update -y",
                                                                    flatpakStart, flatpakEnd, "Flatpak",
                                                                    flatpakStage, static_cast<int>(flatpakUpgradable.size()));
                    }
                    if (!flatpakOk && !DryRun) {
                        cout << "\n" << YELLOW << "Warning: Flatpak update encountered some issues." << RESET << endl;
                        cout << YELLOW << "Check " << LOG_PATH << " for details." << RESET << endl;
                    }
                    if (g_interrupted || flatpakStage.exitCode == 130) {
                        g_cancelledByUser = true;
                        cout << "\n" << YELLOW << "Operation cancelled by user (Ctrl+C)." << RESET << endl;
                        return 130;
                    }
                }

                // --- STAGE 4: SNAP ---
                if (hasSnap) {
                    drawGlobalBar(snapStart, DryRun ? "Dry-run: simulating Snap updates..." : "Updating Snaps...");
                    bool snapOk = true;
                    if (DryRun) {
                        snapOk = runCommandWithEstimatedProgress("snap refresh --list",
                                                                 snapStart, snapEnd, "Snap dry-run",
                                                                 snapStage, static_cast<int>(snapUpgradable.size()));
                    } else {
                        snapOk = runCommandWithEstimatedProgress("snap refresh",
                                                                 snapStart, snapEnd, "Snap",
                                                                 snapStage, static_cast<int>(snapUpgradable.size()));
                    }
                    if (!snapOk) {
                        cout << "\n" << YELLOW << "Warning: Snap stage encountered some issues." << RESET << endl;
                        cout << YELLOW << "Check " << LOG_PATH << " for details." << RESET << endl;
                    }
                    if (g_interrupted || snapStage.exitCode == 130) {
                        g_cancelledByUser = true;
                        cout << "\n" << YELLOW << "Operation cancelled by user (Ctrl+C)." << RESET << endl;
                        return 130;
                    }
                }

                drawGlobalBar(100, "Done!");
                cout << endl << "\n" << GREEN << (DryRun ? "Dry-run finished successfully!" : "Update successful!") << RESET << endl;
                cout << YELLOW << "\n[Report]\n" << RESET;
                cout << "- refresh: " << (refreshStage.ok ? "OK" : "FAIL") << " (code " << refreshStage.exitCode << ")\n";
                cout << "- apt: " << (aptStage.ok ? "OK" : "FAIL") << " (code " << aptStage.exitCode << ")\n";
                if (hasFlatpak) {
                    cout << "- flatpak: " << (flatpakStage.ok ? "OK" : "FAIL") << " (code " << flatpakStage.exitCode << ")\n";
                }
                if (hasSnap) {
                    cout << "- snap: " << (snapStage.ok ? "OK" : "FAIL") << " (code " << snapStage.exitCode << ")\n";
                }
                updateSuccessful = true;
            } else {
                cout << "\n" << RED << "CRITICAL: System upgrade failed! Check " << LOG_PATH << RESET << endl;
                cout << YELLOW << "Check " << LOG_PATH << " for details." << RESET << endl;
                cout << YELLOW << "[APT details] " << RESET << aptStage.details << "\n";
                cout << YELLOW << "[Exit code] " << RESET << aptStage.exitCode << "\n";
                return 0;
            }
            return 0;
        }
    
        cout << "\n" << YELLOW << "Proceed with update?" << RESET;
        cout << " [y/n]: " << RESET;
         cin >> ans;

        if (ans == "y" || ans == "Y" || ans == "yes" || ans == "Yes" || ans == "YES") {
            StageStatus refreshStage, aptStage, flatpakStage, snapStage;

            vector<StageRange> ranges = buildStageRanges(hasFlatpak, hasSnap,
                                                        static_cast<int>(aptUpgradable.size()),
                                                        static_cast<int>(flatpakUpgradable.size()),
                                                        static_cast<int>(snapUpgradable.size()));
            const float refreshStart = ranges[0].start;
            const float refreshEnd = ranges[0].end;
            const float aptStart = ranges[1].start;
            const float aptEnd = ranges[1].end;
            const float flatpakStart = ranges[2].start;
            const float flatpakEnd = ranges[2].end;
            const float snapStart = ranges[3].start;
            const float snapEnd = ranges[3].end;

            // --- STAGE 1: REFRESH ---
            drawGlobalBar(refreshStart, DryRun ? "Dry-run: validating metadata..." : "Refreshing package lists...");
            int updateStatus = system("apt-get update -o APT::Update::Post-Invoke-Success=\"\" -o APT::Cmd::Show-Update-Stats=0 -qq >/dev/null 2>&1");
            refreshStage.exitCode = updateStatus;
            refreshStage.ok = (updateStatus == 0);
            refreshStage.details = refreshStage.ok ? "Package lists refreshed." : "apt-get update failed.";

            if (g_interrupted || wasInterruptedBySigint(updateStatus)) {
                g_cancelledByUser = true;
                cout << "\n" << YELLOW << "Operation cancelled by user (Ctrl+C)." << RESET << endl;
                return 130;
            }

            if (!refreshStage.ok) {
                cout << endl << RED << "CRITICAL: apt update failed! Operation aborted." << RESET << endl;
                cout << YELLOW << "Check " << LOG_PATH << " for details." << RESET << endl;
                return 1;
            }
            drawGlobalBar(refreshEnd, DryRun ? "Dry-run: metadata ready." : "Package lists refreshed.");

            // --- STAGE 2: APT UPGRADE ---
            string mode = FullUpdate ? "dist-upgrade" : "upgrade";
            bool aptOk = false;
            if (DryRun) {
                aptOk = runMockDryRunPreview(aptStart, aptEnd, aptStage);
            } else {
                AptRunResult aptResult = executeAptWithGlobalProgress(mode, aptStart, aptEnd);
                aptStage.ok = aptResult.ok;
                aptStage.exitCode = aptResult.exitCode;
                aptStage.details = aptResult.ok
                    ? (aptResult.sawStructuredProgress ? "Upgrade completed with structured progress." : "Upgrade completed using fallback parser.")
                    : ("Upgrade failed near: " + aptResult.lastTask);
                aptOk = aptResult.ok;
            }

            if (g_interrupted || aptStage.exitCode == 130) {
                g_cancelledByUser = true;
                cout << "\n" << YELLOW << "Operation cancelled by user (Ctrl+C)." << RESET << endl;
                return 130;
            }

            if (aptOk) {

                // --- STAGE 3: FLATPAK ---
                if (hasFlatpak) {
                    drawGlobalBar(flatpakStart, DryRun ? "Dry-run: simulating Flatpak updates..." : "Updating Flatpaks...");
                    bool flatpakOk = true;
                    if (DryRun) {
                        flatpakOk = runCommandWithEstimatedProgress("flatpak remote-ls --updates --columns=application",
                                                                    flatpakStart, flatpakEnd, "Flatpak dry-run",
                                                                    flatpakStage, static_cast<int>(flatpakUpgradable.size()));
                    } else {
                        flatpakOk = runCommandWithEstimatedProgress("flatpak update -y",
                                                                    flatpakStart, flatpakEnd, "Flatpak",
                                                                    flatpakStage, static_cast<int>(flatpakUpgradable.size()));
                    }
                    if (!flatpakOk && !DryRun) {
                        cout << "\n" << YELLOW << "Warning: Flatpak update encountered some issues." << RESET << endl;
                        cout << YELLOW << "Check " << LOG_PATH << " for details." << RESET << endl;
                    }
                    if (g_interrupted || flatpakStage.exitCode == 130) {
                        g_cancelledByUser = true;
                        cout << "\n" << YELLOW << "Operation cancelled by user (Ctrl+C)." << RESET << endl;
                        return 130;
                    }
                }

                // --- STAGE 4: SNAP ---
                if (hasSnap) {
                    drawGlobalBar(snapStart, DryRun ? "Dry-run: simulating Snap updates..." : "Updating Snaps...");
                    bool snapOk = true;
                    if (DryRun) {
                        snapOk = runCommandWithEstimatedProgress("snap refresh --list",
                                                                 snapStart, snapEnd, "Snap dry-run",
                                                                 snapStage, static_cast<int>(snapUpgradable.size()));
                    } else {
                        snapOk = runCommandWithEstimatedProgress("snap refresh",
                                                                 snapStart, snapEnd, "Snap",
                                                                 snapStage, static_cast<int>(snapUpgradable.size()));
                    }
                    if (!snapOk) {
                        cout << "\n" << YELLOW << "Warning: Snap stage encountered some issues." << RESET << endl;
                        cout << YELLOW << "Check " << LOG_PATH << " for details." << RESET << endl;
                    }
                    if (g_interrupted || snapStage.exitCode == 130) {
                        g_cancelledByUser = true;
                        cout << "\n" << YELLOW << "Operation cancelled by user (Ctrl+C)." << RESET << endl;
                        return 130;
                    }
                }

                drawGlobalBar(100, "Done!");
                cout << endl << "\n" << GREEN << (DryRun ? "Dry-run finished successfully!" : "Update successful!") << RESET << endl;
                cout << YELLOW << "\n[Report]\n" << RESET;
                cout << "- refresh: " << (refreshStage.ok ? "OK" : "FAIL") << " (code " << refreshStage.exitCode << ")\n";
                cout << "- apt: " << (aptStage.ok ? "OK" : "FAIL") << " (code " << aptStage.exitCode << ")\n";
                if (hasFlatpak) {
                    cout << "- flatpak: " << (flatpakStage.ok ? "OK" : "FAIL") << " (code " << flatpakStage.exitCode << ")\n";
                }
                if (hasSnap) {
                    cout << "- snap: " << (snapStage.ok ? "OK" : "FAIL") << " (code " << snapStage.exitCode << ")\n";
                }
                updateSuccessful = true;
            } else {
                cout << "\n" << RED << "CRITICAL: System upgrade failed! Check " << LOG_PATH << RESET << endl;
                cout << YELLOW << "Check " << LOG_PATH << " for details." << RESET << endl;
                cout << YELLOW << "[APT details] " << RESET << aptStage.details << "\n";
                cout << YELLOW << "[Exit code] " << RESET << aptStage.exitCode << "\n";
            }
        }
    }

    // --- POWER ACTIONS ---
    if (updateSuccessful && !g_cancelledByUser) {
        if (Reboot) { cout << YELLOW << "Rebooting..." << RESET << endl; sync(); system("reboot"); }
        else if (Shutdown) { cout << YELLOW << "Shutting down..." << RESET << endl; sync(); system("poweroff"); }


        

    }

    return 0;
}
