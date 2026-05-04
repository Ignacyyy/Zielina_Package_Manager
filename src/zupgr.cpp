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

const string GREEN = "\033[1;32m";
const string RESET = "\033[0m";
const string RED   = "\033[31m";
const string YELLOW = "\033[33m";

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
  string api_url = "https://api.github.com/repos/Ignacyyy/Zielina_Package_Manager/releases/latest";
  string cmd = "curl -s -H 'User-Agent: ZPM' " + api_url +
  " | grep '\"tag_name\"' | head -1 | sed 's/.*\"v\\([0-9.]*\\)\".*/\\1/'";

  string version = exec(cmd.c_str());
  version.erase(remove(version.begin(), version.end(), '\n'), version.end());
  version.erase(remove(version.begin(), version.end(), ' '), version.end());

  return version;
}

string get_installed_version() {
  FILE* file = fopen("/opt/ZPM/VERSION.txt", "r");
  if (!file) return "none";

  char line[64];
  string version;

  if (fgets(line, sizeof(line), file))
    version = line;

  fclose(file);

  version.erase(remove(version.begin(), version.end(), '\n'), version.end());
  return version.empty() ? "none" : version;
}

// ───────────────────────── UI ─────────────────────────

void print_banner() {
  cout << RED << "Zielina Package Manager Updater (zupgr)" << RESET << "\n\n";
}

// ───────────────────────── MAIN ─────────────────────────

int main(int argc, char* argv[]) {

  bool help = false;
  bool version = false;

  for (int i = 1; i < argc; i++) {
    string arg = argv[i];

    if (arg == "--help" || arg == "-h") help = true;
    if (arg == "--version" || arg == "-v") version = true;
  }

  // ── HELP / VERSION ─────────────────────────────

  if (help && version) {
    cout << YELLOW <<"--version" << RESET << endl;
    cout << RED << "zupgr component version: 1.1 of ZPM" << RESET << endl;
    cout << "https://github.com/Ignacyyy/Zielina_Package_Manager" << endl;
    cout << "Copyright (c) 2026 Ignacyyy" << endl;
    cout << "License: MIT" << endl;
    cout << "" << endl;
    cout << YELLOW << "--help" << RESET << endl;
    cout << RED << "Usage: "<< RESET << argv[0] << " [options]" << RESET << endl;
    cout << "" <<endl;
    cout << RED << "Options:" << RESET << endl;
    cout << "  --help, -h    Show this help message" << endl;
    cout << "  --version, -v    Show version information" << endl;
    return 0;
  }

  if (version) {
    cout << RED << "zupgr component version: 1.1 of ZPM\n" << RESET;
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

  string current = get_installed_version();
  string latest  = get_latest_version();

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

  string temp = "/tmp/zupgr_" + to_string(time(nullptr));
  string archive = temp + "/zpm.tar.gz";

  system(("mkdir -p " + temp).c_str());

  // ── DOWNLOAD ─────────────────────────────

  string url = "https://github.com/Ignacyyy/Zielina_Package_Manager/archive/refs/tags/v"
  + latest + ".tar.gz";

  cout << "Downloading...\n";

  if (system(("curl -L -o " + archive + " " + url).c_str()) != 0) {
    cout << RED << "Download failed!\n" << RESET;
    return 1;
  }

  // ── EXTRACT ─────────────────────────────

  cout << "Extracting...\n";

  if (system(("tar -xzf " + archive + " -C " + temp).c_str()) != 0) {
    cout << RED << "Extract failed!\n" << RESET;
    return 1;
  }

  // ── FIND ROOT DIR (SAFE) ─────────────────────────────

  string extracted = exec(("find " + temp + " -mindepth 1 -maxdepth 1 -type d | head -1").c_str());
  extracted.erase(remove(extracted.begin(), extracted.end(), '\n'), extracted.end());

  if (extracted.empty()) {
    cout << RED << "No extracted folder found!\n" << RESET;
    return 1;
  }

  cout << "Installing...\n";

  // ── SAFE INSTALL (NO GUESSING) ─────────────────────────────

  string install_cmd =
  "FILE=$(find " + extracted + " -name INSTALL.sh | head -1) && "
  "[ -n \"$FILE\" ] && chmod +x \"$FILE\" && sudo \"$FILE\" || "
  "echo 'INSTALL.sh not found' && exit 1";

  int rc = system(install_cmd.c_str());

  system(("rm -rf " + temp).c_str());

  if (rc != 0) {
    cout << RED << "Installation failed.\n" << RESET;
    return 1;
  }

  cout << GREEN << "\nUpdate complete!\n" << RESET;

  return 0;
}
