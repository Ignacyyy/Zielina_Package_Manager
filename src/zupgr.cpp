// Compile: g++ -O2 -o zupdate zupdate.cpp

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

string exec(const char* cmd) {
  array<char, 128> buffer;
  string result;
  unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) return "";
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    result += buffer.data();
  return result;
}

string get_latest_version() {
  string api_url = "https://api.github.com/repos/Ignacyyy/Zielina_Package_Manager/releases/latest";
  string cmd = "curl -s -H 'User-Agent: Zielina-PM' " + api_url
  + " 2>/dev/null | grep '\"tag_name\"' | head -1"
  + " | sed 's/.*\"v\\([0-9.]*\\)\".*/\\1/'";
  string version = exec(cmd.c_str());
  version.erase(remove(version.begin(), version.end(), '\n'), version.end());
  version.erase(remove(version.begin(), version.end(), ' '),  version.end());

  if (version.empty()) {
    string fallback = "curl -s 2>/dev/null https://raw.githubusercontent.com/Ignacyyy/"
    "Zielina_Package_Manager/main,-APT(debian,ubuntu.)/VERSION.txt"
    " | grep -oP '\\d+(\\.\\d+)*'";
    version = exec(fallback.c_str());
    version.erase(remove(version.begin(), version.end(), '\n'), version.end());
  }
  return version;
}

string get_installed_version() {
  FILE* file = fopen("/opt/ZPM/VERSION.txt", "r");
  if (!file) return "none";
  char line[64];
  string version;
  if (fgets(line, sizeof(line), file)) {
    version = line;
    version.erase(remove(version.begin(), version.end(), '\n'), version.end());
  }
  fclose(file);
  return version.empty() ? "none" : version;
}

void print_banner() {
  cout << RED << "Zielina Package Manager updater program" << RESET << endl;

  cout << GREEN;

 cout << R"(


                               ↑
                               ↑↑
                               ↖↑↙←                       ↑
                               ↑↖↙↖↘↖                     ↑
                               ↗↙↑↑↙↑↑←                 ↖↑←
                               →↖↖↓↑↖↑↓↑              →↑↑↑
                               ↙↑↖↖↑↖←↙↓↑           ↑↑↖ ↖←
                               ↑↑↖↖↗→↖↑↖↑→         ↙↖↑↑→↖↑
                                ↙↙↑↙↗↘↖↓↖↑       ↑↙←↑↓↖↘↑   ↑
                                ↑↘↖↖←↑↗↓↖↑      →↑↖↑←→↖↖↑  ←↑
                                 ↑←↑←↖↖←↓↘↖    ←↑ ↑↖↗↖↗↙↑ ↙←←
                                   ↑↗→↙↑↑↓    ↓↖↖↓↖↑→↖→→ ↖↖↖↖↙
                                     ↑↙←↘→    ↙↗↖↑↑↓↑↖↖↑ ↓↖↑↘←
               ↑↑↑↓↑↖↖↖↖↑↙↙↑↙→↙↘↖     →→↑↓←  ↑←↗↖→↖↖↖↑↑ ↖↑↖↗↖→
                 ↗←↖↖↑↑↘→↘↗↖↖↗↖↖↑↙↖↘    ↑↙↗  ↗↖↙↖↙←↑↓↑ ↓↖←→←↙←
                   ↙↑↙↖↖↖↙↑↑→↘↖→↖↑↖↖↑↑↖   ↑  ↙↘↖↑↖↙↑  ↖→↑↖↑←←↑
                      ↑↑↖↖↖↓←↑↗↗↑↖↑↓↖↙↖↘←    ↙↖↑↖→↑  ↓↙↑↖↖↑↖↖↓
                        ↑↑→←↖↙↖↖↑↓→↓↑↑↑←↙↙   ↖↑→↑    ↑↓↖↑↖↖↘↖→
                            →↙→↙↙↖→↖↖↖↖↓→↑↑ ↑↖↖     ↓↖↑↖→↑↖←↓↘
                                  ↑↘→↑↓→↑    ↑      →↖↖→←↖↘←↑
                                              ↑    ←→↗↖←↘↖↘←↓
                                               ↑   ←↖↖←↗↖↖↗↘
                                                ↘  ←→←↖↓↗↙→
                          ↓↘↑↑→↑↑↙↑↑←↓↑↙↖↓←↓     ↖ ↙↙↖↑↖↓→
                     ↓↖↓↑↓←↖↘↖↖↓↖↖↖↖↖↑↖↗↑↗↑↓↙↑←←    ↖↓↑←↑
                  ↑↘→↑↑↙↑↙↗↑↑↘ ↑↑→↘→↗↖↓↖↖↖↖↖←↖↙↖↖↖ ↑ ↖↗
                        ↙↙↓↑←↙↓↑↖↓↘↖→↘↓↑↑↑↑↗↑↑↑↖↑←→ ↑↖
                            →↑↖↓↙←↑←←→↓↖↙↗↖←↗↖←↖↙↑   ↑
                                  →→↙↓↘↓←↙↘↗↙          ↖
                                                        ↑

                                                           ↑
                                                            ↑
                                                              ↑
                                                                ↑↖
                                                                   ↑
                                                                     ↑
                                                                        ↑
                                                                           ↑↖
)" << endl;
}

