#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct ProcessInfo
{
unsigned long processId = 0;
std::string name;
std::uint64_t memoryBytes = 0;
};

class ProcessMonitor
{
public:
std::vector<ProcessInfo> getProcesses() const;
};