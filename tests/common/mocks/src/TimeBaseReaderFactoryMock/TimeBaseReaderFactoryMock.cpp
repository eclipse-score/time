/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "TimeBaseReaderFactoryMock.h"

#include "SharedMemTimeBaseReaderMock.h"
#include "score/time/utility/SharedMemTimeBaseReader.h"
#include "score/time/utility/TimeBaseReaderFactory.h"

namespace score {
namespace time {

extern std::unique_ptr<::testing::NiceMock<TimeBaseReaderFactoryMock>> reader_factory_mock;
bool reader_factory_mock_return_real_reader = false;

std::unique_ptr<ITimeBaseReader> TimeBaseReaderFactory::Create(std::string_view name, std::size_t max_size) {
    if (reader_factory_mock_return_real_reader) {
        return std::make_unique<SharedMemTimeBaseReader>(name, max_size);
    } else {
        return reader_factory_mock->Create(name, max_size);
    }
}

}  // namespace time
}  // namespace score
