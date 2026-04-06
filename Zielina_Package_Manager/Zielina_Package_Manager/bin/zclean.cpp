#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <cstdlib>
#include <unistd.h>
#include <vector>

using namespace std;

mutex consoleMutex;
int progressPercent = 0;

const string GREEN  = "\033[32m";
const string RED    = "\033[31m";
const string RESET  = "\033[0m";

struct Task {
    string name;
    string command;
};

void showProgress(int totalTasks, int& completedTasks) {
    char spinner[] = {'|','/','-','\\'};
    int spinIndex = 0;
    int width = 50;
    while (completedTasks < totalTasks) {
        {
            lock_guard<mutex> lock(consoleMutex);
            int pos = (progressPercent * width) / 100;
            cout << "\r" << GREEN
                 << "Progress: ["
                 << string(pos,'=')
                 << string(width - pos,' ')
                 << "] " << progressPercent << "% "
                 << spinner[spinIndex]
                 << RESET << flush;
        }
        spinIndex = (spinIndex +1)%4;
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    {
        lock_guard<mutex> lock(consoleMutex);
        cout << "\r" << GREEN
             << "Progress: [" << string(width,'=') << "] 100%"
             << RESET << endl;
    }
}

int main() {
    if (geteuid() != 0) {
        cout << RED << "Run with sudo!\n" << RESET;
        return 1;
    }

    cout << GREEN << "Cleaning system cache and unused packages..." << RESET << endl << endl;

    vector<Task> tasks = {
        {"Autoremove","apt autoremove -y"},
        {"Clean","apt clean"},
        {"Autoclean","apt autoclean"}
    };

    int totalTasks = tasks.size();
    int completedTasks = 0;

    thread progressThread(showProgress,totalTasks,ref(completedTasks));

    for(auto& task : tasks){
        string fullCmd = task.command + " > /dev/null 2>&1";
        system(fullCmd.c_str());
        completedTasks++;
        progressPercent = (completedTasks*100)/totalTasks;
    }

    progressPercent = 100;
    progressThread.join();

    cout << endl;
    for(auto& t : tasks)
        cout << GREEN << "Running '" << t.command << "' done." << RESET << endl;

    cout << RED << "System cleanup complete!" << RESET << endl;

    return 0;
}