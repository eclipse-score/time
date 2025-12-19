/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "score/time/daemon/ConfigLoader.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>

#include "score/result/result.h"

#include "flatcfg/flatcfg.h"
#include "score/time/common/Abort.h"
#include "score/time/daemon/TimeBaseConfiguration.h"
#include "score/time/daemon/tsync_flatcfg_addon.h"
#include "src/tsync-daemon/tsync_flatcfg_generated.h"

using score::time::common::logFatalAndAbort;

namespace score {
namespace time {
namespace daemon {

ConfigLoader::ConfigLoader() {
    flatcfg::FlatCfg cfg(flatbuffers::score::TSYNC{});
    auto cluster_list_result{cfg.getSwClusterList()};
    if (!cluster_list_result) {
        logFatalAndAbort((std::string{"tsync daemon - ConfigLoader: getSwClusterList() failed: "}
                          + cluster_list_result.error().Message().data()).c_str());
    }

    auto& cl{*cluster_list_result};
    if (cl.empty()) {
        logFatalAndAbort("tsync daemon - ConfigLoader: cluster list is empty.");
    }

    auto& cluster_to_load{cl.front()};

    auto load_result{cfg.load(cluster_to_load)};
    if (!load_result) {
        logFatalAndAbort((std::string{"tsync daemon - ConfigLoader: load(\""} + cluster_to_load
                          + "\") failed: " + load_result.error().Message().data()).c_str());
    }

    ParseDomainConfig(*load_result);
}

void ConfigLoader::ParseDomainConfig(std::shared_ptr<void> cfg) {
    auto ecu_cfg{GetEcuCfg(cfg)};
    auto domains{ecu_cfg->globalTimeDomain()};

    if (domains != nullptr) {
        // With the AUTOSAR extensions, there's 16 time domains (0-15) and
        // 16 offset time domains (16-31). That's the current understanding.
        constexpr uint32_t kTimeDomainIdMax{31u};
        auto& cfg_instance{TimeBaseConfiguration::GetInstance()};
        for (auto domain : *domains) {
            TimeBaseConfigData cfg_data;
            auto short_name{domain->shortName()};
            // short_name can't be nullptr since ShortNames must be non-empty
            if (short_name != nullptr) {
                cfg_data.timebase_name = short_name->c_str();
            }

            auto domain_id{domain->domainId()};

            if (domain_id > kTimeDomainIdMax) {
                std::string msg(
                    "tsync daemon - ConfigLoader::ParseDomainConfig: invalid domain id set for domain = ");
                msg += cfg_data.timebase_name;
                logFatalAndAbort(msg.c_str());
            }

            cfg_data.timebase_config.domain_id = static_cast<uint16_t>(domain_id);

            auto dur{std::chrono::duration<double>(domain->syncLossTimeout())};
            cfg_data.timebase_config.sync_loss_timeout = std::chrono::duration_cast<std::chrono::milliseconds>(dur);

            dur = std::chrono::duration<double>(domain->debounceTime());
            cfg_data.timebase_config.debounce_time = std::chrono::duration_cast<std::chrono::milliseconds>(dur);

            auto eth_domain_config{domain->ethGlobalTimeDomainProps()};
            if (eth_domain_config == nullptr || eth_domain_config->size() == 0U) {
                std::string msg(
                    "tsync daemon - ConfigLoader::ParseDomainConfig: ethernet global time properties not set for domain = ");
                msg += cfg_data.timebase_name;
                logFatalAndAbort(msg.c_str());
            }

            ParseEthernetDomainConfig(cfg_data.timebase_name, *eth_domain_config->Get(0U),
                                      cfg_data.timebase_config.eth_time_domain);

            auto provider_config{domain->synchronizedTimeBaseProviders()};
            if (provider_config && provider_config->size() != 0U) {
                ParseProviderConfig(cfg_data.timebase_name, *provider_config->Get(0U),
                                    cfg_data.timebase_config.provider_config);
            }

            auto consumer_config{domain->synchronizedTimeBaseConsumers()};
            if (consumer_config && consumer_config->size() != 0U) {
                ParseConsumerConfig(cfg_data.timebase_name, *consumer_config->Get(0U),
                                    cfg_data.timebase_config.consumer_config);
            }

            cfg_instance.AddConfigData(cfg_data);
        }
    } else {
        logFatalAndAbort("tsync daemon - ConfigLoader::ParseDomainConfig: No time domains found.");
    }
}

void ConfigLoader::ParseEthernetDomainConfig(const std::string& domain_name,
                                             const TSYNCFlatBuffer::EthGlobalTimeDomainProps& cfg,
                                             TsyncEthTimeDomain& out) {
    switch (cfg.messageCompliance()) {
        case TSYNCFlatBuffer::EthGlobalTimeMessageFormatEnum::EthGlobalTimeMessageFormatEnum_IEEE802_1AS:
            out.message_format = TsyncEthMessageFormat::kIeee8021AS;
            break;
        case TSYNCFlatBuffer::EthGlobalTimeMessageFormatEnum::EthGlobalTimeMessageFormatEnum_IEEE802_1AS_AUTOSAR:
            out.message_format = TsyncEthMessageFormat::kIeee8021ASAutosar;
            break;
        default: {
            std::string msg(
                "tsync daemon - ConfigLoader::ParseEthernetDomainConfig: unknown messageCompliance enum set for EthGlobalTimeDomainProps for time domain = ");
            msg += domain_name;
            logFatalAndAbort(msg.c_str());
        }
    }

    auto str{cfg.destinationPhysicalAddress()};

    if (str != nullptr) {
        std::size_t len{std::min<std::size_t>(str->size(), kMacAddressStringLength) + 1U};
        std::memcpy(&out.dest_mac_address, str->c_str(), len);
    }

    auto data_id_list{cfg.fupDataIdList()};

    if (data_id_list != nullptr) {
        if (data_id_list->size() > 0U) {
            if (data_id_list->size() == kMaxFupDataIdEntries) {
                std::memcpy(out.fup_data_id_list, data_id_list->data(), kMaxFupDataIdEntries);
                out.num_fup_data_id_entries = kMaxFupDataIdEntries;
            } else {
                std::string msg(
                    "tsync daemon - ConfigLoader::ParseEthernetDomainConfig: The number of fupDataIdList entries must be either 0 or 16 for time domain = ");
                msg += domain_name;

                logFatalAndAbort(msg.c_str());
            }
        } else {
            out.num_fup_data_id_entries = 0U;
        }
    } else {
        out.num_fup_data_id_entries = 0U;
    }

    out.vlan_priority = cfg.vlanPriority();

    auto crc_flags{cfg.crcFlags()};
    if (crc_flags != nullptr) {
        out.crcFlags.correction_field = (*crc_flags)[0u]->crcCorrectionField();
        out.crcFlags.domain_number = (*crc_flags)[0u]->crcDomainNumber();
        out.crcFlags.message_length = (*crc_flags)[0u]->crcMessageLength();
        out.crcFlags.precise_origin_timestamp = (*crc_flags)[0u]->crcPreciseOriginTimestamp();
        out.crcFlags.sequence_id = (*crc_flags)[0u]->crcSequenceId();
        out.crcFlags.source_port_identity = (*crc_flags)[0u]->crcSourcePortIdentity();
    }

    out.is_valid = true;
}

void ConfigLoader::ParseConsumerConfig(const std::string& domain_name,
                                       const TSYNCFlatBuffer::SynchronizedTimeBaseConsumer& cfg,
                                       TsyncConsumerConfig& out) {
    auto short_name{cfg.shortName()};
    // short_name can't be nullptr since ShortNames must be non-empty
    if (short_name != nullptr) {
        out.name = short_name->c_str();
    }

    auto nw_consumer{cfg.networkTimeConsumer()};
    if (!nw_consumer) {
        std::cerr << "ConfigLoader::ParseProviderConfig(): Network consumer not configured!" << std::endl;
        return;
    }

    // with new dsl, SynchronizedTimeBaseConsumer instance can only be generated/recored when it is referenced by slave.
    // Therefore it is not referenced, it doesn't exist => no need to check nullptr for nw_provider

    switch (nw_consumer->crcValidated()) {
        case TSYNCFlatBuffer::GlobalTimeCrcValidationEnum::GlobalTimeCrcValidationEnum_CrcValidated:
            out.time_slave_config.crc_validation = TsyncCrcValidation::kValidated;
            break;
        case TSYNCFlatBuffer::GlobalTimeCrcValidationEnum::GlobalTimeCrcValidationEnum_CrcNotValidated:
            out.time_slave_config.crc_validation = TsyncCrcValidation::kNotValidated;
            break;
        case TSYNCFlatBuffer::GlobalTimeCrcValidationEnum::GlobalTimeCrcValidationEnum_CrcIgnored:
            out.time_slave_config.crc_validation = TsyncCrcValidation::kIgnored;
            break;
        case TSYNCFlatBuffer::GlobalTimeCrcValidationEnum::GlobalTimeCrcValidationEnum_CrcOptional:
            out.time_slave_config.crc_validation = TsyncCrcValidation::kOptional;
            break;
        default: {
            std::string msg(
                "tsync daemon - ConfigLoader::ParseConsumerConfig: unknown crcValidated enum set for networkTimeConsumer for time domain = ");
            msg += domain_name;
            logFatalAndAbort(msg.c_str());
        }
    }

    out.time_slave_config.time_leap_healing_counter = nw_consumer->timeLeapHealingCounter();
    auto dur{std::chrono::duration<double>(nw_consumer->followUpTimeoutValue())};
    out.time_slave_config.follow_up_timeout_value = std::chrono::duration_cast<std::chrono::milliseconds>(dur);

    dur = std::chrono::duration<double>(nw_consumer->timeLeapFutureThreshold());
    out.time_slave_config.time_leap_future_threshold = std::chrono::duration_cast<std::chrono::milliseconds>(dur);
    dur = std::chrono::duration<double>(nw_consumer->timeLeapPastThreshold());
    out.time_slave_config.time_leap_past_threshold = std::chrono::duration_cast<std::chrono::milliseconds>(dur);

    out.time_slave_config.global_time_sequence_counter_jump_width = nw_consumer->globalTimeSequenceCounterJumpWidth();

    auto sub_tlv_cfg_vec = nw_consumer->subTlvConfig();
    if (sub_tlv_cfg_vec != nullptr) {
        auto sub_tlv_cfg = (*sub_tlv_cfg_vec)[0u];
        out.time_slave_config.sub_tlv_config.status_enabled = sub_tlv_cfg->statusSubTlv();
        out.time_slave_config.sub_tlv_config.time_enabled = sub_tlv_cfg->timeSubTlv();
        out.time_slave_config.sub_tlv_config.user_data_enabled = sub_tlv_cfg->userDataSubTlv();
    }

    out.time_slave_config.is_valid = true;
}

void ConfigLoader::ParseProviderConfig(const std::string& domain_name,
                                       const TSYNCFlatBuffer::SynchronizedTimeBaseProvider& cfg,
                                       TsyncProviderConfig& out) {
    auto short_name{cfg.shortName()};
    // short_name can't be nullptr since ShortNames must be non-empty
    if (short_name != nullptr) {
        out.name = short_name->c_str();
    }

    auto nw_provider{cfg.networkTimeProvider()};
    if (!nw_provider) {
        std::cerr << "ConfigLoader::ParseProviderConfig(): Network provider not configured for time domain = "
                  << domain_name << std::endl;
        return;
    }

    // with new dsl, SynchronizedTimeBaseProvider instance can only be generated/recored when it is referenced by
    // master. Therefore it is not referenced, it doesn't exist => no need to check nullptr for nw_provider

    auto dur{std::chrono::duration<double>(nw_provider->immediateResumeTime())};
    out.time_master_config.immediate_resume_time = std::chrono::duration_cast<std::chrono::milliseconds>(dur);
    dur = std::chrono::duration<double>(nw_provider->syncPeriod());
    out.time_master_config.sync_period = std::chrono::duration_cast<std::chrono::milliseconds>(dur);
    out.time_master_config.is_system_wide_global_time_master = nw_provider->isSystemWideGlobalTimeMaster();
    out.time_master_config.is_valid = true;

    switch (nw_provider->crcSecured()) {
        case TSYNCFlatBuffer::GlobalTimeCrcSupportEnum::GlobalTimeCrcSupportEnum_CrcSupported:
            out.time_master_config.crc_support = TsyncCrcSupport::kSupported;
            break;
        case TSYNCFlatBuffer::GlobalTimeCrcSupportEnum::GlobalTimeCrcSupportEnum_CrcNotSupported:
            out.time_master_config.crc_support = TsyncCrcSupport::kNotSupported;
            break;
        default: {
            std::string msg(
                "tsync daemon - ConfigLoader::ParseProviderConfig: crcSecured not set for networkTimeProvider for time domain = ");
            msg += domain_name;
            logFatalAndAbort(msg.c_str());
        }
    }

    auto ts_correction_vec{cfg.timeSyncCorrection()};
    if (ts_correction_vec != nullptr && ts_correction_vec->size() == 1U) {
        auto ts_correction{ts_correction_vec->Get(0u)};
        out.time_sync_correction_config.allow_provider_rate_correction = ts_correction->allowProviderRateCorrection();
        out.time_sync_correction_config.num_rate_corrections_per_measurement_duration =
            ts_correction->rateCorrectionsPerMeasurementDuration();
        dur = std::chrono::duration<double>(ts_correction->offsetCorrectionAdaptionInterval());
        out.time_sync_correction_config.offset_correction_adaption_interval =
            std::chrono::duration_cast<std::chrono::milliseconds>(dur);
        dur = std::chrono::duration<double>(ts_correction->offsetCorrectionJumpThreshold());
        out.time_sync_correction_config.offset_correction_jump_threshold =
            std::chrono::duration_cast<std::chrono::milliseconds>(dur);
        dur = std::chrono::duration<double>(ts_correction->rateDeviationMeasurementDuration());
        out.time_sync_correction_config.rate_deviation_measurement_duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(dur);

        auto sub_tlv_cfg_vec = nw_provider->subTlvConfig();
        if (sub_tlv_cfg_vec != nullptr) {
            auto sub_tlv_cfg = (*sub_tlv_cfg_vec)[0u];
            out.time_master_config.sub_tlv_config.status_enabled = sub_tlv_cfg->statusSubTlv();
            out.time_master_config.sub_tlv_config.time_enabled = sub_tlv_cfg->timeSubTlv();
            out.time_master_config.sub_tlv_config.user_data_enabled = sub_tlv_cfg->userDataSubTlv();
        }

        out.time_sync_correction_config.is_valid = true;
    } else {
        out.time_sync_correction_config.is_valid = false;
    }
}

// coverity[autosar_cpp14_a8_4_11_violation] shared_ptr's lifetime is not supposed to be affected here.
const TSYNCFlatBuffer::TSYNCEcuCfg* ConfigLoader::GetEcuCfg(std::shared_ptr<void> cfg) {
    auto ecu_cfg{TSYNCFlatBuffer::GetTSYNCEcuCfg(cfg.get())};
    if (ecu_cfg == nullptr) {
        logFatalAndAbort("tsync daemon - ConfigLoader::GetModuleInstantiation: GetTSYNCEcuCfg returned nullptr.");
    } else {
    }

    return ecu_cfg;
}

void ConfigLoader::DumpConfig() noexcept {
    auto& cfg_instance{TimeBaseConfiguration::GetInstance()};

    for (auto& it : cfg_instance) {
        std::cout << "Timebase name: " << it.second.timebase_name << std::endl;
        std::cout << "Timebase id: " << static_cast<std::int32_t>(it.second.timebase_config.domain_id) << std::endl;
        std::cout << "Syncloss timeout: " << it.second.timebase_config.sync_loss_timeout.count() << "ms" << std::endl;
        std::cout << "--------------------------------- Consumer Config ----------------------------------"
                  << std::endl;
        if (it.second.timebase_config.consumer_config.time_slave_config.is_valid) {
            std::cout << "Consumer Name: " << it.second.timebase_config.consumer_config.name << std::endl;
            std::cout << "time_leap_healing_counter: "
                      << it.second.timebase_config.consumer_config.time_slave_config.time_leap_healing_counter
                      << std::endl;
            std::cout << "time_leap_future_threshold: "
                      << it.second.timebase_config.consumer_config.time_slave_config.time_leap_future_threshold.count()
                      << "ms" << std::endl;
            std::cout << "time_leap_past_threshold: "
                      << it.second.timebase_config.consumer_config.time_slave_config.time_leap_past_threshold.count()
                      << "ms" << std::endl;
            std::cout << "follow_up_timeout_value: "
                      << it.second.timebase_config.consumer_config.time_slave_config.follow_up_timeout_value.count()
                      << "ms" << std::endl;
            std::cout << "subtlv status: "
                      << (it.second.timebase_config.consumer_config.time_slave_config.sub_tlv_config.status_enabled
                              ? "true"
                              : "false")
                      << std::endl;
            std::cout << "subtlv time: "
                      << (it.second.timebase_config.consumer_config.time_slave_config.sub_tlv_config.time_enabled
                              ? "true"
                              : "false")
                      << std::endl;
            std::cout << "subtlv user data: "
                      << (it.second.timebase_config.consumer_config.time_slave_config.sub_tlv_config.user_data_enabled
                              ? "true"
                              : "false")
                      << std::endl;
            std::cout
                << "global_time_sequence_counter_jump_width: "
                << it.second.timebase_config.consumer_config.time_slave_config.global_time_sequence_counter_jump_width
                << std::endl;
        } else {
            std::cout << "- None -" << std::endl;
        }
        std::cout << "--------------------------------- /Consumer Config ---------------------------------"
                  << std::endl;
        std::cout << "--------------------------------- Provider Config ----------------------------------"
                  << std::endl;
        if (it.second.timebase_config.provider_config.time_master_config.is_valid) {
            std::cout << "Provider Name: " << it.second.timebase_config.provider_config.name << std::endl;
            std::cout << "is_system_wide_global_time_master: "
                      << (it.second.timebase_config.provider_config.time_master_config.is_system_wide_global_time_master
                              ? "true"
                              : "false")
                      << std::endl;
            std::cout << "immediate_resume_time: "
                      << it.second.timebase_config.provider_config.time_master_config.immediate_resume_time.count()
                      << "ms" << std::endl;
            std::cout << "sync_period: "
                      << it.second.timebase_config.provider_config.time_master_config.sync_period.count() << "ms"
                      << std::endl;
            std::cout
                << "allow_provider_rate_correction: "
                << (it.second.timebase_config.provider_config.time_sync_correction_config.allow_provider_rate_correction
                        ? "true"
                        : "false")
                << std::endl;
            std::cout << "rate_deviation_measurement_duration: "
                      << it.second.timebase_config.provider_config.time_sync_correction_config
                             .rate_deviation_measurement_duration.count()
                      << "ms" << std::endl;
            std::cout << "num_rate_corrections_per_measurement_duration: "
                      << it.second.timebase_config.provider_config.time_sync_correction_config
                             .num_rate_corrections_per_measurement_duration
                      << std::endl;
            std::cout << "offset_correction_adaption_interval: "
                      << it.second.timebase_config.provider_config.time_sync_correction_config
                             .offset_correction_adaption_interval.count()
                      << "ms" << std::endl;
            std::cout << "offset_correction_jump_threshold: "
                      << it.second.timebase_config.provider_config.time_sync_correction_config
                             .offset_correction_jump_threshold.count()
                      << "ms" << std::endl;
            std::cout << "subtlv status: "
                      << (it.second.timebase_config.provider_config.time_master_config.sub_tlv_config.status_enabled
                              ? "true"
                              : "false")
                      << std::endl;
            std::cout << "subtlv time: "
                      << (it.second.timebase_config.provider_config.time_master_config.sub_tlv_config.time_enabled
                              ? "true"
                              : "false")
                      << std::endl;
            std::cout << "subtlv user data: "
                      << (it.second.timebase_config.provider_config.time_master_config.sub_tlv_config.user_data_enabled
                              ? "true"
                              : "false")
                      << std::endl;
        } else {
            std::cout << "- None -" << std::endl;
        }
        std::cout << "--------------------------------- /Provider Config ---------------------------------"
                  << std::endl;
    }
}

const TimeBaseConfiguration& ConfigLoader::GetConfig() const noexcept {
    return TimeBaseConfiguration::GetInstance();
}

}  // namespace daemon
}  // namespace time
}  // namespace score
