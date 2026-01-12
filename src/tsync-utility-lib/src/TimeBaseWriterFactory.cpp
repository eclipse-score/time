/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "score/time/utility/TimeBaseWriterFactory.h"

#include "SharedMemTimeBaseWriter.h"

namespace score {
namespace time {

std::unique_ptr<ITimeBaseWriter> TimeBaseWriterFactory::Create(std::string_view name, bool is_owner) {
    return std::make_unique<SharedMemTimeBaseWriter>(name, is_owner);
}

std::unique_ptr<ITimeBaseWriter> TimeBaseWriterFactory::Create(std::string_view name, std::size_t max_size, bool is_owner) {
    return std::make_unique<SharedMemTimeBaseWriter>(name, max_size, is_owner);
}

}  // namespace time
}  // namespace score
