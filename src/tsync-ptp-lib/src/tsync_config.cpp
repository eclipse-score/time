/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <cstring>
#include <mutex>

#include "score/time/common/Abort.h"

#include "score/time/utility/ITimeBaseReader.h"
#include "score/time/utility/TimeBaseReaderFactory.h"
#include "score/time/utility/TsyncConfigTypes.h"

#include "tsync_data_broker.h"
#include "tsync_ptp_lib.h"

using score::time::ITimeBaseReader;
using score::time::TimeBaseReaderFactory;
using score::time::TsyncTimeDomainConfig;
using score::time::TsyncCrcSupport;
using score::time::TsyncCrcValidation;
using score::time::TsyncEthMessageFormat;
using score::time::common::logFatalAndAbort;


TSync_CrcSupport Convert(TsyncCrcSupport crc_support) {
    switch (crc_support) {
        case TsyncCrcSupport::kSupported:
            return TSYNC_CRC_SUPPORTED;
        case TsyncCrcSupport::kNotSupported:
            return TSYNC_CRC_NOT_SUPPORTED;
        default:
            return TSYNC_CRC_NOT_SUPPORTED;
    }
}

TSync_RxCrcValidation Convert(TsyncCrcValidation crc_validation) {
    switch(crc_validation) {
        case TsyncCrcValidation::kIgnored:
            return TSYNC_CRC_IGNORED;
        case TsyncCrcValidation::kNotValidated:
            return TSYNC_CRC_NOT_VALIDATED;
        case TsyncCrcValidation::kOptional:
            return TSYNC_CRC_OPTIONAL;
        case TsyncCrcValidation::kValidated:
            return TSYNC_CRC_VALIDATED;
        default:
            return TSYNC_CRC_OPTIONAL;
    }
}

TSync_MessageCompliance Convert(TsyncEthMessageFormat msg_format) {
    switch(msg_format) {
        case TsyncEthMessageFormat::kIeee8021AS:
            return TSYNC_MC_IEEE8021AS;
        case TsyncEthMessageFormat::kIeee8021ASAutosar:
            return TSYNC_MC_IEEE8021AS_AUTOSAR;
        default:
            return TSYNC_MC_IEEE8021AS_AUTOSAR;
    }
}

