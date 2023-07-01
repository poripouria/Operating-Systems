/*
Principles of Operating Systems, Dr. Bejani
Parallel matrix multiplication server (Project 2)
Pouria Alimoradpor 9912035
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <pthread.h>

#define Port 8080
#define BUFFER_SIZE 1024
#define MATRIX_SIZE 1000
#define NUM_TASKS (MATRIX_SIZE * MATRIX_SIZE)
#define NUMBER_OF_WORKERS 4

typedef struct {
    int row;
    int col;
} Task;

void handle_client_request(int client_socket);

void worker_process(int worker_id, int read_fd, int write_fd, sem_t *task_semaphore);

void matrix_multiply(Task task);

void collect_results();

void return_matrix_as_http_response();

void increase_workers();

void decrease_workers();

int NUM_WORKERS = NUMBER_OF_WORKERS;
int matrix1[MATRIX_SIZE][MATRIX_SIZE];
int matrix2[MATRIX_SIZE][MATRIX_SIZE];
int result_matrix[MATRIX_SIZE][MATRIX_SIZE];
pthread_mutex_t result_mutex;
sem_t task_semaphore;

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_length;

    int num_workers = 4; // Number of worker processes to create
    int server_to_worker[num_workers][2]; // Pipe for communication from server to worker
    int worker_to_server[num_workers][2]; // Pipe for communication from worker to server
    pid_t pid;

    sem_t task_semaphore;
    sem_init(&task_semaphore, 0, NUM_TASKS); // Initialize semaphore with the number of tasks

    // Initialize result mutex
    pthread_mutex_init(&result_mutex, NULL);

    // Initialize task semaphore
    sem_init(&task_semaphore, 0, NUM_TASKS);

    // Pipes and worker processes creation...

    int task_counter = 0;

    // Assign tasks to worker processes
    for (int i = 0; i < NUM_WORKERS; i++) {
        sem_wait(&task_semaphore); // Wait for an available task

        // Send the task information to the worker process
        write(server_to_worker[i][1], &task_counter, sizeof(task_counter));

        task_counter++;
    }

    // Create pipes for IPC
    for (int i = 0; i < num_workers; i++) {
        if (pipe(server_to_worker[i]) < 0 || pipe(worker_to_server[i]) < 0) {
            perror("Error creating pipe");
            exit(EXIT_FAILURE);
        }
    }

    // Create worker processes
    for (int i = 0; i < num_workers; i++) {
        pid = fork();
        if (pid < 0) {
            perror("Error creating worker process");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process (worker)
            close(server_to_worker[i][1]); // Close unused write end of server_to_worker pipe
            close(worker_to_server[i][0]); // Close unused read end of worker_to_server pipe
            worker_process(i, server_to_worker[i][0], worker_to_server[i][1]);
            exit(EXIT_SUCCESS);
        }
    }

    // Parent process (server)
    for (int i = 0; i < num_workers; i++) {
        close(server_to_worker[i][0]); // Close unused read end of server_to_worker pipe
        close(worker_to_server[i][1]); // Close unused write end of worker_to_server pipe
    }
    
    // Clean up and wait for worker processes to finish
    for (int i = 0; i < NUM_WORKERS; i++) {
        sem_post(&task_semaphore); // Release the semaphore for each completed task
    }

    for (int i = 0; i < NUM_WORKERS; i++) {
        wait(NULL); // Wait for worker processes to exit
    }

    sem_destroy(&task_semaphore); // Destroy the semaphore

    // Clean up
    for (int i = 0; i < num_workers; i++) {
        close(server_to_worker[i][1]); // Close write end of server_to_worker pipe
        close(worker_to_server[i][0]); // Close read end of worker_to_server pipe
    }

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }
 
    // Prepare the server address structure
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the specified port
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) < 0) {
        perror("Error listening for connections");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    // Accept client connections and handle requests
    while (1) {
        client_address_length = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_length);
        if (client_socket < 0) {
            perror("Error accepting connection");
            exit(EXIT_FAILURE);
        }

        printf("New client connected\n");

        // Handle client request
        handle_client_request(client_socket);

        close(client_socket);
        printf("Client disconnected\n");
    }

    // Wait for all worker processes to complete their tasks
    for (int i = 0; i < NUM_WORKERS; i++) {
        wait(NULL);
    }

    // Collect results and return the matrix
    collect_results();
    return_matrix_as_http_response();

    close(server_socket);
    return 0;
}

void handle_client_request(int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Read the HTTP request from the client
    bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("Error reading from client socket");
        exit(EXIT_FAILURE);
    }

    // Null-terminate the received data
    buffer[bytes_read] = '\0';

    printf("Received request:\n%s\n", buffer);

    // Parse the HTTP request to extract the matrices' data
    // TODO: Implement parsing logic here

    // Send HTTP response back to the client
    const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nHello World!";
    ssize_t bytes_written = write(client_socket, response, strlen(response));
    if (bytes_written < 0) {
        perror("Error writing to client socket");
        exit(EXIT_FAILURE);
    }
}

void worker_process(int worker_id, int read_fd, int write_fd, sem_t *task_semaphore) {
    int task_number;
    read(read_fd, &task_number, sizeof(task_number));

    Task task;
    task.row = task_number / MATRIX_SIZE;
    task.col = task_number % MATRIX_SIZE;

    // Perform the assigned task (matrix multiplication)
    matrix_multiply(task);

    // Release the semaphore to indicate task completion
    sem_post(task_semaphore);
    
    // ... Worker process logic ...

    // Close unused pipe ends
    close(read_fd); // Close read end of server_to_worker pipe
    close(write_fd); // Close write end of worker_to_server pipe

    // ... Worker process implementation ...

    read(read_fd, &task_number, sizeof(task_number));

    // Perform the assigned task (e.g., matrix multiplication)

    // Release the semaphore to indicate task completion
    sem_post(task_semaphore);
}

void matrix_multiply(Task task) {
    int row = task.row;
    int col = task.col;

    int sum = 0;
    for (int k = 0; k < MATRIX_SIZE; k++) {
        sum += matrix1[row][k] * matrix2[k][col];
    }

    // Acquire the result mutex to update the result matrix
    pthread_mutex_lock(&result_mutex);

    result_matrix[row][col] = sum;

    // Release the result mutex
    pthread_mutex_unlock(&result_mutex);
}

void collect_results() {
    // No need for mutex here since the server is the only one accessing the result matrix
    // But you can use the result mutex for consistency if needed

    // Collect the results from each worker process
    // and construct the final result matrix
    // Here's an example of collecting results row by row
    int task_counter = 0;
    for (int row = 0; row < MATRIX_SIZE; row++) {
        for (int col = 0; col < MATRIX_SIZE; col++) {
            result_matrix[row][col] = worker_results[task_counter];
            task_counter++;
        }
    }
}

void return_matrix_as_http_response() {
    // Return the result matrix as an HTTP response to the client
    // You can format the matrix as desired in the HTTP response body
    // Here's an example of printing the matrix to the console
    for (int row = 0; row < MATRIX_SIZE; row++) {
        for (int col = 0; col < MATRIX_SIZE; col++) {
            printf("%d ", result_matrix[row][col]);
        }
        printf("\n");
    }
}

void increase_workers() {
    // Increase the number of worker processes at runtime
    // You can create additional worker processes using fork() or another process creation mechanism
    // Update the NUM_WORKERS variable accordingly
}

void decrease_workers() {
    // Decrease the number of worker processes at runtime
    // You can terminate worker processes or set a flag to indicate their termination
    // Update the NUM_WORKERS variable accordingly
}

/* 1. Implement the HTTP server:
- Create a socket and bind it to a specific port.
- Listen for incoming connections from clients.
- Accept client connections and handle HTTP requests.
- Parse the HTTP request to extract the matrices' data.
-------------------------------------------------------------------------
In this example, we create a TCP socket, bind it to the specified port,
and listen for incoming connections. When a client connects, we accept
the connection and handle the client's HTTP request. The request is read 
from the socket, and you can implement the logic to parse the HTTP 
request and extract the matrices' data. After parsing, you can send 
an HTTP response back to the client.
Please note that this is a basic example, and you'll need to expand on 
it to handle different types of HTTP requests, error conditions, and 
implement the matrix multiplication logic as well as the synchronization 
mechanisms.
*/

