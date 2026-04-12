#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <cstdio>
#include <memory>
#include <array>
#include <algorithm>

using namespace std;

string exec(const char* cmd) {
  array<char, 128> buffer;
  string result;
  unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
    return "";
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

string get_latest_version() {
  string api_url = "https://api.github.com/repos/Ignacyyy/Zielina_Package_Manager/releases/latest";
  string cmd = "curl -s -H 'User-Agent: Zielina-PM' " + api_url + " | grep '\"tag_name\"' | head -1 | sed 's/.*\"v\\([0-9.]*\\)\".*/\\1/'";
  string version = exec(cmd.c_str());
  version.erase(remove(version.begin(), version.end(), '\n'), version.end());
  version.erase(remove(version.begin(), version.end(), ' '), version.end());


  if (version.empty()) {
    string fallback_cmd = "curl -s https://raw.githubusercontent.com/Ignacyyy/Zielina_Package_Manager/main,-APT(debian,ubuntu.)/VERSION.txt | grep -oP '\\d+(\\.\\d+)*'";
    version = exec(fallback_cmd.c_str());
    version.erase(remove(version.begin(), version.end(), '\n'), version.end());
  }

  return version;
}

string get_installed_version() {
  string version = "";
  FILE* file = fopen("/opt/ZPM/VERSION.txt", "r");
  if (file) {
    char line[64];
    if (fgets(line, sizeof(line), file)) {
      version = line;
      version.erase(remove(version.begin(), version.end(), '\n'), version.end());
    }
    fclose(file);
  }
  return version.empty() ? "none" : version;
}

void print_banner() {
  const string GREEN = "\033[1;32m";
  const string RESET = "\033[0m";

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

int main() {
  const string GREEN = "\033[1;32m";
  const string RESET = "\033[0m";
  const string RED = "\033[31m";

  string answer;

  if (geteuid() != 0) {
    cout << RED << "Run with sudo!\n" << RESET;
    return 1;
  }

  print_banner();

  string current_version = get_installed_version();
  string latest_version = get_latest_version();

  cout << "\n==========================================" << endl;
  cout << "Installed version: " << current_version << endl;
  cout << "Latest release:    " << latest_version << endl;
  cout << "==========================================\n" << endl;

  if (current_version == latest_version && current_version != "none") {
    cout << GREEN << "You already have the latest version (" << current_version << ")." << RESET << endl;
    return 0;
  }

  cout << RED << "New version available!" << RESET << endl;
  cout << "Do you want to update? [Y/n]: ";
  cin >> answer;

  if (answer != "y" && answer != "Y" && !answer.empty()) {
    cout << "Update canceled." << endl;
    return 0;
  }

  cout << GREEN << "Updating..." << RESET << endl;
  sleep(1);

  cout << "Updating system packages..." << endl;
  system("sudo apt update && sudo apt upgrade -y");

  cout << "Downloading latest Zielina release (" << latest_version << ")..." << endl;

  string temp_dir = "/tmp/zielina_update_" + to_string(time(nullptr));
  string mkdir_cmd = "mkdir -p " + temp_dir;
  system(mkdir_cmd.c_str());

  string download_url = "https://github.com/Ignacyyy/Zielina_Package_Manager/archive/refs/tags/v" + latest_version + ".tar.gz";
  string archive_path = temp_dir + "/zielina.tar.gz";
  string download_cmd = "curl -L -o " + archive_path + " " + download_url;

  if (system(download_cmd.c_str()) != 0) {
    cerr << RED << "Failed to download release archive!" << RESET << endl;
    return 1;
  }

  cout << "Extracting..." << endl;
  string extract_cmd = "tar -xzf " + archive_path + " -C " + temp_dir;
  if (system(extract_cmd.c_str()) != 0) {
    cerr << RED << "Failed to extract archive!" << RESET << endl;
    return 1;
  }

  string find_cmd = "find " + temp_dir + " -maxdepth 1 -type d -name 'Zielina_Package_Manager-*' | head -1";
  string extracted_dir = exec(find_cmd.c_str());
  extracted_dir.erase(remove(extracted_dir.begin(), extracted_dir.end(), '\n'), extracted_dir.end());

  if (extracted_dir.empty()) {
    cerr << RED << "Could not find extracted files!" << RESET << endl;
    return 1;
  }

  cout << "Installing Zielina " << latest_version << "..." << endl;
  string install_cmd = "cd " + extracted_dir + " && chmod +x INSTALL.sh && sudo ./INSTALL.sh";
  system(install_cmd.c_str());

  string cleanup_cmd = "rm -rf " + temp_dir;
  system(cleanup_cmd.c_str());

  cout << GREEN << "\nUpdate completed!" << RESET << endl;

  return 0;
}
