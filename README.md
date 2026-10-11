# Sentinel

**A native Windows system monitoring application built with C++20 and Qt 6.**

Sentinel provides a desktop interface for monitoring CPU utilization, physical memory usage, and running processes. It includes process search, sortable resource columns, performance history charts, and basic system diagnostics.

## Features

- **System overview** — CPU utilization, memory usage, and process count.
- **Process manager** — Inspect running processes, search by name or PID, and sort by CPU, memory, or process ID.
- **End Task** — Request process termination with a confirmation prompt.
- **Performance history** — View recent CPU and memory utilization samples.
- **Diagnostics** — Review current monitoring readings and update status.
- **Native desktop UI** — Built with Qt 6 for Windows.

## Technology

- C++20
- Qt 6 Widgets
- CMake
- Windows API
- Windows Performance Data Helper (PDH)
- Windows Process Status API (PSAPI)

## Requirements

- Windows 10 or Windows 11
- A compatible C++20 compiler
- CMake 3.25 or newer
- Qt 6 Widgets
- Ninja or another supported CMake build tool

## Build from Source

Clone the repository:

```powershell
git clone https://github.com/egeexcworks/Sentinel.git
cd Sentinel
```

Configure the project using your Qt installation:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64
```

Build:

```powershell
cmake --build build
```

Run with the MSYS2 UCRT64 runtime libraries available:

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
.\build\Sentinel.exe
```

Adjust the Qt path if your installation differs.

## Platform

Sentinel currently targets Windows. Some processes may require elevated permissions to inspect or terminate.

**Warning:** Terminating a process can cause unsaved work to be lost or destabilize Windows. Only terminate processes you understand.

## Project Status

Sentinel is under active development. Features and compatibility may change between releases.

## License

See [LICENSE](LICENSE) for licensing terms.

## Author

Developed by [egeexcworks](https://github.com/egeexcworks).