/* 2. Implement the worker processes:
- Create multiple worker processes using fork() or another process 
  creation mechanism.
- Establish IPC channels (such as pipes or shared memory) between the
  server and worker processes.
-------------------------------------------------------------------------
In this example, we create multiple worker processes using fork() and 
establish IPC channels between the server and worker processes using 
pipes. The number of worker processes is specified by num_workers. Each 
worker process is assigned a unique worker_id for identification purposes. 
The server and worker processes communicate through the server_to_worker 
and worker_to_server pipes.
In the main server process, we create the pipes for IPC, create the worker 
processes in a loop, and then close the unused ends of the pipes. In each 
worker process, we close the unused ends of the pipes as well.
Please note that this is a basic example, and you'll need to expand on it
to implement the actual matrx multiplication tasks, handle synchronization 
using semaphores and mutex locks, and manage the distribution of work
between the workers.
*/

/* 3. Implement task distribution:
- Use semaphores to manage the distribution of work between worker 
processes.
- The server should assign each worker process a specific task (e.g., a 
row from the first matrix and a column from the second matrix).
-------------------------------------------------------------------------
In this example, we introduce a task_semaphore semaphore that manages the
availability of tasks. We initialize the semaphore with the total number 
of tasks (NUM_TASKS). Each worker process waits for an available task by 
calling sem_wait() on the semaphore. Once a task becomes available, the 
server sends the task information (in this case, the task_counter) to the
respective worker process through the pipe. After completing the assigned
task, the worker process calls sem_post() to release the semaphore and
indicate task completion.
Please note that you will need to adjust the code according to your
specific matrix multiplication logic and how you want to distribute the
tasks among the workers.
*/

