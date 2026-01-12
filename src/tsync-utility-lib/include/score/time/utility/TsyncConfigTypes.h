/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_UTILITY_TSYNCCONFIGTYPES_H_
#define SCORE_TIME_UTILITY_TSYNCCONFIGTYPES_H_

#include <chrono>
#include <cstdint>
#include <string>

namespace score {
namespace time {

// Note:
// Ecu-Config uses the unit seconds for most time values given as a
// floating point value. To accomodate this, milliseconds are used below
// to be able to work with fractions of a second. The actual represenation of
// these values is a int64_t.

// coverity[autosar_cpp14_a0_1_1_violation:FALSE] #49860: This value is used subsequently.
constexpr std::size_t kMacAddressStringLength = 12U;
// coverity[autosar_cpp14_a0_1_1_violation:FALSE] #49860: This value is used subsequently.
constexpr std::uint32_t kMaxFupDataIdEntries = 16U;

enum class TsyncCrcSupport : uint32_t { kSupported = 0U, kNotSupported = 1U };

enum class TsyncCrcValidation : uint32_t { kValidated = 0U, kNotValidated = 1U, kIgnored = 2U, kOptional = 3U };

enum class TsyncEthMessageFormat : uint32_t { kIeee8021AS = 0U, kIeee8021ASAutosar = 1U };

struct TsyncCrcFlags {
    bool correction_field = false;
    bool domain_number = false;
    bool message_length = false;
    bool precise_origin_timestamp = false;
    bool sequence_id = false;
    bool source_port_identity = false;
};

struct TsyncEthTimeDomain {
    uint8_t fup_data_id_list[kMaxFupDataIdEntries]{};
    char dest_mac_address[kMacAddressStringLength + 1U]{};
    TsyncEthMessageFormat message_format = TsyncEthMessageFormat::kIeee8021ASAutosar;
    uint32_t vlan_priority = 0U;
    uint32_t num_fup_data_id_entries = 0U;
    TsyncCrcFlags crcFlags{};
    bool is_valid = false;  // set to true when the config values have been initialized
};

struct TsyncSubTlvConfig {
    bool time_enabled = false;
    bool status_enabled = false;
    bool user_data_enabled = false;
};

struct TsyncTimeMaster {
    std::chrono::milliseconds immediate_resume_time = std::chrono::milliseconds::zero();
    std::chrono::milliseconds sync_period = std::chrono::milliseconds::zero();
    TsyncCrcSupport crc_support = TsyncCrcSupport::kNotSupported;
    bool is_system_wide_global_time_master = false;
    TsyncSubTlvConfig sub_tlv_config{};
    bool is_valid = false;  // set to true when the config values have been initialized
};

struct TsyncTimeSlave {
    std::chrono::milliseconds follow_up_timeout_value = std::chrono::milliseconds::zero();
    std::chrono::milliseconds time_leap_future_threshold = std::chrono::milliseconds::zero();
    std::chrono::milliseconds time_leap_past_threshold = std::chrono::milliseconds::zero();
    std::uint32_t time_leap_healing_counter = 0;
    TsyncCrcValidation crc_validation = TsyncCrcValidation::kIgnored;
    std::uint32_t global_time_sequence_counter_jump_width = 0;
    TsyncSubTlvConfig sub_tlv_config{};
    bool is_valid = false;  // set to true when the config values have been initialized
};

struct TsyncConsumerConfig {
    std::string name{};
    TsyncTimeSlave time_slave_config{};
};

struct TsyncTimeSyncCorrection {
    std::chrono::milliseconds offset_correction_adaption_interval = std::chrono::milliseconds::zero();
    std::chrono::milliseconds offset_correction_jump_threshold = std::chrono::milliseconds::zero();
    std::chrono::milliseconds rate_deviation_measurement_duration = std::chrono::milliseconds::zero();
    std::uint32_t num_rate_corrections_per_measurement_duration = 0;
    bool allow_provider_rate_correction = false;
    bool is_valid = false;  // set to true when the config values have been initialized
};

struct TsyncProviderConfig {
    std::string name{};
    TsyncTimeMaster time_master_config{};
    TsyncTimeSyncCorrection time_sync_correction_config{};
};

struct TsyncTimeDomainConfig {
    TsyncEthTimeDomain eth_time_domain{};
    TsyncConsumerConfig consumer_config{};
    TsyncProviderConfig provider_config{};
    std::chrono::milliseconds sync_loss_timeout = std::chrono::milliseconds::zero();
    std::chrono::milliseconds debounce_time = std::chrono::milliseconds::zero();
    std::uint16_t domain_id = 0U;
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_UTILITY_TSYNCCONFIGTYPES_H_
