/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_DAEMON_TIMEBASECONFIGURATION_H_
#define SCORE_TIME_DAEMON_TIMEBASECONFIGURATION_H_

#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "score/time/utility/TsyncConfigTypes.h"

namespace score {
namespace time {
namespace daemon {

struct TimeBaseConfigData {
    std::string timebase_name{};
    TsyncTimeDomainConfig timebase_config{};
};

class TimeBaseConfiguration final {
public:
    using iterator = std::map<std::string, TimeBaseConfigData>::iterator;
    using const_iterator = std::map<std::string, TimeBaseConfigData>::const_iterator;

    static TimeBaseConfiguration& GetInstance() noexcept;
    ~TimeBaseConfiguration() = default;
    TimeBaseConfiguration(const TimeBaseConfiguration&) = delete;
    TimeBaseConfiguration& operator=(const TimeBaseConfiguration&) = delete;
    TimeBaseConfiguration(TimeBaseConfiguration&&) = delete;
    TimeBaseConfiguration& operator=(TimeBaseConfiguration&&) = delete;

    void AddConfigData(const TimeBaseConfigData& config_data) noexcept;
    std::optional<TimeBaseConfigData> GetConfigData(
        const std::string_view& instance_specifier) const noexcept;
    std::optional<TimeBaseConfigData> GetConfigData(std::string_view name) const noexcept;
    void Clear();
    iterator begin();
    const_iterator cbegin() const;
    iterator end();
    const_iterator cend() const;
    std::size_t size() const;

private:
    TimeBaseConfiguration() noexcept = default;

private:
    std::map<std::string, TimeBaseConfigData> configurations_{};
};

}  // namespace daemon
}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_DAEMON_TIMEBASECONFIGURATION_H_