/* 4. Implement matrix multiplication:
- Each worker process should perform matrix multiplication on the assigned
task.
- Use mutex locks to ensure that only one worker process updates a
particular cell of the result matrix at a time.
-------------------------------------------------------------------------
In this example, we introduce a result_mutex of type pthread_mutex_t to 
ensure exclusive access to the result matrix. Before updating the result 
matrix, each worker process acquires the result mutex using 
pthread_mutex_lock(), and once the update is complete, it releases the 
mutex using pthread_mutex_unlock().
Please note that you'll need to adjust the code according to your 
specific matrix size and the actual matrix data you receive from the 
HTTP server. Additionally, make sure to initialize and destroy the result
mutex properly, and handle any potential errors that may occur.
*/

/* 5. Collect results and return the matrix:
- The server should wait for all worker processes to complete their tasks.
Collect the results from each worker and construct the final result matrix.
- Send the result matrix back to the client as an HTTP response.
-------------------------------------------------------------------------
In this example, the collect_results() function collects the results from
each worker process and constructs the final result matrix. The 
return_matrix_as_http_response() function sends the result matrix back to
the client as an HTTP response. Adjust the code according to your 
specific requirements, such as formatting the matrix in the HTTP response
body.
*/

/* 6. Implement the dynamic worker allocation system (bonus):
- Provide an API to increase or decrease the number of worker processes.
- Dynamically distribute the matrix multiplication tasks among the 
available workers.
-------------------------------------------------------------------------
For the dynamic worker allocation system, you can implement the
increase_workers() and decrease_workers() functions to create additional
 worker processes or terminate existing worker processes, respectively. 
 Update the NUM_WORKERS variable accordingly. You may need to manage the 
 task distribution and synchronization among the available workers when 
 the number of workers changes.
*/
/*
    The main goal of this exercise was to gain hands-on experience with inter-process communication (IPC), semaphores, and mutexes by implementing a simple HTTP server that distributes matrix multiplication tasks between worker processes in parallel.

    - Question

    You need to develop a simple HTTP server that receives matrix multiplication requests. The server must launch a certain number of worker processes to perform matrix multiplication tasks in parallel. Server must communicate with Worker processes using IPC mechanisms. Task distribution should be managed using semaphores, and data synchronization should be ensured using mutex locks.

    - Project details

    server
    1. The server must provide an HTTP API that takes two matrices as input from client (first recive matrix1 then matrix2).
    2. During startup, the server must launch a certain number of Worker processes to perform matrix multiplication tasks.
    3. The server must distribute multiplication tasks between Worker processes using IPⅭ mechanisms.
    4. The server must collect the results from the Worker processes and return the result matrix to the client.

    Mutex
    Implement mutex locks to ensure that only one worker process updates a particular cell of the result matrix at a time.

    semaphore
    Use a semaphore to manage the distribution of work between worker processes. Each worker must send a signal when it has completed a task, and the server must wait until a worker is available.

    Work distribution
    The server must distribute the rows of the first matrix and the columns of the second matrix among the Worker processes for multiplication. Each Worker must calculate the dot product of one row of the first matrix and one column of the second matrix.

    Bonus section (Necessary)
    Implement a dynamic worker allocation system:
    1. Provide an API that can increase or decrease the number of Worker processes at runtime.
    2. The server must dynamically distribute the matrix multiplication tasks among the available workers.
*/
/* 1. Implement the HTTP server:
- Create a socket and bind it to a specific port.
- Listen for incoming connections from clients.
- Accept client connections and handle HTTP requests.
- Parse the HTTP request to extract the matrices' data.
-------------------------------------------------------------------------
In this example, we create a TCP socket, bind it to the specified port,
and listen for incoming connections. When a client connects, we accept
the connection and handle the client's HTTP request. The request is read 
from the socket, and you can implement the logic to parse the HTTP 
request and extract the matrices' data. After parsing, you can send 
an HTTP response back to the client.
Please note that this is a basic example, and you'll need to expand on 
it to handle different types of HTTP requests, error conditions, and 
implement the matrix multiplication logic as well as the synchronization 
mechanisms.
*/

/* 2. Implement the worker processes:
- Create multiple worker processes using fork() or another process 
  creation mechanism.
- Establish IPC channels (such as pipes or shared memory) between the
  server and worker processes.
-------------------------------------------------------------------------
In this example, we create multiple worker processes using fork() and 
establish IPC channels between the server and worker processes using 
pipes. The number of worker processes is specified by num_workers. Each 
worker process is assigned a unique worker_id for identification purposes. 
The server and worker processes communicate through the server_to_worker 
and worker_to_server pipes.
In the main server process, we create the pipes for IPC, create the worker 
processes in a loop, and then close the unused ends of the pipes. In each 
worker process, we close the unused ends of the pipes as well.
Please note that this is a basic example, and you'll need to expand on it
to implement the actual matrx multiplication tasks, handle synchronization 
using semaphores and mutex locks, and manage the distribution of work
between the workers.
*/
