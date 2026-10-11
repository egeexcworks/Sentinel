#include "ProcessMonitor.h"

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#include <algorithm>
#include <string>
#include <vector>

namespace
{
std::string toUtf8(const wchar_t* text)
{
if (text == nullptr || *text == L'\0')
{
return {};
}

    const int required = WideCharToMultiByte(
        CP_UTF8, 0, text, -1,
        nullptr, 0, nullptr, nullptr);

    if (required <= 1)
    {
        return {};
    }

    std::string result(
        static_cast<std::size_t>(required - 1), '\0');

    WideCharToMultiByte(
        CP_UTF8, 0, text, -1,
        result.data(), required,
        nullptr, nullptr);

    return result;
}

}

std::vector<ProcessInfo> ProcessMonitor::getProcesses() const
{
std::vector<ProcessInfo> processes;

HANDLE snapshot = CreateToolhelp32Snapshot(
    TH32CS_SNAPPROCESS, 0);

if (snapshot == INVALID_HANDLE_VALUE)
{
    return processes;
}

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
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
            FALSE,
            entry.th32ProcessID);

        if (process != nullptr)
        {
            PROCESS_MEMORY_COUNTERS memory{};
            memory.cb = sizeof(memory);

            if (GetProcessMemoryInfo(
                    process, &memory, sizeof(memory)))
            {
                info.memoryBytes =
                    static_cast<std::uint64_t>(
                        memory.WorkingSetSize);
            }

            CloseHandle(process);
        }

        processes.push_back(std::move(info));

    } while (Process32NextW(snapshot, &entry));
}

CloseHandle(snapshot);

std::sort(
    processes.begin(),
    processes.end(),
    [](const ProcessInfo& a, const ProcessInfo& b)
    {
        return a.memoryBytes > b.memoryBytes;
    });

return processes;

}