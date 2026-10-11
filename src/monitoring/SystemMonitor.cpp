#include "SystemMonitor.h"

#include <windows.h>

#include <pdh.h>
#include <pdhmsg.h>

SystemMonitor::~SystemMonitor()
{
    if (query_ != nullptr)
    {
        PdhCloseQuery(query_);
    }
}

bool SystemMonitor::initialize(std::string& error)
{
    PDH_STATUS status = PdhOpenQueryW(
        nullptr, 0, &query_);

    if (status != ERROR_SUCCESS)
    {
        error = "Failed to open performance query. Code: "
              + std::to_string(status);
        return false;
    }

    status = PdhAddEnglishCounterW(
        query_,
        L"\\Processor(_Total)\\% Processor Time",
        0,
        &cpuCounter_);

    if (status != ERROR_SUCCESS)
    {
        error = "Failed to create CPU counter. Code: "
              + std::to_string(status);
        return false;
    }

    // The first collection establishes a baseline.
    status = PdhCollectQueryData(query_);

    if (status != ERROR_SUCCESS)
    {
        error = "Failed to collect initial CPU data. Code: "
              + std::to_string(status);
        return false;
    }

    return true;
}

bool SystemMonitor::sample(
    SystemMetrics& metrics,
    std::string& error)
{
    PDH_STATUS status = PdhCollectQueryData(query_);

    if (status != ERROR_SUCCESS)
    {
        error = "Failed to collect CPU data. Code: "
              + std::to_string(status);
        return false;
    }

    PDH_FMT_COUNTERVALUE cpuValue{};

    status = PdhGetFormattedCounterValue(
        cpuCounter_,
        PDH_FMT_DOUBLE,
        nullptr,
        &cpuValue);

    if (status != ERROR_SUCCESS ||
        cpuValue.CStatus != PDH_CSTATUS_VALID_DATA)
    {
        error = "CPU measurement is not ready yet.";
        return false;
    }

    MEMORYSTATUSEX memoryStatus{};
    memoryStatus.dwLength = sizeof(memoryStatus);

    if (!GlobalMemoryStatusEx(&memoryStatus))
    {
        error = "Failed to read physical memory statistics.";
        return false;
    }

    metrics.cpuUsagePercent = cpuValue.doubleValue;
    metrics.memoryTotalBytes = memoryStatus.ullTotalPhys;
    metrics.memoryUsedBytes =
        memoryStatus.ullTotalPhys - memoryStatus.ullAvailPhys;

    return true;
}