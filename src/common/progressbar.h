#pragma once
#include "main.h"

/**
 * ProgressBar.h — ZPM Universal Progress Bar
 *
 * Features:
 * - Adaptive width based on terminal size.
 * - ANSI color support (Yellow/Green).
 * - Throttled updates to prevent flickering.
 * - Clean line handling using carriage return (\r) and erase-in-line (\033[K).
 */

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
    string label        = "Progress";
    int    maxBarWidth  = 40;
    int    minBarWidth  = 10;
    long   throttleMs   = 150; // Minimum time between redraws

    /**
     * Renders the progress bar to the current terminal line.
     * @param pct Percentage (0.0 to 100.0)
     * @param task Short description of the current action
     */
    void draw(float pct, const string& task) {
        int termWidth = getTermWidth();

        int percent = static_cast<int>(pct);
        if (percent < 0)   percent = 0;
        if (percent > 100) percent = 100;

        // 1. Safety margin to prevent terminal auto-wrap (crucial for Termux/VSCode)
        int safeWidth = termWidth - 2;
        if (safeWidth < 20) safeWidth = 20;

        // 2. Calculate lengths of static elements
        // Format: "Label: [#########] 100% | Task"
        int labelLen  = (int)label.size() + 3; // "Label: ["
        int suffixLen = 10;                    // "] 100% | "

        // 3. Determine dynamic bar width
        int taskSpace = safeWidth / 3;
        int barWidth = safeWidth - labelLen - suffixLen - taskSpace;

        if (barWidth > maxBarWidth) barWidth = maxBarWidth;
        if (barWidth < minBarWidth) barWidth = minBarWidth;

        // 4. Truncate task description to ensure everything fits on ONE line
        int remainingForTask = safeWidth - labelLen - suffixLen - barWidth;
        string t = task;
        if ((int)t.size() > remainingForTask) {
            if (remainingForTask > 5) {
                t = t.substr(0, remainingForTask - 3) + "...";
            } else {
                t = t.substr(0, max(0, remainingForTask));
            }
        }

        // 5. OUTPUT CONSTRUCTION
        // \r returns the cursor to the start of the line
        cout << "\r";

        // Draw Label
        cout << colorYellow << label << ": [" << colorReset;

        // Draw Bar
        int pos = (barWidth * percent) / 100;
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) cout << colorGreen << "#" << colorReset;
            else cout << " ";
        }

        // Draw Suffix, Percentage, and Task
        // \033[K erases everything from the cursor to the end of the line (cleans old text)
        cout << colorYellow << "] " << percent << "% "
        << colorReset << "| " << t << "\033[K" << flush;

        _drawn = true;
        clock_gettime(CLOCK_MONOTONIC, &_lastDraw);
    }

    /**
     * Throttled version of draw(). Skips rendering if called too frequently.
     */
    bool drawThrottled(float pct, const string& task) {
        if (!_drawn || msSinceLastDraw() >= throttleMs) {
            draw(pct, task);
            return true;
        }
        return false;
    }

    /**
     * Finalizes the bar, forcing 100% and moving to a new line.
     */
    void finish(const string& finalTask = "Done!") {
        if (_drawn) {
            draw(100.0f, finalTask);
            cout << "\n" << flush;
        }
        _drawn = false;
    }

    /**
     * Reset the drawing state (useful when starting a new major operation).
     */
    void resetThrottle() {
        _drawn = false;
    }

private:
    bool            _drawn    = false;
    struct timespec _lastDraw  = {0, 0};

    /**
     * Fetches the current terminal width via ioctl.
     */
    int getTermWidth() const {
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
            return ws.ws_col;
        return 80; // Fallback
    }

    /**
     * Calculates milliseconds since the last draw call.
     */
    long msSinceLastDraw() const {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        return (now.tv_sec  - _lastDraw.tv_sec)  * 1000
        + (now.tv_nsec - _lastDraw.tv_nsec) / 1000000;
    }
};
