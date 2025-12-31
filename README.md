# Bounded Buffer

This is a thread safe implementation of a bounded buffer. The producer thread
pushes data to the buffer if there is capacity. The consumer thread reads from
the buffer if its not empty. The producer and consumer runs continuously until
a SIGINT is received. At this point, the producer stops pushing data to the
buffer. The consumer continues until is reads all the data until the buffer is
empty thus ensuring a graceful shutdown.

## Build instructions

This is a simple C++ project that uses CMake and Ninja for building.

## Prerequisites

- C++17 compiler (like GCC, Clang, or MSVC)
- CMake (version 3.10 or later)
- Ninja

## How to Compile and Run

1.  **Create a build directory:**
    ```bash
    mkdir build
    ```

2.  **Navigate to the build directory:**
    ```bash
    cd build
    ```

3.  **Run CMake to configure the project:**
    ```bash
    cmake -G Ninja ..
    ```

4.  **Build the project using Ninja:**
    ```bash
    ninja
    ```

5.  **Run the executable:**
    ```bash
    ./boundbuf
    ```
