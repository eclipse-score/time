/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_UTILITY_TIMEBASEREADERFACTORY_H_
#define SCORE_TIME_UTILITY_TIMEBASEREADERFACTORY_H_

#include <cstddef>
#include <memory>
#include <string_view>

#include "score/time/utility/ITimeBaseReader.h"

namespace score {
namespace time {

class TimeBaseReaderFactory {
public:
    using PointerType = std::unique_ptr<ITimeBaseReader>;

    static PointerType Create(std::string_view name);
    static PointerType Create(std::string_view name, std::size_t max_size);
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_UTILITY_TIMEBASEREADERFACTORY_H_
