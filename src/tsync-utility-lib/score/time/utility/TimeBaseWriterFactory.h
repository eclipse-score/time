/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_UTILITY_TIMEBASEWRITERFACTORY_H_
#define SCORE_TIME_UTILITY_TIMEBASEWRITERFACTORY_H_

#include <cstddef>
#include <memory>
#include <string_view>

namespace score {
namespace time {

class ITimeBaseWriter;

class TimeBaseWriterFactory {
public:
    using PointerType = std::unique_ptr<ITimeBaseWriter>;
    
    static PointerType Create(std::string_view name, bool is_owner = false);
    static PointerType Create(std::string_view name, std::size_t max_size, bool is_owner = false);
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_UTILITY_TIMEBASEWRITERFACTORY_H_