int main(int argc, char* argv[]) {
  for (int i = 1; i < argc; ++i) {
    string arg = argv[i];
    if (arg == "--version" || arg == "-v") {
      cout << RED << "zupgr component version: 1.0 of ZPM\n" << RESET;
      cout << "https://github.com/Ignacyyy/Zielina_Package_Manager\n";
      cout << "Copyright (c) 2026 Ignacyyy\nLicense: MIT\n";
      return 0;
    }
    if (arg == "--help" || arg == "-h") {
      cout << RED << "Usage: " << RESET << argv[0] << " [options]\n\n";
      cout << RED << "Options:\n" << RESET;
      cout << "  --version, -v  Show version information\n";
      cout << "  --help,    -h  Show this help message\n\n";
      cout << "Checks for ZPM updates and installs them.\n";
      return 0;
    }
  }

  if (geteuid() != 0) {
    cout << RED << "Run with sudo!\n" << RESET;
    return 1;
  }

  print_banner();

  string current_version = get_installed_version();
  string latest_version  = get_latest_version();

  cout << GREEN << "\n==========================================\n" << RESET;
  cout << "Installed version: " << current_version << "\n";
  cout << "Latest release:    " << latest_version  << "\n";
  cout << GREEN << "==========================================\n\n" << RESET;

  if (current_version == latest_version && current_version != "none") {
    cout << RED << "You already have the latest version (" << current_version << ").\n" << RESET;
    return 0;
  }

  cout << RED << "New version available!\n" << RESET;
  cout << "Do you want to update? [Y/n]: ";
  string answer;
  getline(cin, answer);
  if (answer != "y" && answer != "Y" && !answer.empty()) {
    cout << "Update canceled.\n";
    return 0;
  }

  string temp_dir    = "/tmp/zielina_update_" + to_string(time(nullptr));
  string archive     = temp_dir + "/zielina.tar.gz";
  string log_file    = temp_dir + "/update.log";

  // Create temp dir
  (void)system(("mkdir -p " + temp_dir).c_str());

  // ── apt update + upgrade (suppress output, log it) ────────────────────────
  cout << GREEN << "Updating system packages..." << RESET << "\n";
  string apt_cmd = "DEBIAN_FRONTEND=noninteractive apt-get update -y >> "
  + log_file + " 2>&1 && "
  "DEBIAN_FRONTEND=noninteractive apt-get upgrade -y >> "
  + log_file + " 2>&1";
  if (system(apt_cmd.c_str()) != 0)
    cout << RED << "Warning: apt update/upgrade had errors (see " << log_file << ").\n" << RESET;

  // ── download release archive (silent, log errors) ─────────────────────────
  cout << "Downloading ZPM " << latest_version << "...\n";
  string download_url = "https://github.com/Ignacyyy/Zielina_Package_Manager"
  "/archive/refs/tags/v" + latest_version + ".tar.gz";
  string dl_cmd = "curl -L --silent --show-error -o " + archive
  + " " + download_url + " >> " + log_file + " 2>&1";
  if (system(dl_cmd.c_str()) != 0) {
    cerr << RED << "Failed to download release archive! See " << log_file << "\n" << RESET;
    (void)system(("rm -rf " + temp_dir).c_str());
    return 1;
  }

  // ── extract (silent) ──────────────────────────────────────────────────────
  cout << "Extracting...\n";
  string extract_cmd = "tar -xzf " + archive + " -C " + temp_dir
  + " >> " + log_file + " 2>&1";
  if (system(extract_cmd.c_str()) != 0) {
    cerr << RED << "Failed to extract archive! See " << log_file << "\n" << RESET;
    (void)system(("rm -rf " + temp_dir).c_str());
    return 1;
  }

  // ── find extracted dir ────────────────────────────────────────────────────
  string find_cmd    = "find " + temp_dir + " -maxdepth 1 -type d"
  " -name 'Zielina_Package_Manager-*' | head -1";
  string extracted   = exec(find_cmd.c_str());
  extracted.erase(remove(extracted.begin(), extracted.end(), '\n'), extracted.end());

  if (extracted.empty()) {
    cerr << RED << "Could not find extracted files!\n" << RESET;
    (void)system(("rm -rf " + temp_dir).c_str());
    return 1;
  }

  // ── run installer ─────────────────────────────────────────────────────────
  cout << "Installing ZPM " << latest_version << "...\n";
  string install_cmd = "cd " + extracted + " && chmod +x INSTALL.sh"
  " && ./INSTALL.sh >> " + log_file + " 2>&1";
  int install_rc = system(install_cmd.c_str());

  // ── cleanup temp dir (but keep log on failure) ────────────────────────────
  if (install_rc != 0) {
    cerr << RED << "Installation failed! Log saved to " << log_file << "\n" << RESET;
    // Remove everything except the log
    (void)system(("find " + temp_dir + " ! -name 'update.log' -delete 2>/dev/null; "
    "mv " + log_file + " /tmp/zupdate_failed.log 2>/dev/null; "
    "rm -rf " + temp_dir).c_str());
    cerr << RED << "See /tmp/zupdate_failed.log for details.\n" << RESET;
    return 1;
  }

  // Success: remove entire temp dir including log
  (void)system(("rm -rf " + temp_dir).c_str());

  cout << GREEN << "\nUpdate complete! ZPM " << latest_version << " installed.\n" << RESET;
  return 0;
}
