/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_TIMEBASEWRITERFACTORYMOCK_H_
#define SCORE_TIME_TIMEBASEWRITERFACTORYMOCK_H_

#include <string_view>
#include <gmock/gmock.h>

#include <memory>

#include "SharedMemTimeBaseWriterMock.h"

namespace score {
namespace time {

class TimeBaseWriterFactoryMock {
public:
    TimeBaseWriterFactoryMock() {
        ON_CALL(*this, Create).WillByDefault([](std::string_view name, std::size_t max_size, bool is_owner) {
            return std::make_unique<::testing::NiceMock<SharedMemTimeBaseWriterMock>>(name, max_size, is_owner);
        });
    }

    ~TimeBaseWriterFactoryMock() = default;

    MOCK_METHOD3(Create, std::unique_ptr<ITimeBaseWriter>(std::string_view, std::size_t, bool));
};

extern std::unique_ptr<::testing::NiceMock<TimeBaseWriterFactoryMock>> writer_factory_mock;
extern bool writer_factory_mock_return_real_writer;

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_TIMEBASEWRITERFACTORYMOCK_H_
