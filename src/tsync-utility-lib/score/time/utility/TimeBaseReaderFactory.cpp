/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "TimeBaseReaderFactory.h"

#include "SharedMemTimeBaseReader.h"

namespace score {
namespace time {

std::unique_ptr<ITimeBaseReader> TimeBaseReaderFactory::Create(std::string_view name) {
    return std::make_unique<SharedMemTimeBaseReader>(name);
}

std::unique_ptr<ITimeBaseReader> TimeBaseReaderFactory::Create(std::string_view name, std::size_t max_size) {
    return std::make_unique<SharedMemTimeBaseReader>(name, max_size);
}

}  // namespace time
}  // namespace score
