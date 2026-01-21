/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "TimeBaseReaderFactoryMock.h"

#include "score/time/utility/TimeBaseReaderFactory.h"
#include "SharedMemTimeBaseReaderMock.h"
#include "SharedMemTimeBaseReader.h"

namespace score {
namespace time {

constexpr size_t kTimeBaseShmemFileSize{4096u};
std::unique_ptr<::testing::NiceMock<TimeBaseReaderFactoryMock>> reader_factory_mock;
bool reader_factory_mock_return_real_reader = false;

std::unique_ptr<ITimeBaseReader> TimeBaseReaderFactory::Create(std::string_view name) {
    return TimeBaseReaderFactory::Create(name, kTimeBaseShmemFileSize);
}

std::unique_ptr<ITimeBaseReader> TimeBaseReaderFactory::Create(std::string_view name, std::size_t max_size) {
    if (reader_factory_mock_return_real_reader) {
        return std::make_unique<SharedMemTimeBaseReader>(name, max_size);
    } else {
        return reader_factory_mock->Create(name, max_size);
    }
}

}  // namespace time
}  // namespace score
