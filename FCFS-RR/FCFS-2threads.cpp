#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <limits>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

// Define a struct to represent a Process
struct Process {
    string name;            // Name of the process
    int execution_time;     // Execution time of the process
    int arrival_time;       // Arrival time of the process
    int waiting_time;       // Amount of time the process has to wait before executing
    int turnaround_time;    // Total time taken for the process to complete
};

// Global variables
int n;                                  // Number of processes
vector<Process> processes;              // Vector to store processes
vector<Process> sorted_processes;       // Vector to store sorted processes
mutex mtx;                              // Mutex for synchronization
condition_variable cv;                  // Condition variable for synchronization
bool input_completed = false;           // Flag to indicate completion of user input

// Function for user input
void userInput() {
    // Read the number of processes
    cout << "Enter the number of processes:" << endl;
    cin >> n;

    processes.resize(n);
    sorted_processes.resize(n);

    // Get process information from input
    cout << "Enter the processes details in this format:" << endl;
    cout << "Process-Name Execution-Time Arrival-Time:" << endl;
    for (int i = 0; i < n; i++) {
        cout << "Process" << i+1 << ": ";
        string name;
        int execution_time, arrival_time;
        cin >> name >> execution_time >> arrival_time;
        processes[i].name = name;
        processes[i].execution_time = execution_time;
        processes[i].arrival_time = arrival_time;
        processes[i].waiting_time = numeric_limits<int>::max() / 2;
        processes[i].turnaround_time = 0;
    }

    // Sort the processes based on arrival time
    sorted_processes = processes;
    sort(sorted_processes.begin(), sorted_processes.end(), [](const Process &a, const Process &b) {
        return a.arrival_time < b.arrival_time;
    });

    // Notify the other thread that user input is completed
    {
        lock_guard<mutex> lock(mtx);
        input_completed = true;
    }
    cv.notify_one();
}

// Function to perform calculations
void performCalculations() {
    // Wait for user input to be completed
    {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, []{ return input_completed; });
    }

    int current_time = 0;
    int total_waiting_time = 0;
    int total_turnaround_time = 0;

    // Loop through the sorted processes and calculate waiting time and turnaround time
    for (auto &process : sorted_processes) {
        // Calculate waiting time
        if (process.waiting_time == numeric_limits<int>::max() / 2) {
            if (current_time < process.arrival_time) {
                process.waiting_time = process.arrival_time - current_time;
                current_time = process.arrival_time;
            } else {
                process.waiting_time = current_time - process.arrival_time;
            }
        }
        // Execute the process
        current_time += process.execution_time;
        // Calculate turnaround time
        process.turnaround_time = current_time - process.arrival_time;
        // Add times to the total times
        total_waiting_time += process.waiting_time;
        total_turnaround_time += process.turnaround_time;
    }

    double n_double = static_cast<double>(n);
    // Calculate and print the average waiting time and turnaround time
    double awt = static_cast<double>(total_waiting_time) / n_double;
    double att = static_cast<double>(total_turnaround_time) / n_double;
    cout << "Average-Waiting-Time: " << fixed << setprecision(2) << awt << endl;
    cout << "Average-Turnaround-Time: " << fixed << setprecision(2) << att << endl;
}

int main() {
    // Create two threads
    thread input_thread(userInput);
    thread calculations_thread(performCalculations);

    // Join the threads
    input_thread.join();
    calculations_thread.join();

    return 0;
}
