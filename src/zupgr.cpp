

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

string get_latest_version() {
    string cmd =
        "curl -fsSL -H 'User-Agent: ZPM' "
        "https://api.github.com/repos/Ignacyyy/Zielina_Package_Manager/releases/latest"
        " | grep '\"tag_name\"' | head -1"
        " | sed 's/.*\"v\\([0-9.]*\\)\".*/\\1/'";
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

    // Utwórz nowe symlinki
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

    bool help    = false;
    bool version = false;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--help"    || arg == "-h") help    = true;
        if (arg == "--version" || arg == "-v") version = true;
    }

    // ── HELP / VERSION ─────────────────────────────

    if (help && version) {
        cout << YELLOW << "--version" << RESET << "\n";
        cout << RED << "zupgr component version: 1.2 of ZPM" << RESET << "\n";
        cout << "https://github.com/Ignacyyy/Zielina_Package_Manager\n";
        cout << "Copyright (c) 2026 Ignacyyy\nLicense: MIT\n\n";
        cout << YELLOW << "--help" << RESET << "\n";
        cout << RED << "Usage: " << RESET << argv[0] << " [options]\n\n";
        cout << RED << "Options:" << RESET << "\n";
        cout << "  --help,    -h   Show this help message\n";
        cout << "  --version, -v   Show version information\n";
        return 0;
    }

    if (version) {
        cout << RED << "zupgr component version: 1.2 of ZPM\n" << RESET;
        cout << "https://github.com/Ignacyyy/Zielina_Package_Manager\n";
        cout << "License: MIT\n";
        return 0;
    }

    if (help) {
        cout << RED << "Usage: zupgr [options]\n\n" << RESET;
        cout << "Options:\n";
        cout << "  -h, --help      Show help\n";
        cout << "  -v, --version   Show version\n";
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

    string current = get_installed_version();
    string latest  = get_latest_version();

    if (latest.empty()) {
        cout << RED << "ERROR: Could not fetch latest version. Check your internet connection.\n" << RESET;
        return 1;
    }

    cout << GREEN << "Installed: " << current << RESET << "\n";
    cout << GREEN << "Latest:    " << latest  << RESET << "\n\n";

    if (current == latest && current != "none") {
        cout << RED << "Already up to date.\n" << RESET;
        return 0;
    }

    cout << RED << "Update available. Continue? [Y/n]: " << RESET;
    string ans;
    getline(cin, ans);
    if (!(ans.empty() || ans == "y" || ans == "Y")) {
        cout << "Cancelled.\n";
        return 0;
    }

    // ── TEMP DIR ─────────────────────────────

    string temp    = "/tmp/zupgr_" + to_string(time(nullptr));
    string archive = temp + "/zpm.tar.gz";
    system(("mkdir -p " + temp).c_str());

    // ── DOWNLOAD ─────────────────────────────

    string url = "https://github.com/Ignacyyy/Zielina_Package_Manager/archive/refs/tags/v"
                 + latest + ".tar.gz";

    cout << "Downloading v" << latest << "...\n";
    if (system(("curl -fsSL -o " + archive + " " + url).c_str()) != 0) {
        cout << RED << "Download failed!\n" << RESET;
        system(("rm -rf " + temp).c_str());
        return 1;
    }

    // ── EXTRACT ─────────────────────────────

    cout << "Extracting...\n";
    if (system(("tar -xzf " + archive + " -C " + temp).c_str()) != 0) {
        cout << RED << "Extract failed!\n" << RESET;
        system(("rm -rf " + temp).c_str());
        return 1;
    }

    // ── FIND EXTRACTED DIR ─────────────────────────────

    string extracted = exec(("find " + temp + " -mindepth 1 -maxdepth 1 -type d | head -1").c_str());
    extracted.erase(remove(extracted.begin(), extracted.end(), '\n'), extracted.end());

    if (extracted.empty()) {
        cout << RED << "No extracted folder found!\n" << RESET;
        system(("rm -rf " + temp).c_str());
        return 1;
    }

    // ── INSTALL ─────────────────────────────

    if (!install_from_dir(extracted)) {
        cout << RED << "Installation failed.\n" << RESET;
        system(("rm -rf " + temp).c_str());
        return 1;
    }

    FILE* vf = fopen((TARGET + "/VERSION.txt").c_str(), "w");
    if (vf) {
        fprintf(vf, "%s\n", latest.c_str());
        fclose(vf);
    }

    // ── CLEANUP ─────────────────────────────

    system(("rm -rf " + temp).c_str());

    cout << GREEN << "\nUpdate complete! Version: " << latest << "\n" << RESET;
    return 0;
}