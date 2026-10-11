cmake_minimum_required(VERSION 3.25)

project(Sentinel
VERSION 0.1.0
DESCRIPTION "Native Windows system monitoring and diagnostics"
LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(Sentinel
src/main.cpp

# System monitoring
src/monitoring/SystemMonitor.h
src/monitoring/SystemMonitor.cpp

# Process monitoring
src/processes/ProcessMonitor.h
src/processes/ProcessMonitor.cpp

)

target_include_directories(Sentinel PRIVATE
src
src/monitoring
src/processes
)

target_compile_options(Sentinel PRIVATE
-Wall
-Wextra
-Wpedantic
)

if(WIN32)
target_compile_definitions(Sentinel PRIVATE
NOMINMAX
WIN32_LEAN_AND_MEAN
)

target_link_libraries(Sentinel PRIVATE
    pdh
    psapi
)

endif()