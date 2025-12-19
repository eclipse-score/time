/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATBUFFERS_GENERATED_TSYNCFLATCFG_COMFLATBUFFER_H_
#define FLATBUFFERS_GENERATED_TSYNCFLATCFG_COMFLATBUFFER_H_

#include <gmock/gmock.h>

#include "flatbuffers/flatbuffers.h"

static bool g_GetTSYNCEcuCfg_nullptr = false;

namespace TSYNCFlatBuffer {

enum EthGlobalTimeMessageFormatEnum : int8_t {
    EthGlobalTimeMessageFormatEnum_IEEE802_1AS = 0,
    EthGlobalTimeMessageFormatEnum_IEEE802_1AS_AUTOSAR = 1,
    EthGlobalTimeMessageFormatEnum_MIN = EthGlobalTimeMessageFormatEnum_IEEE802_1AS,
    EthGlobalTimeMessageFormatEnum_MAX = EthGlobalTimeMessageFormatEnum_IEEE802_1AS_AUTOSAR
};

enum GlobalTimeCrcSupportEnum : int8_t {
    GlobalTimeCrcSupportEnum_CrcSupported = 0,
    GlobalTimeCrcSupportEnum_CrcNotSupported = 1,
    GlobalTimeCrcSupportEnum_MIN = GlobalTimeCrcSupportEnum_CrcSupported,
    GlobalTimeCrcSupportEnum_MAX = GlobalTimeCrcSupportEnum_CrcNotSupported
};

enum GlobalTimeCrcValidationEnum : int8_t {
    GlobalTimeCrcValidationEnum_CrcValidated = 0,
    GlobalTimeCrcValidationEnum_CrcNotValidated = 1,
    GlobalTimeCrcValidationEnum_CrcIgnored = 2,
    GlobalTimeCrcValidationEnum_CrcOptional = 3,
    GlobalTimeCrcValidationEnum_MIN = GlobalTimeCrcValidationEnum_CrcValidated,
    GlobalTimeCrcValidationEnum_MAX = GlobalTimeCrcValidationEnum_CrcOptional
};

enum PncGatewayTypeEnum : int8_t {
    PncGatewayTypeEnum_None = 0,
    PncGatewayTypeEnum_Active = 1,
    PncGatewayTypeEnum_Passive = 2,
    PncGatewayTypeEnum_MIN = PncGatewayTypeEnum_None,
    PncGatewayTypeEnum_MAX = PncGatewayTypeEnum_Passive
};

class Mock_CrcFlags {
   public:
    MOCK_METHOD0(crcCorrectionField, bool());
    MOCK_METHOD0(crcDomainNumber, bool());
    MOCK_METHOD0(crcMessageLength, bool());
    MOCK_METHOD0(crcPreciseOriginTimestamp, bool());
    MOCK_METHOD0(crcSequenceId, bool());
    MOCK_METHOD0(crcSourcePortIdentity, bool());
};
class Mock_EthGlobalTimeDomainProps {
   public:
    MOCK_CONST_METHOD0(messageCompliance, EthGlobalTimeMessageFormatEnum());
    MOCK_CONST_METHOD0(destinationPhysicalAddress, flatbuffers::String*());
    MOCK_CONST_METHOD0(fupDataIdList, flatbuffers::Vector<uint8_t>*());
    MOCK_CONST_METHOD0(vlanPriority, uint32_t());
    MOCK_CONST_METHOD0(crcFlags, flatbuffers::Vector<flatbuffers::Offset<Mock_CrcFlags>>*());
};

class Mock_SubTlvConfig {
   public:
    MOCK_METHOD0(statusSubTlv, bool());
    MOCK_METHOD0(timeSubTlv, bool());
    MOCK_METHOD0(userDataSubTlv, bool());
};

class Mock_GlobalTimeEthSlave {
   public:
    MOCK_METHOD0(crcValidated, GlobalTimeCrcValidationEnum());
    MOCK_METHOD0(timeLeapHealingCounter, uint32_t());
    MOCK_METHOD0(followUpTimeoutValue, double());
    MOCK_METHOD0(timeLeapFutureThreshold, double());
    MOCK_METHOD0(timeLeapPastThreshold, double());
    MOCK_METHOD0(globalTimeSequenceCounterJumpWidth, uint32_t());
    MOCK_CONST_METHOD0(subTlvConfig, flatbuffers::Vector<flatbuffers::Offset<Mock_SubTlvConfig>>*());
};

class Mock_SynchronizedTimeBaseConsumer {
   public:
    MOCK_CONST_METHOD0(shortName, flatbuffers::String*());
    MOCK_CONST_METHOD0(networkTimeConsumer, Mock_GlobalTimeEthSlave*());
};

class Mock_GlobalTimeEthMaster {
   public:
    MOCK_METHOD0(immediateResumeTime, double());
    MOCK_METHOD0(syncPeriod, double());
    MOCK_METHOD0(isSystemWideGlobalTimeMaster, bool());
    MOCK_METHOD0(crcSecured, GlobalTimeCrcSupportEnum());
    MOCK_CONST_METHOD0(subTlvConfig, flatbuffers::Vector<flatbuffers::Offset<Mock_SubTlvConfig>>*());
};

class Mock_TimeSyncCorrection {
   public:
    MOCK_METHOD0(allowProviderRateCorrection, bool());
    MOCK_METHOD0(rateCorrectionsPerMeasurementDuration, uint32_t());
    MOCK_METHOD0(offsetCorrectionAdaptionInterval, double());
    MOCK_METHOD0(offsetCorrectionJumpThreshold, double());
    MOCK_METHOD0(rateDeviationMeasurementDuration, double());
};

class Mock_SynchronizedTimeBaseProvider {
   public:
    MOCK_CONST_METHOD0(shortName, flatbuffers::String*());
    MOCK_CONST_METHOD0(networkTimeProvider, Mock_GlobalTimeEthMaster*());
    MOCK_CONST_METHOD0(timeSyncCorrection, flatbuffers::Vector<flatbuffers::Offset<Mock_TimeSyncCorrection>>*());
};

class Mock_GlobalTimeDomain {
   public:
    MOCK_METHOD0(shortName, flatbuffers::String*());
    MOCK_METHOD0(domainId, uint32_t());
    MOCK_METHOD0(syncLossTimeout, double());
    MOCK_METHOD0(debounceTime, double());
    MOCK_METHOD0(ethGlobalTimeDomainProps, flatbuffers::Vector<flatbuffers::Offset<Mock_EthGlobalTimeDomainProps>>*());
    MOCK_CONST_METHOD0(synchronizedTimeBaseProviders,
                       flatbuffers::Vector<flatbuffers::Offset<Mock_SynchronizedTimeBaseProvider>>*());
    MOCK_CONST_METHOD0(synchronizedTimeBaseConsumers,
                       flatbuffers::Vector<flatbuffers::Offset<Mock_SynchronizedTimeBaseConsumer>>*());
};

class Mock_TSYNCEcuCfg {
   public:
    MOCK_CONST_METHOD0(globalTimeDomain, flatbuffers::Vector<flatbuffers::Offset<Mock_GlobalTimeDomain>>*());
};

using EthGlobalTimeDomainProps = Mock_EthGlobalTimeDomainProps;
using SubTlvConfig = Mock_SubTlvConfig;
using GlobalTimeEthSlave = Mock_GlobalTimeEthSlave;
using SynchronizedTimeBaseConsumer = Mock_SynchronizedTimeBaseConsumer;
using GlobalTimeEthMaster = Mock_GlobalTimeEthMaster;
using TimeSyncCorrection = Mock_TimeSyncCorrection;
using SynchronizedTimeBaseProvider = Mock_SynchronizedTimeBaseProvider;
using GlobalTimeDomain = Mock_GlobalTimeDomain;
using CrcFlags = Mock_CrcFlags;
using TSYNCEcuCfg = Mock_TSYNCEcuCfg;

inline const TSYNCEcuCfg* GetTSYNCEcuCfg(const void* buf) {
    if (g_GetTSYNCEcuCfg_nullptr) {
        return nullptr;
    }
    return const_cast<TSYNCEcuCfg*>(static_cast<const TSYNCEcuCfg*>(buf));
}
}  // namespace TSYNCFlatBuffer
#endif  // FLATBUFFERS_GENERATED_TSYNCFLATCFG_COMFLATBUFFER_H_
