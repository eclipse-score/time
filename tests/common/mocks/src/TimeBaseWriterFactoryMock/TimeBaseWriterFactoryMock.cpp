/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "TimeBaseWriterFactoryMock.h"

#include "SharedMemTimeBaseWriterMock.h"
#include "score/time/utility/SharedMemTimeBaseWriter.h"
#include "score/time/utility/TimeBaseWriterFactory.h"

namespace score {
namespace time {

extern std::unique_ptr<::testing::NiceMock<TimeBaseWriterFactoryMock>> writer_factory_mock;
bool writer_factory_mock_return_real_writer = false;

std::unique_ptr<ITimeBaseWriter> TimeBaseWriterFactory::Create(std::string_view name, std::size_t max_size,
                                                               bool is_owner) {
    if (writer_factory_mock_return_real_writer) {
        return std::make_unique<SharedMemTimeBaseWriter>(name, max_size, is_owner);
    } else {
        return writer_factory_mock->Create(name, max_size, is_owner);
    }
}

}  // namespace time
}  // namespace score
