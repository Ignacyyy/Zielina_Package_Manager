#pragma once
// progressbar.h — ZPM universal progress bar
// Usage:
//   #include "ProgressBar.h"
//   ProgressBar bar;
//   bar.draw(42.5f, "APT: installing...");
//   bar.finish();

using std::string;
using std::cout;
using std::flush;
using std::min;
using std::max;
using std::to_string;

class ProgressBar {
public:
    string colorYellow  = "\033[33m";
    string colorGreen   = "\033[32m";
    string colorReset   = "\033[0m";
    string label        = "Install Progress";
    int    maxBarWidth  = 40;
    int    minBarWidth  = 10;
    long   throttleMs   = 150;   // min ms between redraws

    // Draw bar at given percent with task description.
    // Safe on any terminal width — adapts and cleans up wrapped lines.
    void draw(float pct, const string& task) {
        int termWidth = getTermWidth();

        int percent = static_cast<int>(pct);
        if (percent < 0)   percent = 0;
        if (percent > 100) percent = 100;

        // Bar width: fill available space, clamped
        // Line format: "Label: [####...] 100% | task"
        int labelLen  = (int)label.size() + 3; // "Label: ["
        int suffixLen = 9;                      // "] 100% | "
        int taskMaxLen = min((int)task.size(), termWidth / 3);
        int barWidth = termWidth - labelLen - suffixLen - taskMaxLen - 1;
        if (barWidth < minBarWidth) barWidth = minBarWidth;
        if (barWidth > maxBarWidth) barWidth = maxBarWidth;

        // Truncate task to fit
        string t = task;
        int tMax = termWidth - labelLen - suffixLen - barWidth - 1;
        if (tMax < 1) tMax = 1;
        if ((int)t.size() > tMax) t = t.substr(0, tMax);

        // Build bar string (without ANSI for length calculation)
        string plain = label + ": [";
        for (int i = 0; i < barWidth; ++i) plain += (i < barWidth * percent / 100) ? "#" : " ";
        plain += "] " + to_string(percent) + "% | " + t;

        // Calculate how many terminal lines plain text occupies
        int newLines = max(1, ((int)plain.size() + termWidth - 1) / termWidth);

        // Erase previous bar (handle line-wrap on narrow terminals like Termux)
        if (_drawn) {
            // Go up to top of previous bar
            if (_prevLines > 1)
                cout << "\033[" << (_prevLines - 1) << "A";
            // Clear each line downward
            for (int i = 0; i < _prevLines; ++i) {
                cout << "\r\033[2K";
                if (i < _prevLines - 1) cout << "\033[1B";
            }
            // Return to top line
            if (_prevLines > 1)
                cout << "\033[" << (_prevLines - 1) << "A";
        }

        // Print colored bar
        int pos = barWidth * percent / 100;
        cout << "\r"
             << colorYellow << label << ": [" << colorReset;
        for (int i = 0; i < barWidth; ++i)
            cout << (i < pos ? colorGreen + string("#") + colorReset : string(" "));
        cout << colorYellow << "] " << percent << "% "
             << colorReset << "| " << t << flush;

        _prevLines = newLines;
        _drawn     = true;
        clock_gettime(CLOCK_MONOTONIC, &_lastDraw);
    }

    // Throttled version — skips redraw if called too soon.
    // Returns true if actually drew.
    bool drawThrottled(float pct, const string& task) {
        if (!_drawn || msSinceLastDraw() >= throttleMs) {
            draw(pct, task);
            return true;
        }
        return false;
    }

    // Move to a new line after the bar is complete.
    void finish(const string& finalTask = "Done!") {
        draw(100.0f, finalTask);
        cout << "\n" << flush;
        _drawn     = false;
        _prevLines = 0;
    }

    // Reset throttle timer (call before starting a new stage)
    void resetThrottle() {
        clock_gettime(CLOCK_MONOTONIC, &_lastDraw);
    }

private:
    bool            _drawn     = false;
    int             _prevLines = 0;
    struct timespec _lastDraw  = {0, 0};

    int getTermWidth() const {
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 10)
            return ws.ws_col;
        return 80;
    }

    long msSinceLastDraw() const {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        return (now.tv_sec  - _lastDraw.tv_sec)  * 1000
             + (now.tv_nsec - _lastDraw.tv_nsec) / 1000000;
    }
};
