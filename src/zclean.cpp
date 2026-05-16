#include "main.h"

using namespace std;


const string LOG_PATH = "/tmp/zclean.log";

volatile sig_atomic_t g_interrupted = 0;
void handleSigint(int) { g_interrupted = 1; }

// ─── progress bar ─────────────────────────────────────────────────────────────
void drawGlobalBar(float pct, const string& task) {
    struct winsize w;
    int termWidth = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
        termWidth = w.ws_col;

    const int barWidth = max(10, min(40, termWidth / 3));
    const int visualPrefixLen = 26 + barWidth;
    const int taskMaxLen = max(1, termWidth - visualPrefixLen);

    string taskTrimmed = task;
    taskTrimmed.erase(remove(taskTrimmed.begin(), taskTrimmed.end(), '\n'), taskTrimmed.end());
    if ((int)taskTrimmed.size() > taskMaxLen)
        taskTrimmed = taskTrimmed.substr(0, taskMaxLen - 1) + "~";

    int percent = max(0, min(100, (int)pct));
    int pos = barWidth * percent / 100;

    cout << "\r\033[K" << YELLOW << "Clean Progress: [" << RESET;
    for (int i = 0; i < barWidth; ++i)
        cout << (i < pos ? GREEN + "#" + RESET : " ");
    cout << YELLOW << "] " << setw(3) << percent << "% " << RESET << "| " << taskTrimmed << "\033[K" << flush;
}

// ─── progress render thread ───────────────────────────────────────────────────
struct BarState {
    atomic<float> progress{0.0f};
    atomic<bool>  running{true};
    string        label;
};

static void* barThread(void* arg) {
    BarState* s = static_cast<BarState*>(arg);
    while (s->running.load()) {
        drawGlobalBar(s->progress.load(), s->label);
        usleep(80000);
    }
    return nullptr;
}

