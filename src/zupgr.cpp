// Compile: g++ -O2 -o zupgr zupgr.cpp

#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <cstdio>
#include <memory>
#include <array>
#include <algorithm>
#include <ctime>

using namespace std;

const string GREEN  = "\033[1;32m";
const string RESET  = "\033[0m";
const string RED    = "\033[31m";
const string YELLOW = "\033[33m";
const string CYAN   = "\033[1;36m";

const string TARGET = "/opt/ZPM";

string exec(const char* cmd) {
    array<char, 128> buffer;
    string result;
    unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        result += buffer.data();
    return result;
}

// ───────────────────────── VERSION ─────────────────────────

// Pobiera najnowszy stabilny release (nie-prerelease)
string get_latest_version() {
    string cmd =
        "curl -fsSL -H 'User-Agent: ZPM' "
        "https://api.github.com/repos/Ignacyyy/ZPM/releases/latest"
        " | grep '\"tag_name\"' | head -1"
        " | sed 's/.*\"v\\([0-9.]*\\)\".*/\\1/'";
    string v = exec(cmd.c_str());
    v.erase(remove(v.begin(), v.end(), '\n'), v.end());
    v.erase(remove(v.begin(), v.end(), ' '),  v.end());
    return v;
}

// Pobiera najnowszy prerelease (filtruje po "prerelease": true)
string get_latest_prerelease_version() {
    string cmd =
        "curl -fsSL -H 'User-Agent: ZPM' "
        "https://api.github.com/repos/Ignacyyy/ZPM/releases"
        " | python3 -c \""
        "import sys, json; "
        "releases = json.load(sys.stdin); "
        "pre = [r for r in releases if r.get('prerelease', False)]; "
        "print(pre[0]['tag_name'].lstrip('v') if pre else '')\"";
    string v = exec(cmd.c_str());
    v.erase(remove(v.begin(), v.end(), '\n'), v.end());
    v.erase(remove(v.begin(), v.end(), ' '),  v.end());
    return v;
}

string get_installed_version() {
    FILE* f = fopen("/opt/ZPM/VERSION.txt", "r");
    if (!f) return "none";
    char line[64];
    string v;
    if (fgets(line, sizeof(line), f)) v = line;
    fclose(f);
    v.erase(remove(v.begin(), v.end(), '\n'), v.end());
    return v.empty() ? "none" : v;
}

// Odczytuje zainstalowaną wersję pre (z PREVERSION.txt, jeśli istnieje)
string get_installed_preversion() {
    FILE* f = fopen("/opt/ZPM/PREVERSION.txt", "r");
    if (!f) return "none";
    char line[64];
    string v;
    if (fgets(line, sizeof(line), f)) v = line;
    fclose(f);
    v.erase(remove(v.begin(), v.end(), '\n'), v.end());
    return v.empty() ? "none" : v;
}

// ───────────────────────── UI ─────────────────────────

void print_banner() {
    cout << RED << "Zielina Package Manager Updater (zupgr)" << RESET << "\n\n";
}

// ───────────────────────── INSTALL ─────────────────────────

bool install_from_dir(const string& src) {
    cout << "Installing to " << TARGET << "...\n";
    if (system(("rm -rf " + TARGET).c_str()) != 0) return false;
    if (system(("mkdir -p " + TARGET).c_str()) != 0) return false;
    if (system(("cp -r " + src + "/. " + TARGET + "/").c_str()) != 0) return false;

    string bin = TARGET + "/bin";

    string chmod_cmd = "find " + bin + " -type f -exec chmod +x {} +";
    system(chmod_cmd.c_str());

    cout << "Updating symlinks in /usr/bin/...\n";
    string rm_links =
        "find /usr/bin -maxdepth 1 -type l | while read -r link; do "
        "  dest=$(readlink \"$link\"); "
        "  if echo \"$dest\" | grep -q '^" + TARGET + "/bin/'; then "
        "    rm -f \"$link\"; "
        "  fi; "
        "done";
    system(rm_links.c_str());

    string ln_cmd =
        "if [ -d " + bin + " ] && [ -n \"$(ls -A " + bin + " 2>/dev/null)\" ]; then "
        "  for f in " + bin + "/*; do "
        "    [ -f \"$f\" ] || continue; "
        "    ln -sf \"$f\" /usr/bin/\"$(basename $f)\"; "
        "  done; "
        "fi";
    system(ln_cmd.c_str());

    return true;
}

