
#include "ProcessMonitor.h"

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>

namespace
{
std::uint64_t toUint64(const FILETIME& time)
{
    ULARGE_INTEGER value{};
    value.LowPart = time.dwLowDateTime;
    value.HighPart = time.dwHighDateTime;
    return value.QuadPart;
}

std::string toUtf8(const wchar_t* text)
{
    if (text == nullptr || *text == L'\0')
        return {};

    const int required = WideCharToMultiByte(
        CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);

    if (required <= 1)
        return {};

    std::string result(static_cast<std::size_t>(required), '\0');

    const int written = WideCharToMultiByte(
        CP_UTF8, 0, text, -1, result.data(), required,
        nullptr, nullptr);

    if (written == 0)
        return {};

    result.pop_back();
    return result;
}

bool readSystemCpuTime(std::uint64_t& total)
{
    FILETIME idle{}, kernel{}, user{};

    if (!GetSystemTimes(&idle, &kernel, &user))
        return false;

    // Windows kernel time includes idle time.
    total = toUint64(kernel) + toUint64(user);
    return true;
}

bool readProcessCpuTime(HANDLE process, std::uint64_t& total)
{
    FILETIME creation{}, exit{}, kernel{}, user{};

    if (!GetProcessTimes(
            process, &creation, &exit, &kernel, &user))
        return false;

    total = toUint64(kernel) + toUint64(user);
    return true;
}
}

std::vector<ProcessInfo> ProcessMonitor::getProcesses()
{
    std::vector<ProcessInfo> processes;
    std::unordered_map<unsigned long, std::uint64_t> currentCpuTimes;

    std::uint64_t currentSystemTime = 0;
    const bool haveSystemTime = readSystemCpuTime(currentSystemTime);

    HANDLE snapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS, 0);

    if (snapshot == INVALID_HANDLE_VALUE)
        return processes;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            ProcessInfo info;
            info.processId = entry.th32ProcessID;
            info.name = toUtf8(entry.szExeFile);

            HANDLE process = OpenProcess(
                PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
                FALSE,
                entry.th32ProcessID);

            if (process != nullptr)
            {
                PROCESS_MEMORY_COUNTERS_EX memory{};
                memory.cb = sizeof(memory);

                if (GetProcessMemoryInfo(
                        process,
                        reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory),
                        sizeof(memory)))
                {
                    info.memoryBytes =
                        static_cast<std::uint64_t>(
                            memory.WorkingSetSize);
                }

                std::uint64_t processTime = 0;

                if (readProcessCpuTime(process, processTime))
                    currentCpuTimes[info.processId] = processTime;

                CloseHandle(process);
            }

            processes.push_back(std::move(info));

        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);

    if (hasPreviousSample_ &&
        currentSystemTime > previousSystemTime_)
    {
        const std::uint64_t systemDelta =
            currentSystemTime - previousSystemTime_;

        for (auto& info : processes)
        {
            const auto current = currentCpuTimes.find(info.processId);
            const auto previous = previousCpuTimes_.find(info.processId);

            if (current == currentCpuTimes.end() ||
                previous == previousCpuTimes_.end() ||
                current->second < previous->second)
            {
                continue;
            }

            const std::uint64_t processDelta =
                current->second - previous->second;

            // Percentage of total system CPU capacity.
            const double percentage =
                100.0 * static_cast<double>(processDelta) /
                static_cast<double>(systemDelta);

            info.cpuUsagePercent =
                std::clamp(percentage, 0.0, 100.0);
        }
    }

    if (haveSystemTime)
    {
        previousSystemTime_ = currentSystemTime;
        previousCpuTimes_ = std::move(currentCpuTimes);
        hasPreviousSample_ = true;
    }

    std::sort(
        processes.begin(),
        processes.end(),
        [](const ProcessInfo& a, const ProcessInfo& b)
        {
            return a.memoryBytes > b.memoryBytes;
        });

    return processes;
}

EndProcessResult ProcessMonitor::endProcess(unsigned long processId)
{
    if (processId == 0 || processId == 4)
    {
        return {
            false,
            "This protected system process cannot be terminated."
        };
    }

    HANDLE process = OpenProcess(
        PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION,
        FALSE,
        processId);

    if (process == nullptr)
    {
        return {
            false,
            "Windows denied access, or the process has already exited."
        };
    }

    DWORD exitCode = 0;

    if (!GetExitCodeProcess(process, &exitCode) ||
        exitCode != STILL_ACTIVE)
    {
        CloseHandle(process);
        return {false, "The process has already exited."};
    }

    if (!TerminateProcess(process, 1))
    {
        const DWORD error = GetLastError();
        CloseHandle(process);

        return {
            false,
            "Windows could not terminate this process. Error code: " +
                std::to_string(error)
        };
    }

    CloseHandle(process);

    return {true, "The termination request was sent successfully."};
}