// ─── run a single apt task with progress ─────────────────────────────────────
int runAptTask(const string& label, const vector<const char*>& args,
               BarState& bs, float startRange, float endRange) {

    int pfd[2];
    if (pipe(pfd) != 0) return 1;

    pid_t pid = fork();
    if (pid == 0) {
        close(pfd[0]);
        dup2(pfd[1], 3);
        close(pfd[1]);
        int logfd = open(LOG_PATH.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (logfd >= 0) { dup2(logfd, 1); dup2(logfd, 2); close(logfd); }
        setenv("DEBIAN_FRONTEND", "noninteractive", 1);

        vector<const char*> argv = {"apt-get", "-y", "-o", "APT::Status-Fd=3"};
        for (auto a : args) argv.push_back(a);
        argv.push_back(nullptr);
        execvp("apt-get", const_cast<char* const*>(argv.data()));
        _exit(127);
    }
    if (pid < 0) { close(pfd[0]); close(pfd[1]); return 1; }
    close(pfd[1]);

    bs.label = label + ": preparing...";

    FILE* f = fdopen(pfd[0], "r");
    if (f) setvbuf(f, nullptr, _IONBF, 0);
    char buf[512];

    while (f && fgets(buf, sizeof(buf), f)) {
        if (g_interrupted) {
            fclose(f); kill(pid, SIGTERM); waitpid(pid, nullptr, 0); return 130;
        }
        string line = buf;
        if (line.rfind("pmstatus:", 0) == 0 || line.rfind("dlstatus:", 0) == 0) {
            size_t c1 = line.find(':');
            size_t c2 = line.find(':', c1 + 1);
            size_t c3 = line.find(':', c2 + 1);
            if (c3 != string::npos) {
                float pct = 0.0f;
                try { pct = stof(line.substr(c2 + 1, c3 - c2 - 1)); } catch (...) {}
                float gp = startRange + (pct / 100.0f) * (endRange - startRange);
                if (gp > bs.progress.load()) bs.progress = gp;
                string msg = line.substr(c3 + 1);
                msg.erase(msg.find_last_not_of(" \n\r\t") + 1);
                if (msg.size() > 48) msg = msg.substr(0, 48);
                bs.label = label + ": " + msg;
            }
        }
    }
    if (f) fclose(f);

    int wst = 0;
    waitpid(pid, &wst, 0);
    return WIFEXITED(wst) ? WEXITSTATUS(wst) : 1;
               }

               // ─── run a flatpak task (no Status-Fd, just fake smooth progress) ─────────────
               int runFlatpakTask(const string& label, const vector<const char*>& args,
                                  BarState& bs, float startRange, float endRange) {

                   pid_t pid = fork();
                   if (pid == 0) {
                       int logfd = open(LOG_PATH.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
                       if (logfd >= 0) { dup2(logfd, 1); dup2(logfd, 2); close(logfd); }

                       vector<const char*> argv = {"flatpak"};
                       for (auto a : args) argv.push_back(a);
                       argv.push_back(nullptr);
                       execvp("flatpak", const_cast<char* const*>(argv.data()));
                       _exit(127);
                   }
                   if (pid < 0) return 1;

                   bs.label = label + ": running...";

                   // Flatpak nie ma Status-Fd — animujemy pasek płynnie aż do zakończenia procesu
                   float fakeProgress = startRange;
                   const float slowdownAt = startRange + (endRange - startRange) * 0.85f;

                   while (true) {
                       if (g_interrupted) {
                           kill(pid, SIGTERM); waitpid(pid, nullptr, 0); return 130;
                       }

                       int wst = 0;
                       pid_t r = waitpid(pid, &wst, WNOHANG);
                       if (r == pid) {
                           bs.progress = endRange;
                           return WIFEXITED(wst) ? WEXITSTATUS(wst) : 1;
                       }


                       float increment = (fakeProgress < slowdownAt) ? 0.4f : 0.05f;
                       fakeProgress = min(fakeProgress + increment, endRange - 0.5f);
                       if (fakeProgress > bs.progress.load()) bs.progress = fakeProgress;

                       usleep(80000);
                   }
                                  }

                                  // ─── main ─────────────────────────────────────────────────────────────────────
                                  int main(int argc, char* argv[]) {
                                      signal(SIGINT, handleSigint);
                                      setvbuf(stdout, nullptr, _IONBF, 0);
                                      zpm_update::checkForUpdates();

                                      bool Help = false;
                                      bool Version = false;
                                      for (int i = 1; i < argc; ++i) {
                                          string arg = argv[i];
                                          if (arg == "--version" || arg == "-v") Version = true;
                                          if (arg == "--help"    || arg == "-h") Help    = true;
                                      }

                                      if (Version && Help) {
                                          cout << YELLOW << "--version" << RESET << "\n";
                                          cout << RED << "zclean component version: 1.2 of ZPM\n" << RESET;
                                          cout << "https://github.com/Ignacyyy/ZPM\n";
                                          cout << "Copyright (c) 2026 Ignacyyy\nLicense: MIT\n\n";
                                          cout << YELLOW << "--help\n" << RESET;
                                          cout << RED << "Usage: " << RESET << argv[0] << " [options] or zpm clean [options]\n\n";
                                          cout << RED << "Options:\n" << RESET;
                                          cout << "  --version, -v  Show version information\n";
                                          cout << "  --help,    -h  Show this help message\n\n";
                                          return 0;
                                      }
                                      if (Version) {
                                          cout << RED << "zclean component version: 1.2 of ZPM\n" << RESET;
                                          cout << "https://github.com/Ignacyyy/ZPM\n";
                                          cout << "Copyright (c) 2026 Ignacyyy\nLicense: MIT\n";
                                          return 0;
                                      }
                                      if (Help) {
                                          cout << RED << "Usage: " << RESET << argv[0] << " [options] or zpm clean [options]\n\n";
                                          cout << RED << "Options:\n" << RESET;
                                          cout << "  --version, -v  Show version information\n";
                                          cout << "  --help,    -h  Show this help message\n\n";
                                          return 0;
                                      }

                                      if (geteuid() != 0) {
                                          cout << RED << "Run with sudo!\n" << RESET;
                                          return 1;
                                      }

                                      // Sprawdź czy flatpak jest zainstalowany
                                      bool hasFlatpak = (system("command -v flatpak >/dev/null 2>&1") == 0);

                                      { ofstream log(LOG_PATH, ios::trunc); }

                                      struct Task {
                                          string              label;
                                          vector<const char*> args;
                                          bool                isFlatpak{false};
                                      };

                                      vector<Task> tasks = {
                                          { "Autoremove", {"autoremove"}, false },
                                          { "Clean",      {"clean"},      false },
                                          { "Autoclean",  {"autoclean"},  false },
                                      };

                                      if (hasFlatpak) {
                                          tasks.push_back({ "Flatpak uninstall unused", {"uninstall", "--unused", "-y"}, true });
                                      }

                                      int   total = static_cast<int>(tasks.size());
                                      float step  = 100.0f / total;

                                      cout << RED << "Cleaning system cache and unused packages..." << RESET << "\n";

                                      cout << "\n";

                                      BarState bs;
                                      bs.progress = 0.0f;
                                      bs.label    = "Starting...";
                                      pthread_t tid;
                                      pthread_create(&tid, nullptr, barThread, &bs);

                                      vector<pair<string,bool>> results;
                                      bool anyFailed = false;

                                      for (int i = 0; i < total; ++i) {
                                          if (g_interrupted) {
                                              bs.running = false; pthread_join(tid, nullptr);
                                              cout << "\n" << YELLOW << "Cancelled by user (Ctrl+C).\n" << RESET;
                                              return 130;
                                          }

                                          float startRange = step * i;
                                          float endRange   = step * (i + 1);

                                          if (startRange > bs.progress.load()) bs.progress = startRange;

                                          int rc;
                                          if (tasks[i].isFlatpak)
                                              rc = runFlatpakTask(tasks[i].label, tasks[i].args, bs, startRange, endRange);
                                          else
                                              rc = runAptTask(tasks[i].label, tasks[i].args, bs, startRange, endRange);

                                          if (rc == 130) {
                                              bs.running = false; pthread_join(tid, nullptr);
                                              cout << "\n" << YELLOW << "Cancelled.\n" << RESET;
                                              return 130;
                                          }

                                          bs.progress = endRange;

                                          bool ok = (rc == 0);
                                          results.push_back({tasks[i].label, ok});
                                          if (!ok) anyFailed = true;
                                      }

                                      bs.progress = 100.0f;
                                      bs.label    = "Done!";
                                      usleep(100000);
                                      bs.running = false;
                                      pthread_join(tid, nullptr);
                                      drawGlobalBar(100.0f, "Done!");
                                      cout << "\n\n";

                                      for (const auto& r : results) {
                                          if (r.second)
                                              cout << r.first << ": done." << RESET << "\n";
                                          else
                                              cout << RED << r.first << ": failed." << RESET << "\n";
                                      }

                                      if (anyFailed) {
                                          cout << RED    << "\nCleanup finished with errors!\n" << RESET;
                                          cout << YELLOW << "Check " << LOG_PATH << " for details.\n" << RESET;
                                          return 1;
                                      }

                                      cout << GREEN << "\nSystem cleanup complete!\n" << RESET;
                                      return 0;
                                  }