// ───────────────────────── MAIN ─────────────────────────

int main(int argc, char* argv[]) {

    bool help         = false;
    bool version      = false;
    bool force        = false;
    bool experimental = false;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--help"         || arg == "-h")  help         = true;
        if (arg == "--version"      || arg == "-v")  version      = true;
        if (arg == "--force"        || arg == "-f")  force        = true;
        if (arg == "--experimental" || arg == "-ex") experimental = true;
    }

// ── HELP / VERSION ─────────────────────────────

    if (help && version) {
        cout << YELLOW << "--version" << RESET << "\n";
        cout << RED << "zupgr component version: 1.3 of ZPM" << RESET << "\n";
        cout << "https://github.com/Ignacyyy/ZPM/n";
        cout << "Copyright (c) 2026 Ignacyyy\nLicense: MIT\n\n";
        cout << YELLOW << "--help" << RESET << "\n";
        cout << RED << "Usage: " << RESET << argv[0] << " [options]" << " or zpm upgr/upgrade [options]"  "\n\n";
        cout << RED << "Options:" << RESET << "\n";
        cout << "  --help,    -h   Show this help message\n";
        cout << "  --version, -v   Show version information\n";
        cout << "  --force,   -f   Force reinstall even if already up to date\n";
        return 0;
    }

    if (version) {
        cout << RED << "zupgr component version: 1.3 of ZPM\n" << RESET;
        cout << "https://github.com/Ignacyyy/ZPM/n";
        cout << "License: MIT\n";
        return 0;
    }

    if (help) {
        cout << RED << "Usage: " << RESET << argv[0] << " [options]" << " or zpm upgr/upgrade [options]"  "\n\n";
        cout << "Options:\n";
        cout << "  -h, --help      Show help\n";
        cout << "  -v, --version   Show version\n";
        cout << "  -f, --force     Force reinstall even if already up to date\n";
        cout << "\nChecks and installs updates for ZPM.\n";
        return 0;
    }

    // ── ROOT CHECK ─────────────────────────────

    if (geteuid() != 0) {
        cout << RED << "Run with sudo!\n" << RESET;
        return 1;
    }

    print_banner();

    // ── VERSION CHECK ─────────────────────────────

    string current        = get_installed_version();
    string current_pre    = get_installed_preversion();
    string latest         = get_latest_version();
    string latest_pre     = get_latest_prerelease_version();

    if (latest.empty()) {
        cout << RED << "ERROR: Could not fetch latest version. Check your internet connection.\n" << RESET;
        return 1;
    }

    // Ustal co jest aktualnie zainstalowane i jakiego typu
    bool is_pre_installed = (current_pre != "none");
    string installed_ver  = is_pre_installed ? current_pre : current;

    // Pokaż status
    if (installed_ver == "none") {
        cout << YELLOW << "Installed:               none\n" << RESET;
    } else if (is_pre_installed) {
        cout << CYAN  << "Installed:               " << installed_ver << " [pre-release]\n" << RESET;
    } else {
        cout << GREEN << "Installed:               " << installed_ver << " [stable]\n" << RESET;
    }
    cout << GREEN << "Latest    (stable):      " << latest     << RESET << "\n";
    cout << CYAN  << "Latest    (pre-release): " << (latest_pre.empty() ? "none" : latest_pre) << RESET << "\n\n";

    // ── EXPERIMENTAL (pre-release) PATH ─────────────────────────────

    if (experimental) {
        if (latest_pre.empty()) {
            cout << RED << "ERROR: Could not fetch latest pre-release version.\n" << RESET;
            return 1;
        }

        cout << CYAN << "[EXPERIMENTAL] Pre-release mode active.\n" << RESET;

        if (current_pre == latest_pre && current_pre != "none") {
            if (!force) {
                cout << GREEN << "Already on latest pre-release.\n" << RESET;
                return 0;
            }
            cout << YELLOW << "Already on latest pre-release, but --force specified. Reinstalling...\n" << RESET;
        }

        cout << CYAN << "Pre-release update available: v" << latest_pre << ". Continue? [Y/n]: " << RESET;
        string ans;
        getline(cin, ans);
        if (!(ans.empty() || ans == "y" || ans == "Y")) {
            cout << "Cancelled.\n";
            return 0;
        }

        string temp    = "/tmp/zupgr_" + to_string(time(nullptr));
        string archive = temp + "/zpm.tar.gz";
        system(("mkdir -p " + temp).c_str());

        string url = "https://github.com/Ignacyyy/ZPM/archive/refs/tags/v"
                     + latest_pre + ".tar.gz";

        cout << "Downloading pre-release v" << latest_pre << "...\n";
        if (system(("curl -fsSL -o " + archive + " " + url).c_str()) != 0) {
            cout << RED << "Download failed!\n" << RESET;
            system(("rm -rf " + temp).c_str());
            return 1;
        }

        cout << "Extracting...\n";
        if (system(("tar -xzf " + archive + " -C " + temp).c_str()) != 0) {
            cout << RED << "Extract failed!\n" << RESET;
            system(("rm -rf " + temp).c_str());
            return 1;
        }

        string extracted = exec(("find " + temp + " -mindepth 1 -maxdepth 1 -type d | head -1").c_str());
        extracted.erase(remove(extracted.begin(), extracted.end(), '\n'), extracted.end());

        if (extracted.empty()) {
            cout << RED << "No extracted folder found!\n" << RESET;
            system(("rm -rf " + temp).c_str());
            return 1;
        }

        if (!install_from_dir(extracted)) {
            cout << RED << "Installation failed.\n" << RESET;
            system(("rm -rf " + temp).c_str());
            return 1;
        }

        // Zapisz wersję pre do PREVERSION.txt
        FILE* pvf = fopen((TARGET + "/PREVERSION.txt").c_str(), "w");
        if (pvf) {
            fprintf(pvf, "%s\n", latest_pre.c_str());
            fclose(pvf);
        }

        system(("rm -rf " + temp).c_str());
        cout << CYAN << "\nPre-release update complete! Version: " << latest_pre << "\n" << RESET;
        return 0;
    }

    // ── STABLE PATH ─────────────────────────────

    // Jeśli zainstalowane jest pre-release, zawsze trzeba zainstalować stable
    if (!is_pre_installed && current == latest && current != "none") {
        if (!force) {
            cout << GREEN << "Already up to date.\n" << RESET;
            return 0;
        }
        cout << YELLOW << "Already up to date, but --force specified. Reinstalling...\n" << RESET;
    } else if (is_pre_installed) {
        cout << YELLOW << "Currently on pre-release. Downgrading to stable v" << latest << ".\n" << RESET;
    }

    cout << RED << "Continue? [Y/n]: " << RESET;
    string ans;
    getline(cin, ans);
    if (!(ans.empty() || ans == "y" || ans == "Y")) {
        cout << "Cancelled.\n";
        return 0;
    }

    string temp    = "/tmp/zupgr_" + to_string(time(nullptr));
    string archive = temp + "/zpm.tar.gz";
    system(("mkdir -p " + temp).c_str());

    string url = "https://github.com/Ignacyyy/ZPM/archive/refs/tags/v"
                 + latest + ".tar.gz";

    cout << "Downloading v" << latest << "...\n";
    if (system(("curl -fsSL -o " + archive + " " + url).c_str()) != 0) {
        cout << RED << "Download failed!\n" << RESET;
        system(("rm -rf " + temp).c_str());
        return 1;
    }

    cout << "Extracting...\n";
    if (system(("tar -xzf " + archive + " -C " + temp).c_str()) != 0) {
        cout << RED << "Extract failed!\n" << RESET;
        system(("rm -rf " + temp).c_str());
        return 1;
    }

    string extracted = exec(("find " + temp + " -mindepth 1 -maxdepth 1 -type d | head -1").c_str());
    extracted.erase(remove(extracted.begin(), extracted.end(), '\n'), extracted.end());

    if (extracted.empty()) {
        cout << RED << "No extracted folder found!\n" << RESET;
        system(("rm -rf " + temp).c_str());
        return 1;
    }

    if (!install_from_dir(extracted)) {
        cout << RED << "Installation failed.\n" << RESET;
        system(("rm -rf " + temp).c_str());
        return 1;
    }

    // Zapisz nową wersję stabilną
    FILE* vf = fopen((TARGET + "/VERSION.txt").c_str(), "w");
    if (vf) {
        fprintf(vf, "%s\n", latest.c_str());
        fclose(vf);
    }

    // Usuń PREVERSION.txt — zainstalowana wersja jest teraz stabilna
    remove((TARGET + "/PREVERSION.txt").c_str());

    system(("rm -rf " + temp).c_str());
    cout << GREEN << "\nUpdate complete! Version: " << latest << "\n" << RESET;
    return 0;
}