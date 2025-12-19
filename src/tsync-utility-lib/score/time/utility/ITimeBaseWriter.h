/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_UTILITY_ITIMEBASEWRITER_H_
#define SCORE_TIME_UTILITY_ITIMEBASEWRITER_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "ITimeBaseAccessor.h"
#include "TsyncConfigTypes.h"

namespace score {
namespace time {

struct TsyncConsumerConfig;
struct TsyncProviderConfig;

class ITimeBaseWriter {
public:
    virtual ~ITimeBaseWriter() noexcept = default;

    virtual ITimeBaseAccessor& GetAccessor() /* noexcept */ = 0;

    virtual bool Write(std::uint8_t data) /* noexcept */ = 0;
    virtual bool Write(std::uint16_t data) /* noexcept */ = 0;
    virtual bool Write(std::uint32_t data) /* noexcept */ = 0;
    virtual bool Write(std::uint64_t data) /* noexcept */ = 0;
    virtual bool Write(std::int8_t data) /* noexcept */ = 0;
    virtual bool Write(std::int16_t data) /* noexcept */ = 0;
    virtual bool Write(std::int32_t data) /* noexcept */ = 0;
    virtual bool Write(std::int64_t data) /* noexcept */ = 0;
    virtual bool Write(const std::string& data) /* noexcept */ = 0;
    virtual bool Write(std::string_view data) /* noexcept */ = 0;
    // Function to remove dependency on ptp-lib
    virtual bool Write(const TimestampWithStatus& data) /* noexcept */ = 0;
    virtual bool Write(const VirtualLocalTime& data) /* noexcept */ = 0;
    virtual bool Write(UserData data) /* noexcept */ = 0;
    virtual bool Write(UserDataView data) /* noexcept */ = 0;

    virtual bool Write(const SynchronizationStatus& data) /* noexcept */ = 0;
    // these may not be portable
    virtual bool Write(float data) /* noexcept */ = 0;
    virtual bool Write(double data) /* noexcept */ = 0;

    virtual bool Write(const TsyncTimeDomainConfig& data) /* noexcept */ = 0;
    virtual bool Write(const TsyncConsumerConfig& data) /* noexcept */ = 0;
    virtual bool Write(const TsyncProviderConfig& data) /* noexcept */ = 0;
    virtual bool Write(const char* data, std::size_t size) /* noexcept */ = 0;

    virtual bool WriteDefaults() /* noexcept */ = 0;
    virtual bool Skip(std::size_t num_bytes) /* noexcept */ = 0;
    virtual bool SetPosition(std::size_t pos) /* noexcept */ = 0;
    virtual std::size_t GetPosition() const /* noexcept */ = 0;
    virtual std::size_t GetMaxSize() const /* noexcept */ = 0;

    // std::BasicLockable requirements
    virtual void lock() /* noexcept */ = 0;
    virtual void unlock() /* noexcept */ = 0;
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_UTILITY_ITIMEBASEWRITER_H_
