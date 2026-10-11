
#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct ProcessInfo
{
    unsigned long processId = 0;
    std::string name;
    std::uint64_t memoryBytes = 0;
    double cpuUsagePercent = 0.0;
};

struct EndProcessResult
{
    bool success = false;
    std::string message;
};

class ProcessMonitor
{
public:
    std::vector<ProcessInfo> getProcesses();
    EndProcessResult endProcess(unsigned long processId);

private:
    std::unordered_map<unsigned long, std::uint64_t> previousCpuTimes_;
    std::uint64_t previousSystemTime_ = 0;
    bool hasPreviousSample_ = false;
};
