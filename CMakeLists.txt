cmake_minimum_required(VERSION 3.25)

project(Sentinel
    VERSION 0.3.0
    DESCRIPTION "Native Windows system monitoring and diagnostics"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(Sentinel
    src/main.cpp
    src/monitoring/SystemMonitor.h
    src/monitoring/SystemMonitor.cpp
    src/processes/ProcessMonitor.h
    src/processes/ProcessMonitor.cpp
)

target_include_directories(Sentinel PRIVATE src)

target_compile_definitions(Sentinel PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

target_compile_options(Sentinel PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_link_libraries(Sentinel PRIVATE
    Qt6::Widgets
    pdh
    psapi
)