/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "TsyncIdMappingsHandler.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <limits>
#include <mutex>
#include <sstream>

#include "score/time/common/Abort.h"
#include "SysCalls.h"

using score::time::common::logFatalAndAbort;

namespace score {
namespace time {

constexpr std::uint64_t kConsumerMappingStartMagic = 0xabcddcbaabcddcba;
constexpr std::uint64_t kProviderMappingStartMagic = 0x1122334444332211;

TsyncIdMappingsHandler::TsyncIdMappingsHandler(TsyncIdMappingsHandler&& rhs) = default;

TsyncIdMappingsHandler& TsyncIdMappingsHandler::operator=(TsyncIdMappingsHandler&& rhs) = default;

TsyncIdMappingsHandler::iterator TsyncIdMappingsHandler::begin() noexcept {
    return time_domains_.begin();
}

TsyncIdMappingsHandler::const_iterator TsyncIdMappingsHandler::cbegin() const noexcept {
    return time_domains_.cbegin();
}

TsyncIdMappingsHandler::iterator TsyncIdMappingsHandler::end() noexcept {
    return time_domains_.end();
}

TsyncIdMappingsHandler::const_iterator TsyncIdMappingsHandler::cend() const noexcept {
    return time_domains_.cend();
}

bool TsyncIdMappingsHandler::AddDomainMapping(TsyncIdMappingsHandler::TimeDomainId domain_id,
                                              std::string_view domain_name) noexcept {
    if (domain_id > kMaxTimebaseId) {
        std::cerr << "TsyncIdMappingsHandler::AddDomainMapping(): domain id " << domain_id << " for domain "
                  << domain_name << " is invalid" << std::endl;
        return false;
    } else {
        return time_domains_.insert({domain_id, domain_name}).second;
    }
}

bool TsyncIdMappingsHandler::AddDomainMapping(const TimeDomainMapping& mapping) noexcept {
    if (mapping.first > kMaxTimebaseId) {
        return false;
    } else {
        return time_domains_.insert(mapping).second;
    }
}

bool TsyncIdMappingsHandler::AddConsumerToDomain(std::string_view domain_name, std::string_view consumer_name) noexcept {
    auto id = GetDomainId(domain_name);
    if (id && *id <= kMaxTimebaseId) {
        auto it = consumer_to_time_domain_mappings_.insert(
            TimeConsumerToTimeDomainMapping(consumer_name, TimeDomainMapping(*id, domain_name)));
        return it.second;
    }

    return false;
}

bool TsyncIdMappingsHandler::AddConsumerToDomain(TsyncIdMappingsHandler::TimeDomainId domain_id,
                                                 std::string_view consumer_name) noexcept {
    auto domain_name = GetDomainName(domain_id);
    if (domain_name) {
        auto it = consumer_to_time_domain_mappings_.insert(
            TimeConsumerToTimeDomainMapping(consumer_name, TimeDomainMapping(domain_id, *domain_name)));
        return it.second;
    }

    return false;
}

std::optional<std::string_view> TsyncIdMappingsHandler::GetDomainNameForConsumer(std::string_view consumer_name) {
    DoSharedMemoryRead();
    auto it = consumer_to_time_domain_mappings_.find(consumer_name);
    if (it != consumer_to_time_domain_mappings_.end()) {
        return it->second.second;
    }

    return std::nullopt;
}

std::optional<TsyncIdMappingsHandler::TimeDomainId> TsyncIdMappingsHandler::GetDomainIdForConsumer(
    std::string_view consumer_name) {
    DoSharedMemoryRead();
    auto it = consumer_to_time_domain_mappings_.find(consumer_name);
    if (it != consumer_to_time_domain_mappings_.end()) {
        return it->second.first;
    }
    return std::nullopt;
}

bool TsyncIdMappingsHandler::AddProviderToDomain(std::string_view domain_name, std::string_view provider_name) noexcept {
    auto id = GetDomainId(domain_name);
    if (id && *id <= kMaxTimebaseId) {
        auto prov = GetProviderForDomain(*id);
        if (!prov) {
            auto it = provider_to_time_domain_mappings_.insert(
                TimeProviderToTimeDomainMapping(provider_name, TimeDomainMapping(*id, domain_name)));
            return it.second;
        }
    }

    return false;
}

bool TsyncIdMappingsHandler::AddProviderToDomain(TsyncIdMappingsHandler::TimeDomainId domain_id,
                                                 std::string_view provider_name) noexcept {
    auto domain_name = GetDomainName(domain_id);
    if (domain_name) {
        auto prov = GetProviderForDomain(*domain_name);
        if (!prov) {
            auto it = provider_to_time_domain_mappings_.insert(
                TimeProviderToTimeDomainMapping(provider_name, TimeDomainMapping(domain_id, *domain_name)));
            return it.second;
        }
    }

    return false;
}

std::optional<std::string_view> TsyncIdMappingsHandler::GetProviderForDomain(std::string_view domain_name) noexcept {
    DoSharedMemoryRead();
    auto res = std::find_if(provider_to_time_domain_mappings_.begin(), provider_to_time_domain_mappings_.end(),
                            [domain_name](const auto& it) -> bool { return (it.second.second == domain_name); });
    if (res != provider_to_time_domain_mappings_.end()) {
        return res->first;
    }
    return std::nullopt;
}

std::optional<std::string_view> TsyncIdMappingsHandler::GetProviderForDomain(
    TsyncIdMappingsHandler::TimeDomainId domain_id) noexcept {
    if (domain_id > kMaxTimebaseId) {
        return std::nullopt;
    }

    DoSharedMemoryRead();
    auto res = std::find_if(provider_to_time_domain_mappings_.begin(), provider_to_time_domain_mappings_.end(),
                            [domain_id](const auto& it) -> bool { return (it.second.first == domain_id); });

    if (res != provider_to_time_domain_mappings_.end()) {
        return res->first;
    }

    return std::nullopt;
}

std::optional<std::string_view> TsyncIdMappingsHandler::GetDomainNameForProvider(std::string_view provider_name) {
    DoSharedMemoryRead();
    auto it = provider_to_time_domain_mappings_.find(provider_name);
    if (it != provider_to_time_domain_mappings_.end()) {
        return it->second.second;
    }
    return std::nullopt;
}

std::optional<TsyncIdMappingsHandler::TimeDomainId> TsyncIdMappingsHandler::GetDomainIdForProvider(
    std::string_view provider_name) {
    DoSharedMemoryRead();
    auto it = provider_to_time_domain_mappings_.find(provider_name);
    if (it != provider_to_time_domain_mappings_.end()) {
        return it->second.first;
    }
    return std::nullopt;
}

void TsyncIdMappingsHandler::Clear() {
    writer_.reset();
    reader_.reset();
    provider_to_time_domain_mappings_.clear();
    consumer_to_time_domain_mappings_.clear();
    time_domains_.clear();
}

std::optional<TsyncIdMappingsHandler::TimeDomainId> TsyncIdMappingsHandler::GetDomainId(std::string_view domain_name) noexcept {
    DoSharedMemoryRead();
    for (auto it : time_domains_) {
        if (it.second == domain_name) {
            return std::optional<TsyncIdMappingsHandler::TimeDomainId>(it.first);
        }
    }
    return std::nullopt;
}

std::optional<std::string_view> TsyncIdMappingsHandler::GetDomainName(TsyncIdMappingsHandler::TimeDomainId domain_id) noexcept {
    if (domain_id <= kMaxTimebaseId) {
        DoSharedMemoryRead();
        auto it = time_domains_.find(domain_id);
        if (it != time_domains_.end()) {
            return std::optional<std::string_view>(it->second);
        }
    }
    return std::nullopt;
}

bool TsyncIdMappingsHandler::AddConsumerToDomainMapping(uint32_t domain_id, std::string_view consumer_name) noexcept {
    if (IsEmpty()) {
        return false;
    }

    auto domain = time_domains_.find(domain_id);
    if (domain == time_domains_.end()) {
        return false;
    }

    std::string_view domain_name(domain->second);

    auto it = consumer_to_time_domain_mappings_.insert(
        TimeConsumerToTimeDomainMapping(consumer_name, TimeDomainMapping(domain_id, domain_name)));
    return it.second;
}

bool TsyncIdMappingsHandler::AddProviderToDomainMapping(uint32_t domain_id, std::string_view provider_name) noexcept {
    if (IsEmpty()) {
        return false;
    }

    auto domain = time_domains_.find(domain_id);
    if (domain == time_domains_.end()) {
        return false;
    }

    std::string_view domain_name(domain->second);

    auto it = provider_to_time_domain_mappings_.insert(
        TimeProviderToTimeDomainMapping(provider_name, TimeDomainMapping(domain_id, domain_name)));
    return it.second;
}

void TsyncIdMappingsHandler::CommitMappingsToSharedMemory() noexcept {
    if (!writer_) {
        writer_ = TimeBaseWriterFactory::Create(kIdMappingsShmemFileName, kIdMappingsShmemSize, true);
    }

    writer_->GetAccessor().Open();

    std::lock_guard<ITimeBaseWriter> lock(*writer_);
    std::size_t num_domains{time_domains_.size()};

    if (num_domains > kMaxNumberOfTimeDomains) {
        std::stringstream ss;
        ss << "TsyncIdMappingsHandler::CommitMappingsToSharedMemory(): "
           << "Number of time domains has exceeded the maximum permissible value of " << kMaxNumberOfTimeDomains;
        logFatalAndAbort(ss.str().c_str());
    }

    bool res = writer_->Write(static_cast<std::uint32_t>(num_domains));

    // write time domain mappings
    auto it = time_domains_.begin();
    while (res && it != time_domains_.end()) {
        res = (writer_->Write(it->first) && writer_->Write(it->second));
        ++it;
    }

    if (!res) {
        logFatalAndAbort("TsyncIdMappingsHandler::CommitMappingsToSharedMemory(): Error writing time domain mappings.");
    }

    // write consumer mappings
    std::size_t num_consumer_mappings{consumer_to_time_domain_mappings_.size()};

    if (num_consumer_mappings > kMaxConsumersPerTimeDomain) {
        std::stringstream ss;
        ss << "TsyncIdMappingsHandler::CommitMappingsToSharedMemory(): "
           << "Number of consumer mappings has exceeded the maximum permissible value of "
           << kMaxConsumersPerTimeDomain;
        logFatalAndAbort(ss.str().c_str());
    }

    res = (writer_->Write(kConsumerMappingStartMagic) &&
           writer_->Write(static_cast<std::uint32_t>(num_consumer_mappings)));

    if (!res) {
        logFatalAndAbort("TsyncIdMappingsHandler::CommitMappingsToSharedMemory(): Error writing consumer mappings header.");
    }
    auto consumer_it = consumer_to_time_domain_mappings_.begin();
    while (res && consumer_it != consumer_to_time_domain_mappings_.end()) {
        // storing the consumer name and the id of the time domain
        res = (writer_->Write(consumer_it->first) && writer_->Write(consumer_it->second.first));
        ++consumer_it;
    }

    if (!res) {
        logFatalAndAbort("TsyncIdMappingsHandler::CommitMappingsToSharedMemory(): Error writing consumer mappings.");
    }

    // write provider mappings
    std::size_t num_provider_mappings{provider_to_time_domain_mappings_.size()};
    res = (writer_->Write(kProviderMappingStartMagic) &&
           num_provider_mappings < std::numeric_limits<std::uint32_t>::max() &&
           writer_->Write(static_cast<std::uint32_t>(num_provider_mappings)));

    if (!res) {
        logFatalAndAbort("TsyncIdMappingsHandler::CommitMappingsToSharedMemory(): Error writing provider mappings header.");
    }

    auto provider_it = provider_to_time_domain_mappings_.begin();

    while (res && provider_it != provider_to_time_domain_mappings_.end()) {
        // storing the provider name and the id of the time domain
        res = (writer_->Write(provider_it->first) && writer_->Write(provider_it->second.first));

        ++provider_it;
    }

    if (!res) {
        logFatalAndAbort("TsyncIdMappingsHandler::CommitMappingsToSharedMemory(): Error writing provider mappings.");
    }
}

void TsyncIdMappingsHandler::DoSharedMemoryRead() noexcept {
    if (!IsEmpty()) {
        return;
    }

    if (!reader_) {
        reader_ = TimeBaseReaderFactory::Create(kIdMappingsShmemFileName, kIdMappingsShmemSize);
    }

    reader_->GetAccessor().Open();
    {
        std::lock_guard<ITimeBaseReader> lock(*reader_);
        std::uint32_t num_domains = 0U;
        // coverity[autosar_cpp14_a0_1_1_violation:FALSE] #49860: This value is used subsequently.
        bool res = reader_->Read(num_domains);

        if (!res || num_domains == 0U) {
            logFatalAndAbort("TsyncIdMappingsHandler::DoSharedMemoryRead(): No domain mappings found.");
        }

        while (res && (num_domains-- != 0U)) {
            std::string_view sv{};
            TimeDomainId id{};
            res = (reader_->Read(id) && reader_->Read(sv));
            if (res) {
                AddDomainMapping(id, sv);
                id = 0;
            } else {
                logFatalAndAbort("TsyncIdMappingsHandler::DoSharedMemoryRead(): Error reading domain mapping.");
            }
        }

        std::uint64_t magic{0ULL};
        std::uint32_t num_mappings{0U};

        res = (reader_->Read(magic) && reader_->Read(num_mappings) && (num_mappings > 0u));
        if (res && magic != kConsumerMappingStartMagic) {
            logFatalAndAbort("Error reading start of consumer mappings");
        }

        if (!res) {
            consumer_to_time_domain_mappings_.clear();
        } else {
            while (res && (num_mappings-- != 0U)) {
                std::string_view consumer_name;
                TimeDomainId domain_id{};
                res = (reader_->Read(consumer_name) && reader_->Read(domain_id));
                if (res) {
                    res = AddConsumerToDomainMapping(domain_id, consumer_name);
                } else {
                    logFatalAndAbort("TsyncIdMappingsHandler::DoSharedMemoryRead(): Error reading consumer mapping.");
                }
            }
        }

        res = (reader_->Read(magic) && reader_->Read(num_mappings) && (num_mappings > 0u));
        if (magic != kProviderMappingStartMagic) {
            logFatalAndAbort("Error reading start of provider mappings");
        }

        while (res && (num_mappings-- != 0U)) {
            std::string_view provider_name;
            TimeDomainId domain_id{};
            res = (reader_->Read(provider_name) && reader_->Read(domain_id));
            if (res) {
                res = AddProviderToDomainMapping(domain_id, provider_name);
                if (!res) {
                    std::stringstream ss;
                    ss << "TsyncIdMappingsHandler::DoSharedMemoryRead(): "
                       << "Error adding provider " << provider_name << " to domain " << domain_id;
                    logFatalAndAbort(ss.str().c_str());
                }

            } else {
                logFatalAndAbort("TsyncIdMappingsHandler::DoSharedMemoryRead(): Error reading provider mapping.");
            }
        }
    }
}

void TsyncIdMappingsHandler::DumpMappings() const {
    std::cerr << "Time domains:" << std::endl;
    std::cerr << "-------------" << std::endl;
    for (auto& it : time_domains_) {
        std::cerr << "Domain id: " << it.first << " / Domain name: " << it.second << std::endl << std::endl;
    }

    std::cerr << "Consumer to time domain mappings:" << std::endl;
    std::cerr << "---------------------------------" << std::endl;
    for (auto it : consumer_to_time_domain_mappings_) {
        std::cerr << "Consumer name: " << it.first << " / Domain name: " << it.second.second << std::endl << std::endl;
    }

    std::cerr << "Provider to time domain mappings:" << std::endl;
    std::cerr << "---------------------------------" << std::endl;
    for (auto it : provider_to_time_domain_mappings_) {
        std::cerr << "Provider name: " << it.first << " / Domain name: " << it.second.second << std::endl;
    }
}

}  // namespace time
}  // namespace score
