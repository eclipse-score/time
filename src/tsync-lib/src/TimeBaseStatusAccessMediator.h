/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_TIMEBASESTATUSACCESMEDIATOR_H_
#define SCORE_TIME_TIMEBASESTATUSACCESMEDIATOR_H_

#include "score/time/synchronized_time_base_status.h"
#include "score/time/utility/TsyncSharedUtils.h"

namespace score {
namespace time {

class TimeBaseStatusAccessMediator {
public:
    static SynchronizedTimeBaseStatus CreateSynchronizedTimeBaseStatusInstance() noexcept {
        return SynchronizedTimeBaseStatus();
    }

    static void SetSynchronizedTimeBaseStatusCreationTime(SynchronizedTimeBaseStatus& status,
                                                          const Timestamp& creationTime) noexcept {
        status.creation_time_ = creationTime;
    }

    static void SetSynchronizedTimeBaseStatusUserData(SynchronizedTimeBaseStatus& status,
                                                      UserDataView userData) noexcept {
        status.user_data_ = userData;
    }

    static void SetSynchronizedTimeBaseStatusLeapJump(SynchronizedTimeBaseStatus& status, LeapJump leapJump) noexcept {
        status.leap_jump_ = leapJump;
    }

    static void SetSynchronizedTimeBaseStatusSynchronizationStatus(SynchronizedTimeBaseStatus& status,
                                                                   SynchronizationStatus syncStatus) noexcept {
        status.sync_status_ = syncStatus;
    }

    TimeBaseStatusAccessMediator() = delete;
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_TIMEBASESTATUSACCESMEDIATOR_H_
