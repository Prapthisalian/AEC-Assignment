# Mental Health Diary Project

## Prerequisites

To run this application, you need the following installed on your system:

1.  **C++ Compiler**: You already have `g++` installed.
2.  **CMake**: This project uses CMake for building. Please install it from [cmake.org](https://cmake.org/download/).
    - During installation, make sure to select "Add CMake to the system PATH".
3.  **SQLite3**: You already have this installed.

## How to Run

### Option 1: Using the Helper Script (Recommended)

1.  Ensure you have installed **CMake**.
2.  Double-click `setup_and_run.bat` in this directory.
3.  This script will:
    - Build the backend.
    - Run the backend server.
4.  Once the server is running, open `frontend/index.html` in your web browser.

### Option 2: Manual Build

1.  Open a terminal in the `backend` directory.
2.  Run the following commands:
    ```bash
    mkdir build
    cd build
    cmake ..
    cmake --build .
    ```
3.  Run the generated executable (e.g., `.\Debug\tracker.exe` or `.\tracker.exe`).
4.  Open `frontend/index.html` in your web browser.

## Troubleshooting

-   **CMake not found**: Install CMake and add it to your PATH.
-   **Missing dependencies**: The build process automatically fetches `Crow`, `json`, and `cpr`. Ensure you have an internet connection during the first build.
