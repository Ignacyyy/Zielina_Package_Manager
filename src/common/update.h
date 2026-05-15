#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <array>
#include <memory>
#include <fstream>

namespace zpm_update {

// ───────────────────────── COLORS (zakładam że masz w main.h) ─────────────────────────
// RED, GREEN, YELLOW, CYAN, RESET, BOLD

// ───────────────────────── EXEC ─────────────────────────

static inline std::string exec(const char* cmd) {

    std::array<char, 128> buffer;
    std::string result;

    std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd, "r"), pclose);

    if (!pipe)
        return "";

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        result += buffer.data();

    return result;
}

// ───────────────────────── CONFIG ─────────────────────────

static inline bool updateInfoEnabled() {

    std::ifstream conf("/opt/ZPM/zielina.conf");

    if (!conf.is_open())
        return true; // default ON

    std::string line;

    while (std::getline(conf, line)) {

        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());

        if (line.empty())
            continue;

        if (line[0] == '#')
            continue;

        if (line.rfind("update-info=", 0) == 0) {

            std::string value = line.substr(12);

            std::transform(value.begin(), value.end(), value.begin(),
                [](unsigned char c){ return std::tolower(c); });

            return (value == "true" || value == "1" || value == "yes");
        }
    }

    return true;
}

// ───────────────────────── REMOTE VERSION ─────────────────────────

static inline std::string get_latest_version() {

    std::string cmd =
        "curl -fsSL -H 'User-Agent: ZPM' "
        "https://api.github.com/repos/Ignacyyy/ZPM/releases/latest"
        " | grep '\"tag_name\"' | head -1"
        " | sed 's/.*\"v\\([0-9.]*\\)\".*/\\1/'";

    std::string v = exec(cmd.c_str());

    v.erase(std::remove(v.begin(), v.end(), '\n'), v.end());
    v.erase(std::remove(v.begin(), v.end(), ' '), v.end());

    return v;
}

// ───────────────────────── LOCAL VERSION ─────────────────────────

static inline std::string get_installed_version() {

    FILE* f = fopen("/opt/ZPM/VERSION.txt", "r");

    if (!f)
        return "none";

    char line[64];
    std::string v;

    if (fgets(line, sizeof(line), f))
        v = line;

    fclose(f);

    v.erase(std::remove(v.begin(), v.end(), '\n'), v.end());
    v.erase(std::remove(v.begin(), v.end(), ' '), v.end());

    return v.empty() ? "none" : v;
}

// ───────────────────────── VERSION PARSE ─────────────────────────

static inline std::vector<int> parse_version(const std::string& v) {

    std::vector<int> out;
    std::string cur;

    for (char c : v) {

        if (c == '.') {
            out.push_back(cur.empty() ? 0 : std::stoi(cur));
            cur.clear();
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c)))
            cur += c;
        else
            break;
    }

    if (!cur.empty())
        out.push_back(std::stoi(cur));

    while (out.size() < 2)
        out.push_back(0);

    return out;
}

// ───────────────────────── COMPARE ─────────────────────────

static inline bool is_newer(const std::string& latest, const std::string& current) {

    if (current == "none")
        return true;

    auto a = parse_version(latest);
    auto b = parse_version(current);

    if (a[0] != b[0])
        return a[0] > b[0];

    return a[1] > b[1];
}

// ───────────────────────── CHECK ─────────────────────────

static inline void checkForUpdates() {

    // CONFIG GATE
    if (!updateInfoEnabled())
        return;

    std::string current = get_installed_version();
    std::string latest  = get_latest_version();

    if (latest.empty())
        return;

    if (current == "none")
        return;

    // UP-TO-DATE → SILENT
    if (!is_newer(latest, current))
        return;

    // OUTPUT ONLY IF UPDATE EXISTS
    std::cout << CYAN << "\n====================================\n" << RESET;
    std::cout << YELLOW << "      ZPM UPDATE AVAILABLE\n" << RESET;
    std::cout << CYAN << "====================================\n\n" << RESET;
}

} // namespace zpm_update