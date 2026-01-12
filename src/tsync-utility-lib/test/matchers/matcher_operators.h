/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_MATCHER_OPERATORS_H_
#define SCORE_TIME_MATCHER_OPERATORS_H_

#include "score/span.hpp"

#include "score/time/utility/TsyncConfigTypes.h"
#include "score/time/utility/ITimeBaseAccessor.h"

namespace score {
namespace core {

inline bool operator==(const score::cpp::span<const std::byte>& v1, const score::cpp::span<const std::byte>& v2) {
    return (std::equal(std::begin(v1), std::end(v1), std::begin(v2), std::end(v2)));
}

inline bool operator!=(const score::cpp::span<const std::byte>& v1, const score::cpp::span<const std::byte>& v2) {
    return (!(v1 == v2));
}

}  // namespace core

namespace time {
inline bool operator==(const TsyncConsumerConfig& lhs, const TsyncConsumerConfig& rhs) {
    return ((lhs.name == rhs.name) &&
            (lhs.time_slave_config.is_valid == rhs.time_slave_config.is_valid) && (lhs.time_slave_config.follow_up_timeout_value == rhs.time_slave_config.follow_up_timeout_value) &&
            (lhs.time_slave_config.time_leap_future_threshold == rhs.time_slave_config.time_leap_future_threshold) &&
            (lhs.time_slave_config.time_leap_healing_counter == rhs.time_slave_config.time_leap_healing_counter) &&
            (lhs.time_slave_config.time_leap_past_threshold == rhs.time_slave_config.time_leap_past_threshold));
}

inline bool operator!=(const TsyncConsumerConfig& lhs, const TsyncConsumerConfig& rhs) {
    return !(lhs == rhs);
}

inline bool operator==(const TsyncProviderConfig& lhs, const TsyncProviderConfig& rhs) {
    return ((lhs.name == rhs.name) &&
            (lhs.time_master_config.is_valid == rhs.time_master_config.is_valid) &&
            (lhs.time_master_config.immediate_resume_time == rhs.time_master_config.immediate_resume_time) &&
            (lhs.time_master_config.is_system_wide_global_time_master == rhs.time_master_config.is_system_wide_global_time_master) &&
            (lhs.time_master_config.sync_period == rhs.time_master_config.sync_period) &&
            (lhs.time_sync_correction_config.is_valid == rhs.time_sync_correction_config.is_valid) &&
            (lhs.time_sync_correction_config.allow_provider_rate_correction == rhs.time_sync_correction_config.allow_provider_rate_correction) &&
            (lhs.time_sync_correction_config.num_rate_corrections_per_measurement_duration == rhs.time_sync_correction_config.num_rate_corrections_per_measurement_duration) &&
            (lhs.time_sync_correction_config.offset_correction_adaption_interval == rhs.time_sync_correction_config.offset_correction_adaption_interval) &&
            (lhs.time_sync_correction_config.offset_correction_jump_threshold == rhs.time_sync_correction_config.offset_correction_jump_threshold) &&
            (lhs.time_sync_correction_config.rate_deviation_measurement_duration == rhs.time_sync_correction_config.rate_deviation_measurement_duration));
}

inline bool operator!=(const TsyncProviderConfig& lhs, const TsyncProviderConfig& rhs) {
    return !(lhs == rhs);
}

inline bool operator==(const TsyncEthTimeDomain& lhs, const TsyncEthTimeDomain& rhs) {
    return ( lhs.is_valid == rhs.is_valid &&
             lhs.message_format == rhs.message_format &&
             lhs.num_fup_data_id_entries == rhs.num_fup_data_id_entries &&
             lhs.vlan_priority == rhs.vlan_priority &&
             memcmp(&lhs.dest_mac_address, &rhs.dest_mac_address, sizeof(lhs.dest_mac_address)) == 0 &&
             memcmp(&lhs.fup_data_id_list, &rhs.fup_data_id_list, sizeof(lhs.fup_data_id_list)) == 0);
}

inline bool operator!=(const TsyncEthTimeDomain& lhs, const TsyncEthTimeDomain& rhs) {
    return !(lhs == rhs);
}

inline bool operator==(const TsyncTimeDomainConfig& lhs, const TsyncTimeDomainConfig& rhs) {
    return ((lhs.domain_id == rhs.domain_id) &&
            (lhs.consumer_config == rhs.consumer_config) &&
            (lhs.provider_config == rhs.provider_config) &&
            lhs.eth_time_domain == rhs.eth_time_domain &&
            lhs.debounce_time == rhs.debounce_time &&
            lhs.sync_loss_timeout == rhs.sync_loss_timeout);
}


inline bool operator!=(const TsyncTimeDomainConfig& lhs, const TsyncTimeDomainConfig& rhs) {
    return !(lhs == rhs);
}

constexpr bool operator==(const TimestampWithStatus& v1, const TimestampWithStatus& v2) {
    return ((v1.status == v2.status) && (v1.nanoseconds == v2.nanoseconds) && (v1.seconds == v2.seconds));
}

constexpr bool operator!=(const TimestampWithStatus& v1, const TimestampWithStatus& v2) {
    return (!(v1 == v2));
}

constexpr bool operator==(const VirtualLocalTime& v1, const VirtualLocalTime& v2) {
    return (v1 == v2);
}

constexpr bool operator!=(const VirtualLocalTime& v1, const VirtualLocalTime& v2) {
    return (!(v1 == v2));
}

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_MATCHER_OPERATORS_H_
