#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

// Define a struct to store process information
struct Process {
    string name;            // Name of the process
    int execution_time;     // Execution time of the process
    int arrival_time;       // Arrival time of the process
    int waiting_time;       // Amount of time the process has to wait before executing
    int turnaround_time;    // Total time taken for the process to complete
    int remaining_time;     // Total remaining time for the process to complete
    int quantum;            // Time quantum for the process (Same for all processes in this homework)
};

// Global variables
int n;                                  // Number of processes
int quantum;                            // Time quantum for round-robin scheduling
queue<Process> processes;               // Queue to store processes
mutex mtx;                              // Mutex for synchronization
condition_variable cv;                  // Condition variable for synchronization
bool input_completed = false;           // Flag to indicate completion of user input

// Function for user input
void userInput() {
    // Read the number of processes
    cout << "Enter the number of processes:" << endl;
    cin >> n;

    // Get time quantum for RR scheduling algorithm
    cout << "Enter the value of time quantum:" << endl;
    cin >> quantum;

    // Get process information from input
    cout << "Enter the processes details in this format:" << endl;
    cout << "Process-Name Execution-Time Arrival-Time:" << endl;
    for (int i = 0; i < n; i++) {
        cout << "Process" << i + 1 << ": ";
        string name;
        int execution_time, arrival_time;
        cin >> name >> execution_time >> arrival_time;
        processes.push(Process{name, execution_time, arrival_time, INT_MAX / 2, 0, execution_time, quantum});
    }

    // Notify the other thread that user input is completed
    {
        lock_guard<mutex> lock(mtx);
        input_completed = true;
    }
    cv.notify_one();
}

// Function to perform round-robin scheduling
void performScheduling() {
    // Wait for user input to be completed
    {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [] { return input_completed; });
    }

    vector<Process> temp_vector;
    while (!processes.empty()) {
        temp_vector.push_back(processes.front());
        processes.pop();
    }

    // Sort process by arrival time
    sort(temp_vector.begin(), temp_vector.end(), [](const Process& a, const Process& b) {
        return a.arrival_time < b.arrival_time;
    });

    for (const auto& process : temp_vector) {
        processes.push(process);
    }

    int current_time = 0;
    int total_waiting_time = 0;
    int total_turnaround_time = 0;

    // Schedule the processes using round-robin scheduling algorithm
    while (!processes.empty()) {
        Process current_process = processes.front();
        processes.pop();
        if (current_process.remaining_time <= quantum) {
            current_time += current_process.remaining_time;
            total_turnaround_time += current_time - current_process.arrival_time;
            total_waiting_time += current_time - current_process.arrival_time - current_process.execution_time;
            current_process.remaining_time = 0;
        } else {
            current_time += quantum;
            current_process.remaining_time -= quantum;
            processes.push(current_process);
        }
    }

    double awt = (double)total_waiting_time / n;
    double att = (double)total_turnaround_time / n;

    // Print the result
    cout << "Average-Waiting-Time: " << awt << endl;
    cout << "Average-Turnaround-Time: " << att << endl;
}

int main() {
    // Create two threads
    thread input_thread(userInput);
    thread scheduling_thread(performScheduling);

    // Join the threads
    input_thread.join();
    scheduling_thread.join();

    return 0;
}
