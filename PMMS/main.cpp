/*
Principles of Operating Systems, Dr. Bejani
Parallel Matrix Multiplication Server (Project 2)
Pouria Alimoradpor 9912035
*/

#include "server.h"

int main() {
    Server server;
    server.start();
    return 0;
}

/* Description

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

    Bonus section
    Implement a dynamic worker allocation system:
    1. Provide an API that can increase or decrease the number of Worker processes at runtime.
    2. The server must dynamically distribute the matrix multiplication tasks among the available workers.
*/
