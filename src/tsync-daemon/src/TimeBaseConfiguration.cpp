/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "TimeBaseConfiguration.h"

namespace score {
namespace time {
namespace daemon {

TimeBaseConfiguration& TimeBaseConfiguration::GetInstance() noexcept {
    // clang-format off
    // RULECHECKER_comment(1, 1, check_static_object_dynamic_initialization, "The scope of this object is limited to this function. It is therefore protected during global static initializiation.", true)
    static TimeBaseConfiguration instance{};
    // clang-format on
    return instance;
}

void TimeBaseConfiguration::AddConfigData(const TimeBaseConfigData& config_data) noexcept {
    configurations_[config_data.timebase_name] = config_data;
}

std::optional<TimeBaseConfigData> TimeBaseConfiguration::GetConfigData(const std::string_view& name) const noexcept {
    auto it = configurations_.find(std::string(name));
    if (it != configurations_.end()) {
        return std::optional<TimeBaseConfigData>(it->second);
    }

    return std::optional<TimeBaseConfigData>();
}

void TimeBaseConfiguration::Clear() {
    configurations_.clear();
}

TimeBaseConfiguration::iterator TimeBaseConfiguration::begin() {
    return configurations_.begin();
}

TimeBaseConfiguration::const_iterator TimeBaseConfiguration::cbegin() const {
    return configurations_.cbegin();
}

TimeBaseConfiguration::iterator TimeBaseConfiguration::end() {
    return configurations_.end();
}

TimeBaseConfiguration::const_iterator TimeBaseConfiguration::cend() const {
    return configurations_.cend();
}

std::size_t TimeBaseConfiguration::size() const {
    return configurations_.size();
}

}  // namespace daemon
}  // namespace time
}  // namespace score
