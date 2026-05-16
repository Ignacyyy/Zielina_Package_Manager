#pragma once
#include <string>
#include <fstream>

// how to use:
// #include "version.h", or if you using main.h, #include "main.h"
// to get version:
// zpm_version::version()
namespace zpm_version {
    inline std::string version() {
        std::string ver = "unknown";
        std::ifstream plik("/opt/ZPM/VERSION.txt");
        if (plik.is_open()) {
            plik >> ver;
            plik.close();
        }
        return ver;
    }
}