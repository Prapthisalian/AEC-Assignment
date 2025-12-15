cmake_minimum_required(VERSION 3.25)
project(MentalHealthTracker)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

# --- Fetch Asio (Dependency for Crow) ---
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-30-2 
)
FetchContent_MakeAvailable(asio)

# CRITICAL: Set this variable so Crow's find_package(asio) succeeds.
# FetchContent_MakeAvailable sets asio_SOURCE_DIR.
set(ASIO_INCLUDE_DIR ${asio_SOURCE_DIR}/asio/include CACHE INTERNAL "Asio Include Dir")

# --- Fetch Crow ---
FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG master
)
FetchContent_MakeAvailable(Crow)

# --- Fetch nlohmann_json ---
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# --- Fetch CPR ---
FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.10.5
)
FetchContent_MakeAvailable(cpr)

# --- Fetch SQLite3 ---
FetchContent_Declare(
    sqlite3
    GIT_REPOSITORY https://github.com/azadkuh/sqlite-amalgamation.git
    GIT_TAG master
)
FetchContent_MakeAvailable(sqlite3)
# Workaround: Upstream CMake might not export BUILD_INTERFACE include dir correctly
# target_include_directories(SQLite3 INTERFACE $<BUILD_INTERFACE:${sqlite3_SOURCE_DIR}>) # Caused export error

add_executable(tracker main.cpp)
target_include_directories(tracker PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${sqlite3_SOURCE_DIR})

# Link libraries
# Note: Crow::Crow includes asio if found, but we link asio explicitly implicitly via Crow or directly if needed.
# Since we define 'asio' target and set the include dir, Crow should be happy.
target_link_libraries(tracker PRIVATE Crow::Crow nlohmann_json::nlohmann_json cpr::cpr SQLite3 ws2_32 mswsock)