TSync_ReturnType TSync_GetTimebaseConfiguration(TSync_TimeBaseHandleType timebaseHandle, TSync_Role role_requested, TSync_TimeBaseConfiguration* config) {
    if(timebaseHandle != TSYNC_INVALID_HANDLE && config != nullptr) {
        TSync_DB_Timebase_Meta_Data* timebase_meta_data = static_cast<TSync_DB_Timebase_Meta_Data*>(timebaseHandle);
        TsyncTimeDomainConfig domain_config;
        {
            timebase_meta_data->reader->GetAccessor().Open();
            std::lock_guard<ITimeBaseReader> lock(*(timebase_meta_data->reader));
            if(!timebase_meta_data->reader->Read(domain_config)) {
                logFatalAndAbort("TSync_GetTimebaseConfiguration - Could not read time domain configuration.");
            }
        }

        if(!(domain_config.consumer_config.time_slave_config.is_valid || domain_config.provider_config.time_master_config.is_valid)) {
                logFatalAndAbort("TSync_GetTimebaseConfiguration - Neither the time-consumer config nor the time-provider config is valid.");
        }

        config->numFupDataIdEntries = domain_config.eth_time_domain.num_fup_data_id_entries;
        if(config->numFupDataIdEntries != 0u) {
            std::memcpy(&config->fupDataIdList[0],& domain_config.eth_time_domain.fup_data_id_list[0], sizeof(domain_config.eth_time_domain.fup_data_id_list));
        }
        std::memcpy(&config->destMacAddress[0], &domain_config.eth_time_domain.dest_mac_address[0], sizeof(domain_config.eth_time_domain.dest_mac_address));
        config->vLanPriority = domain_config.eth_time_domain.vlan_priority;
        config->crcSupport = Convert(domain_config.provider_config.time_master_config.crc_support);
        config->crcValidation = Convert(domain_config.consumer_config.time_slave_config.crc_validation);
        config->messageCompliance = Convert(domain_config.eth_time_domain.message_format);
        config->role = domain_config.provider_config.time_master_config.is_valid ? TSYNC_ROLE_MASTER : TSYNC_ROLE_SLAVE;
        config->followUpTimeoutMs = domain_config.consumer_config.time_slave_config.follow_up_timeout_value.count();
        config->immediateResumeTimeMs = domain_config.provider_config.time_master_config.immediate_resume_time.count();
        config->syncPeriodMs = domain_config.provider_config.time_master_config.sync_period.count();
        config->crcFlags.correction_field = domain_config.eth_time_domain.crcFlags.correction_field ? TSYNC_CRC_SUPPORTED : TSYNC_CRC_NOT_SUPPORTED;
        config->crcFlags.domain_number = domain_config.eth_time_domain.crcFlags.domain_number ? TSYNC_CRC_SUPPORTED : TSYNC_CRC_NOT_SUPPORTED;
        config->crcFlags.message_length = domain_config.eth_time_domain.crcFlags.message_length ? TSYNC_CRC_SUPPORTED : TSYNC_CRC_NOT_SUPPORTED;
        config->crcFlags.precise_origin_timestamp = domain_config.eth_time_domain.crcFlags.precise_origin_timestamp ? TSYNC_CRC_SUPPORTED : TSYNC_CRC_NOT_SUPPORTED;
        config->crcFlags.sequence_id = domain_config.eth_time_domain.crcFlags.sequence_id ? TSYNC_CRC_SUPPORTED : TSYNC_CRC_NOT_SUPPORTED;
        config->crcFlags.source_port_identity = domain_config.eth_time_domain.crcFlags.source_port_identity ? TSYNC_CRC_SUPPORTED : TSYNC_CRC_NOT_SUPPORTED;
        config->globalTimeSequenceCounterJumpWidth =
            domain_config.consumer_config.time_slave_config.global_time_sequence_counter_jump_width;

#if 0
        if((role_requested == TSYNC_ROLE_MASTER) && (domain_config.provider_config.time_master_config.is_valid == true)) {
            config->subTlvConfig.followUpStatusSubTLV = domain_config.provider_config.time_master_config.sub_tlv_config.status_enabled ?
                                                            TSYNC_SUBTLV_SUPPORTED : TSYNC_SUBTLV_NOT_SUPPORTED;
            config->subTlvConfig.followUpTimeSubTLV = domain_config.provider_config.time_master_config.sub_tlv_config.time_enabled ?
                                                            TSYNC_SUBTLV_SUPPORTED : TSYNC_SUBTLV_NOT_SUPPORTED;
            config->subTlvConfig.followUpUserDataSubTLV = domain_config.provider_config.time_master_config.sub_tlv_config.user_data_enabled ?
                                                            TSYNC_SUBTLV_SUPPORTED : TSYNC_SUBTLV_NOT_SUPPORTED;
        } else if (role_requested == TSYNC_ROLE_SLAVE) {
            config->subTlvConfig.followUpStatusSubTLV = domain_config.consumer_config.time_slave_config.sub_tlv_config.status_enabled ?
                                                            TSYNC_SUBTLV_SUPPORTED : TSYNC_SUBTLV_NOT_SUPPORTED;
            config->subTlvConfig.followUpTimeSubTLV = domain_config.consumer_config.time_slave_config.sub_tlv_config.time_enabled ?
                                                            TSYNC_SUBTLV_SUPPORTED : TSYNC_SUBTLV_NOT_SUPPORTED;
            config->subTlvConfig.followUpUserDataSubTLV = domain_config.consumer_config.time_slave_config.sub_tlv_config.user_data_enabled ?
                                                            TSYNC_SUBTLV_SUPPORTED : TSYNC_SUBTLV_NOT_SUPPORTED;
        } else {
            config->subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_NOT_SUPPORTED;
            config->subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_NOT_SUPPORTED;
            config->subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_NOT_SUPPORTED;
        }
#endif

        // TODO: SubTLV features are not currently supported (for more details see #94128)
        config->subTlvConfig.followUpStatusSubTLV = TSYNC_SUBTLV_NOT_SUPPORTED;
        config->subTlvConfig.followUpTimeSubTLV = TSYNC_SUBTLV_NOT_SUPPORTED;
        config->subTlvConfig.followUpUserDataSubTLV = TSYNC_SUBTLV_NOT_SUPPORTED;

        return E_OK;
    }
    return E_NOT_OK;
}
