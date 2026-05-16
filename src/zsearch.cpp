#include "main.h"

// lowercase
std::string toLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

// highlight
std::string highlight(const std::string& text, const std::string& query) {
    std::string lowerText  = toLower(text);
    std::string lowerQuery = toLower(query);
    size_t pos = lowerText.find(lowerQuery);
    if (pos == std::string::npos) return text;
    return text.substr(0, pos) +
        YELLOW + text.substr(pos, query.length()) + RESET +
        text.substr(pos + query.length());
}

int main(int argc, char* argv[]) {
    zpm_update::checkForUpdates();
    using namespace std;

    bool showHelp    = false;
    bool showVersion = false;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if      (arg == "--help"    || arg == "-h") showHelp    = true;
        else if (arg == "--version" || arg == "-v") showVersion = true;
    }

    if (showVersion && showHelp) {
        cout << YELLOW << "--version\n" << RESET;
        cout << RED << "zsearch component version: " << zpm_version::version() << " of ZPM\n" << RESET;
        cout << "https://github.com/Ignacyyy/ZPM\n";
        cout << "Copyright (c) 2026 Ignacyyy\nLicense: MIT\n\n";
        cout << YELLOW << "--help\n" << RESET;
        cout << RED << "Usage: " << RESET << argv[0] << " [options] [packages...] or zpm search [options] [packages...]\n\n";
        cout << RED << "Options:\n" << RESET;
        cout << "  --version, -v  Show version information\n";
        cout << "  --help,    -h  Show this help message\n";
        return 0;
    }
    if (showVersion) {
        cout << RED << "zsearch component version: " << zpm_version::version() << " of ZPM\n" << RESET;
        cout << "https://github.com/Ignacyyy/ZPM\n";
        cout << "Copyright (c) 2026 Ignacyyy\nLicense: MIT\n";
        return 0;
    }
    if (showHelp) {
        cout << RED << "Usage: " << RESET << argv[0] << " [options] [packages...] or zpm search [options] [packages...]\n\n";
        cout << RED << "Options:\n" << RESET;
        cout << "  --version, -v  Show version information\n";
        cout << "  --help,    -h  Show this help message\n";
        return 0;
    }

    std::string query;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.find(';') != std::string::npos ||
            arg.find('&') != std::string::npos ||
            arg.find('|') != std::string::npos) {
            std::cout << RED << "Invalid characters!\n" << RESET;
            return 1;
        }
        if (arg[0] != '-') query += arg + " ";
    }

    std::string queryTrimmed = query;
    if (!queryTrimmed.empty()) queryTrimmed.pop_back(); // usuń trailing space

    std::cout << GREEN << " Searching: " << RESET << queryTrimmed << "\n\n";

    // ─── APT ─────────────────────────────────────────────────────────────────
    {
        std::string command = "apt-cache search " + query;
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::cerr << RED << "Error running apt\n" << RESET;
            return 1;
        }

        char buffer[256];
        int count = 0;
        bool headerPrinted = false;

        while (fgets(buffer, sizeof(buffer), pipe)) {
            std::string line(buffer);
            size_t dash = line.find(" - ");
            if (dash != std::string::npos) {
                if (!headerPrinted) {
                    std::cout << YELLOW << "=== APT results ===\n" << RESET;
                    headerPrinted = true;
                }
                std::string name = highlight(line.substr(0, dash), queryTrimmed);
                std::string desc = highlight(line.substr(dash + 3), queryTrimmed);
                std::cout << GREEN << "[APT]" << RESET << " " << name << "\n";
                std::cout << "    " << desc;
                count++;
            }
        }
        pclose(pipe);

        if (count == 0)
            std::cout << YELLOW << "=== APT: no results ===\n" << RESET;
        else
            std::cout << "\n" << GREEN << " APT found: " << count << " packages\n" << RESET;
    }

    // ─── FLATPAK ─────────────────────────────────────────────────────────────
    bool hasFlatpak = (access("/usr/bin/flatpak", X_OK) == 0 ||
                       access("/bin/flatpak",     X_OK) == 0);
    if (hasFlatpak) {
        std::string command = "flatpak search --columns=application,name,description " + query + " 2>/dev/null";
        FILE* pipe = popen(command.c_str(), "r");

        char buffer[512];
        int count = 0;
        bool headerPrinted = false;

        while (fgets(buffer, sizeof(buffer), pipe)) {
            std::string line(buffer);
            line.erase(line.find_last_not_of(" \n\r\t") + 1);

            size_t t1 = line.find('\t');
            size_t t2 = line.find('\t', t1 + 1);
            if (t1 == std::string::npos) continue;

            std::string appId = line.substr(0, t1);
            std::string name  = (t2 != std::string::npos) ? line.substr(t1 + 1, t2 - t1 - 1) : line.substr(t1 + 1);
            std::string desc  = (t2 != std::string::npos) ? line.substr(t2 + 1) : "";

            if (!headerPrinted) {
                std::cout << "\n" << YELLOW << "=== Flatpak results ===\n" << RESET;
                headerPrinted = true;
            }

            std::cout << GREEN << "[FLATPAK]" << RESET << " "
                      << highlight(appId, queryTrimmed) << " - "
                      << highlight(name,  queryTrimmed) << "\n";
            if (!desc.empty())
                std::cout << "    " << highlight(desc, queryTrimmed) << "\n";
            count++;
        }
        pclose(pipe);

        if (count == 0)
            std::cout << "\n" << YELLOW << "=== Flatpak: no results ===\n" << RESET;
        else
            std::cout << "\n" << GREEN << " Flatpak found: " << count << " packages\n" << RESET;
    }

    // ─── SNAP ────────────────────────────────────────────────────────────────
    bool hasSnap = (access("/usr/bin/snap", X_OK) == 0 ||
                    access("/bin/snap",     X_OK) == 0 ||
                    access("/snap/bin/snap",X_OK) == 0);
    if (hasSnap) {
        std::string command = "snap find " + query + " 2>/dev/null";
        FILE* pipe = popen(command.c_str(), "r");

        char buffer[512];
        int count = 0;
        bool headerPrinted = false;

  
        fgets(buffer, sizeof(buffer), pipe);

        while (fgets(buffer, sizeof(buffer), pipe)) {
            std::string line(buffer);
            line.erase(line.find_last_not_of(" \n\r\t") + 1);
            if (line.empty()) continue;

           
            std::istringstream iss(line);
            std::string name, version, publisher, notes, summary;
            iss >> name >> version >> publisher >> notes;
            std::getline(iss, summary);
            if (!summary.empty() && summary[0] == ' ')
                summary.erase(0, summary.find_first_not_of(" \t"));

            if (!headerPrinted) {
                std::cout << "\n" << YELLOW << "=== Snap results ===\n" << RESET;
                headerPrinted = true;
            }

            std::cout << GREEN << "[SNAP]" << RESET << " "
                      << highlight(name, queryTrimmed) << "\n";
            if (!summary.empty())
                std::cout << "    " << highlight(summary, queryTrimmed) << "\n";
            count++;
        }
        pclose(pipe);

        if (count == 0)
            std::cout << "\n" << YELLOW << "=== Snap: no results ===\n" << RESET;
        else
            std::cout << "\n" << GREEN << " Snap found: " << count << " packages\n" << RESET;
    }

    return 0;
}