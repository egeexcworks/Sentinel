#include "monitoring/SystemMonitor.h"
#include "processes/ProcessMonitor.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

int main()
{
SystemMonitor monitor;
std::string error;

if (!monitor.initialize(error))
{
    std::cerr << "Sentinel initialization failed: "
              << error << '\n';
    return 1;
}

ProcessMonitor processMonitor;

std::cout << "Sentinel System Monitor\n";
std::cout << "Press Ctrl+C to exit.\n";

while (true)
{
    std::this_thread::sleep_for(
        std::chrono::seconds(2));

    SystemMetrics metrics;

    if (!monitor.sample(metrics, error))
    {
        std::cerr << "Monitoring error: "
                  << error << '\n';
        continue;
    }

    std::cout << "\nCPU: "
              << std::fixed << std::setprecision(1)
              << metrics.cpuUsagePercent << "%\n";

    std::cout << "RAM: "
              << metrics.memoryUsedBytes / (1024.0 * 1024.0)
              << " / "
              << metrics.memoryTotalBytes / (1024.0 * 1024.0)
              << " MiB\n";

    const auto processes = processMonitor.getProcesses();

    std::cout << "\nTop processes by memory\n";
    std::cout << std::left
              << std::setw(32) << "Process"
              << std::setw(12) << "PID"
              << "Memory (MiB)\n";

    std::cout << std::string(60, '-') << '\n';

    const std::size_t count =
        processes.size() < 10 ? processes.size() : 10;

    for (std::size_t i = 0; i < count; ++i)
    {
        const auto& process = processes[i];

        std::cout << std::left
                  << std::setw(32) << process.name
                  << std::setw(12) << process.processId
                  << std::fixed << std::setprecision(1)
                  << process.memoryBytes / (1024.0 * 1024.0)
                  << '\n';
    }

    std::cout << "\n";
}

}