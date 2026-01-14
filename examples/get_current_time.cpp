/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <iostream>

// For S-Core Time
#include "score/time/synchronized_time_base_consumer.h"
#include "score/time/synchronized_time_base_status.h"

// For OS Time and main loop
#include <chrono> // For std::chrono
#include <thread> // For std::this_thread::sleep_for

// Handle Ctrl-C to abort the loop
#include <csignal>
static volatile std::sig_atomic_t keep_running = true;
void signal_handler(int signum) {
    keep_running = false;
}

using nanoseconds = std::chrono::nanoseconds;

int main() {
    std::signal(SIGINT, signal_handler);
    std::string_view instance_specifier = "consumer";

    std::cout << "Instantiate a SynchronizedTimeBaseConsumer for instance '"
              << instance_specifier << "'" << std::endl;
    score::time::SynchronizedTimeBaseConsumer stbc{instance_specifier};

    auto os_first = std::chrono::high_resolution_clock::now();
    auto os_first_ns = std::chrono::duration_cast<nanoseconds>(os_first.time_since_epoch()).count();
    auto stbc_first = stbc.GetCurrentTime();
    auto stbc_first_ns = stbc_first.time_since_epoch().count();

    while (keep_running) {
        // Get current timestamp from the operating system
        auto os_timestamp = std::chrono::high_resolution_clock::now();
        auto stbc_timestamp = stbc.GetCurrentTime();

        auto stbc_timestamp_with_status = stbc.GetTimeWithStatus();
        auto stbc_sync_status = stbc_timestamp_with_status.GetSynchronizationStatus();

        // auto stbc_timestamp_duration = stbc.GetCurrentTime().time_since_epoch().count();
        auto os_ns = std::chrono::duration_cast<nanoseconds>(os_timestamp.time_since_epoch()).count();
        auto stbc_ns = stbc_timestamp.time_since_epoch().count();

        // Calculate the delta
        auto elapsed_ns = os_ns - os_first_ns;
        auto stbc_elapsed_ns = stbc_ns - stbc_first_ns;

        auto status = "Unavailable";
        if (stbc_sync_status == score::time::SynchronizationStatus::kNotSynchronizedUntilStartup) {
            status = "Not synchronized until startup";
        } else if (stbc_sync_status == score::time::SynchronizationStatus::kTimeOut) {
            status = "Timeout";
        } else if (stbc_sync_status == score::time::SynchronizationStatus::kSynchronized) {
            status = "Synchronized";
        } else if (stbc_sync_status == score::time::SynchronizationStatus::kSynchToGateway) {
            status = "Synchronized to Gateway";
        } else {
            status = "Unknown";
        }

        std::cout
            << "OS="
            << os_ns
            << " PTP="
            << stbc_ns
            << " OS Elapsed="
            << elapsed_ns
            << " PTP Elapsed="
            << stbc_elapsed_ns
            << " Status="
            << status
            << std::endl;

        // Wait for 1 second
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
