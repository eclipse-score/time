/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_UTILITY_ITIMEBASEREADER_H_
#define SCORE_TIME_UTILITY_ITIMEBASEREADER_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "ITimeBaseAccessor.h"
#include "TsyncConfigTypes.h"

namespace score {
namespace time {

class ITimeBaseReader {
public:
    virtual ~ITimeBaseReader() noexcept = default;

    virtual ITimeBaseAccessor& GetAccessor() /* noexcept */ = 0;

    virtual bool Read(std::uint8_t& data) /* noexcept */ = 0;
    virtual bool Read(std::uint16_t& data) /* noexcept */ = 0;
    virtual bool Read(std::uint32_t& data) /* noexcept */ = 0;
    virtual bool Read(std::uint64_t& data) /* noexcept */ = 0;
    virtual bool Read(std::int8_t& data) /* noexcept */ = 0;
    virtual bool Read(std::int16_t& data) /* noexcept */ = 0;
    virtual bool Read(std::int32_t& data) /* noexcept */ = 0;
    virtual bool Read(std::int64_t& data) /* noexcept */ = 0;
    // these may not be portable
    virtual bool Read(float& data) /* noexcept */ = 0;
    virtual bool Read(double& data) /* noexcept */ = 0;

    virtual bool Read(std::string& data) /* noexcept */ = 0;
    // the std::string_view instance will become invalid when the reader is closed.
    virtual bool Read(std::string_view& data) /* noexcept */ = 0;

    // Function to remove dependency on ptp-lib
    virtual bool Read(TimestampWithStatus& data) /* noexcept */ = 0;
    virtual bool Read(VirtualLocalTime& data) /* noexcept */ = 0;
    virtual bool Read(UserDataView& data) /* noexcept */ = 0;
    virtual bool Read(SynchronizationStatus& data) /* noexcept */ = 0;

    // zero-copy read, performance vs. type-safety
    virtual bool Read(const char** p, std::size_t numBytes) /* noexcept */ = 0;

    virtual bool Read(TsyncTimeDomainConfig& data) /* noexcept */ = 0;
    virtual bool Read(TsyncConsumerConfig& data) /* noexcept */ = 0;
    virtual bool Read(TsyncProviderConfig& data) /* noexcept */ = 0;

    virtual bool Skip(std::size_t numBytes) /* noexcept */ = 0;
    virtual bool SetPosition(std::size_t pos) /* noexcept */ = 0;
    virtual std::size_t GetPosition() const /* noexcept */ = 0;
    virtual std::size_t GetMaxSize() const /* noexcept */ = 0;

    // std::BasicLockable requirements
    virtual void lock() /* noexcept */ = 0;
    virtual void unlock() /* noexcept */ = 0;
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_UTILITY_ITIMEBASEREADER_H_
