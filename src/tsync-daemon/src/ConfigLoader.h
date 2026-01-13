/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_DAEMON_CONFIGLOADER_H_
#define SCORE_TIME_DAEMON_CONFIGLOADER_H_

#include <memory>
#include <string>

namespace TSYNCFlatBuffer {
struct TSYNCEcuCfg;
struct EthGlobalTimeDomainProps;
struct SynchronizedTimeBaseConsumer;
struct SynchronizedTimeBaseProvider;
}

namespace score {
namespace time {

struct TsyncEthTimeDomain;
struct TsyncConsumerConfig;
struct TsyncProviderConfig;

namespace daemon {

class TimeBaseConfiguration;

class ConfigLoader final {
public:
    ConfigLoader();
    ~ConfigLoader() = default;
    ConfigLoader(const ConfigLoader&) = delete;
    ConfigLoader& operator=(const ConfigLoader&) = delete;
    ConfigLoader(ConfigLoader&&) = delete;
    ConfigLoader& operator=(ConfigLoader&&) = delete;

    const TimeBaseConfiguration& GetConfig() const noexcept;
    void DumpConfig() noexcept;

private:
    void ParseDomainConfig(std::shared_ptr<void> cfg);
    void ParseEthernetDomainConfig(const std::string& domain_name,
                                   const TSYNCFlatBuffer::EthGlobalTimeDomainProps& cfg,
                                   TsyncEthTimeDomain& out);
    void ParseConsumerConfig(const std::string& domain_name,
                             const TSYNCFlatBuffer::SynchronizedTimeBaseConsumer& cfg,
                             TsyncConsumerConfig& out);
    void ParseProviderConfig(const std::string& domain_name,
                             const TSYNCFlatBuffer::SynchronizedTimeBaseProvider& cfg,
                             TsyncProviderConfig& out);
    const TSYNCFlatBuffer::TSYNCEcuCfg* GetEcuCfg(std::shared_ptr<void> cfg);
};

}  // namespace daemon
}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_DAEMON_CONFIGLOADER_H_
