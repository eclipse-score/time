/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_UTILITY_ITIMEBASEACCESSOR_H_
#define SCORE_TIME_UTILITY_ITIMEBASEACCESSOR_H_

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include "TsyncSharedUtils.h"

namespace score {
namespace time {

class ITimeBaseAccessor {
public:
    enum class State : std::uint32_t { Open, Closed };

    virtual ~ITimeBaseAccessor() noexcept = default;

    virtual void Open() /* noexcept */ = 0;
    virtual void Close() /* noexcept */ = 0;
    virtual State GetState() const /* noexcept */ = 0;
    virtual std::string_view GetName() const /* noexcept */ = 0;
};


class ITimeBaseReader;
class ITimeBaseWriter;

template <class ReaderWriterType, class FIELDTYPE>
typename std::enable_if<(std::is_base_of<ITimeBaseReader, ReaderWriterType>::value ||
                         std::is_base_of<ITimeBaseWriter, ReaderWriterType>::value),
                        bool>::type
Align(ReaderWriterType& a) {
    std::size_t pos{a.GetPosition()};
    const std::size_t alignment_mod{pos % alignof(FIELDTYPE)};
    if (alignment_mod != 0) {
        if (SafeAdd<std::size_t, std::size_t>(pos, (alignof(FIELDTYPE) - alignment_mod), a.GetMaxSize() - 1)) {
            return a.SetPosition(pos);
        } else {
            return false;
        }
    }

    return true;
}

template <class ReaderWriterType, class FIELDTYPE>
typename std::enable_if<(std::is_base_of<ITimeBaseReader, ReaderWriterType>::value ||
                         std::is_base_of<ITimeBaseWriter, ReaderWriterType>::value),
                        bool>::type
AlignedSkip(ReaderWriterType& a) {
    std::size_t pos = a.GetPosition();

    if (SafeAdd<std::size_t, std::size_t>(pos, sizeof(FIELDTYPE), a.GetMaxSize() - 1)) {
        return (a.SetPosition(pos) && Align<ReaderWriterType, FIELDTYPE>(a));
    }

    return false;
}

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_UTILITY_ITIMEBASEACCESSOR_H_
