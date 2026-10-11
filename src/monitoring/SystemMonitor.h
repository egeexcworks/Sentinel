#pragma once

#include <cstdint>
#include <string>
#include <pdh.h>

struct SystemMetrics
{
    double cpuUsagePercent = 0.0;
    std::uint64_t memoryUsedBytes = 0;
    std::uint64_t memoryTotalBytes = 0;
};

class SystemMonitor
{
public:
    SystemMonitor() = default;
    ~SystemMonitor();

    SystemMonitor(const SystemMonitor&) = delete;
    SystemMonitor& operator=(const SystemMonitor&) = delete;

    bool initialize(std::string& error);
    bool sample(SystemMetrics& metrics, std::string& error);

private:
    PDH_HQUERY query_ = nullptr;
    PDH_HCOUNTER cpuCounter_ = nullptr;
};