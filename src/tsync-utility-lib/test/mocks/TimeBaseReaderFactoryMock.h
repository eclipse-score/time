/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_TIMEBASEREADERFACTORYMOCK_H_
#define SCORE_TIME_TIMEBASEREADERFACTORYMOCK_H_

#include <string_view>
#include <gmock/gmock.h>

#include <memory>

#include "SharedMemTimeBaseReaderMock.h"

namespace score {
namespace time {

class TimeBaseReaderFactoryMock {
public:
    TimeBaseReaderFactoryMock() {
        ON_CALL(*this, Create).WillByDefault([](std::string_view name, std::size_t max_size) {
            return std::make_unique<::testing::NiceMock<SharedMemTimeBaseReaderMock>>(name, max_size);
        });
    }

    ~TimeBaseReaderFactoryMock() = default;

    MOCK_METHOD2(Create, std::unique_ptr<ITimeBaseReader>(std::string_view, std::size_t));
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_TIMEBASEREADERFACTORYMOCK_H_
