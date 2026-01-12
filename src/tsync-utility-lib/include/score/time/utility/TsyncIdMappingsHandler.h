/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_UTILITY_TSYNCIDMAPPINGSHANDLER_H_
#define SCORE_TIME_UTILITY_TSYNCIDMAPPINGSHANDLER_H_

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "score/time/utility/ITimeBaseReader.h"
#include "score/time/utility/ITimeBaseWriter.h"
#include "score/time/utility/TimeBaseReaderFactory.h"
#include "score/time/utility/TimeBaseWriterFactory.h"

namespace score {
namespace time {

/* Name of the shared memory file that holds the timebase identifier
 * mappings */
constexpr const char* kIdMappingsShmemFileName{"/tsync_id_mappings"};
/* Size of the shared memory file that holds the timebase identifier
 * mappings */
constexpr size_t kIdMappingsShmemSize{8192u};

class TsyncIdMappingsHandler final {
public:
    using TimeDomainId = uint32_t;
    // domain id / domain name
    using TimeDomainMapping = std::pair<TimeDomainId, std::string_view>;
    // consumer name / domain mapping
    using TimeConsumerToTimeDomainMapping = std::pair<std::string_view, TimeDomainMapping>;
    // provider name / domain mapping
    using TimeProviderToTimeDomainMapping = TimeConsumerToTimeDomainMapping;
    using iterator = std::unordered_map<TimeDomainMapping::first_type, TimeDomainMapping::second_type>::iterator;
    using const_iterator =
        std::unordered_map<TimeDomainMapping::first_type, TimeDomainMapping::second_type>::const_iterator;

    constexpr TsyncIdMappingsHandler() = default;
    TsyncIdMappingsHandler(const TsyncIdMappingsHandler&) = delete;
    TsyncIdMappingsHandler(TsyncIdMappingsHandler&&);
    TsyncIdMappingsHandler& operator=(const TsyncIdMappingsHandler&) = delete;
    TsyncIdMappingsHandler& operator=(TsyncIdMappingsHandler&&);
    ~TsyncIdMappingsHandler() = default;

    // timedomain mappings
    bool AddDomainMapping(TimeDomainId domain_id, std::string_view domain_name) noexcept;
    bool AddDomainMapping(const TimeDomainMapping& mapping) noexcept;
    std::optional<TimeDomainId> GetDomainId(std::string_view domain_name) noexcept;
    std::optional<std::string_view> GetDomainName(TimeDomainId domain_id) noexcept;

    // consumer to timedomain mappings
    bool AddConsumerToDomain(std::string_view domain_name, std::string_view consumer_name) noexcept;
    bool AddConsumerToDomain(TimeDomainId domain_id, std::string_view consumer_name) noexcept;

    std::optional<std::string_view> GetDomainNameForConsumer(std::string_view consumer_name);
    std::optional<TimeDomainId> GetDomainIdForConsumer(std::string_view consumer_name);

    // provider to timedomain mappings (max 1 provider per domain)
    bool AddProviderToDomain(std::string_view domain_name, std::string_view provider_name) noexcept;
    bool AddProviderToDomain(TimeDomainId domain_id, std::string_view provider_name) noexcept;
    std::optional<std::string_view> GetProviderForDomain(std::string_view domain_name) noexcept;
    std::optional<std::string_view> GetProviderForDomain(TimeDomainId domain_id) noexcept;
    std::optional<std::string_view> GetDomainNameForProvider(std::string_view provider_name);
    std::optional<TimeDomainId> GetDomainIdForProvider(std::string_view provider_name);

    const_iterator cbegin() const noexcept;
    iterator end() noexcept;
    const_iterator cend() const noexcept;
    iterator begin() noexcept;

    void Clear();

    // persistency
    void CommitMappingsToSharedMemory() noexcept;
    void DoSharedMemoryRead() noexcept;

    // Debug
    void DumpMappings() const;

    bool IsEmpty() const {
        return time_domains_.empty();
    }

private:
    // coverity[autosar_cpp14_a0_1_1_violation:FALSE] #49860: This value is used subsequently.
    static constexpr const TimeDomainId kMaxTimebaseId = 15u;
    // coverity[autosar_cpp14_a0_1_1_violation:FALSE] #49860: This value is used subsequently.
    static constexpr const std::size_t kMaxNumberOfTimeDomains = 16u;
    // arbitrary upper limit of consumers per time domain to avoid using std::numeric_limits<>::max for boundary checks
    // clang-format off
    // coverity[autosar_cpp14_a0_1_1_violation:FALSE] #49860: This value is used subsequently.
    static constexpr const std::size_t kMaxConsumersPerTimeDomain = 128u;
    // clang-format on

    // TODO: use safe memory allocator (maybe)
    std::unordered_map<TimeDomainMapping::first_type, TimeDomainMapping::second_type> time_domains_{};
    std::unordered_map<TimeConsumerToTimeDomainMapping::first_type, TimeConsumerToTimeDomainMapping::second_type>
        consumer_to_time_domain_mappings_{};
    std::unordered_map<TimeProviderToTimeDomainMapping::first_type, TimeProviderToTimeDomainMapping::second_type>
        provider_to_time_domain_mappings_{};

    TimeBaseWriterFactory::PointerType writer_{};
    TimeBaseReaderFactory::PointerType reader_{};
    bool AddConsumerToDomainMapping(uint32_t domain_id, std::string_view consumer_name) noexcept;
    bool AddProviderToDomainMapping(uint32_t domain_id, std::string_view provider_name) noexcept;
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_UTILITY_TSYNCIDMAPPINGSHANDLER_H_
