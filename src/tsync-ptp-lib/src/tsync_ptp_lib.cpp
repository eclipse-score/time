/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "tsync_ptp_lib.h"

#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <ctime>
#include <mutex>

#include "score/time/common/Abort.h"

#include "score/time/utility/ITimeBaseReader.h"
#include "score/time/utility/ITimeBaseWriter.h"
#include "score/time/utility/SysCalls.h"
#include "score/time/utility/TsyncSharedUtils.h"

#include "tsync_data_broker.h"

using score::time::AlignedSkip;
using score::time::ITimeBaseReader;
using score::time::ITimeBaseWriter;
using score::time::SynchronizationStatus;
using score::time::TsyncSharedUtils;
using score::time::UserData;
using score::time::UserDataView;

using score::time::OsThreadCreate;
using score::time::OsThreadJoin;
using score::time::OsUmask;

using score::time::common::logFatalAndAbort;

TSync_TimeBaseStatusType GetTimeStampStatusFromSyncStatus(SynchronizationStatus input) {
    TSync_TimeBaseStatusType res;
    if (input == SynchronizationStatus::kSynchronized) {
        res = 1U << TIMEBASE_STATUS_BIT_GLOBAL_SYNC;
    } else if (input == SynchronizationStatus::kSynchToGateway) {
        res = 1U << TIMEBASE_STATUS_BIT_SYNC_TO_GATEWAY;
    } else if (input == SynchronizationStatus::kTimeOut) {
        res = 1U << TIMEBASE_STATUS_BIT_TIMEOUT;
    } else {
        res = 0U;
    }
    return res;
}

TSync_ReturnType TSync_Open(void) {
    tsync_db_Open();

    return TSYNC_DB_RESULT_OK;
}

void TSync_Close(void) {
    tsync_db_Close();
}

TSync_TimeBaseHandleType TSync_OpenTimebase(TSync_SynchronizedTimeBaseType timeBaseId) {
    return static_cast<TSync_TimeBaseHandleType>(tsync_db_OpenTimebase(timeBaseId));
}

void TSync_CloseTimebase(TSync_TimeBaseHandleType timeBaseHandle) {
    if (TSync_RegisterTransmitGlobalTimeCallback(timeBaseHandle, nullptr) == E_OK) {
        tsync_db_CloseTimebase(static_cast<TSync_DB_Timebase_Meta_Data*>(timeBaseHandle));
    }
}

// clang-format off
// RULECHECKER_comment(1, 1, check_max_parameters, "The function signature reflects a pre-existing AUTOSAR Classic API.", true)
TSync_ReturnType TSync_BusSetGlobalTime(TSync_TimeBaseHandleType timeBaseHandle,
                                        const TSync_TimeStampType* globalTimePtr, const TSync_UserDataType* userDataPtr,
                                        const TSync_MeasurementType* measureDataPtr,
                                        const TSync_VirtualLocalTimeType* localTimePtr) {
    // clang-format on
    (void)measureDataPtr;
    bool res = false;

    if (timeBaseHandle != TSYNC_INVALID_HANDLE) {
        TSync_DB_Timebase_Meta_Data* timeBaseMetaData = static_cast<TSync_DB_Timebase_Meta_Data*>(timeBaseHandle);
        if (!timeBaseMetaData->isInitialized) {
            return E_NOT_OK;
        }
        score::time::SynchronizationStatus ts_mem;
        {
            // read shared memory to obtain current status
            timeBaseMetaData->reader->GetAccessor().Open();
            std::lock_guard<ITimeBaseReader> r_lock(*(timeBaseMetaData->reader));
            res = timeBaseMetaData->reader->Read(ts_mem);
        }

        if (!res) {
            return E_NOT_OK;
        }

        timeBaseMetaData->writer->GetAccessor().Open();
        std::lock_guard<ITimeBaseWriter> w_lock(*(timeBaseMetaData->writer));

        if (globalTimePtr) {
            TSync_TimeStampType ts = *globalTimePtr;
            score::time::TimestampWithStatus ts_w_status;
            if (ts_mem == SynchronizationStatus::kSynchToGateway) {
                ts_w_status.status = ts_mem;
            } else {
                ts_w_status.status = SynchronizationStatus::kSynchronized;
            }

            ts_w_status.seconds = std::chrono::seconds((static_cast<std::chrono::seconds::rep>(ts.secondsHi) << 32) |
                                                       static_cast<std::chrono::seconds::rep>(ts.seconds));
            ts_w_status.nanoseconds =
                std::chrono::nanoseconds(static_cast<std::chrono::nanoseconds::rep>(ts.nanoseconds));

            res = timeBaseMetaData->writer->Write(ts_w_status);
        } else {
            res = AlignedSkip<ITimeBaseWriter, score::time::TimestampWithStatus>(*(timeBaseMetaData->writer));
        }

        if (!res) {
            return E_NOT_OK;
        }

        if (localTimePtr) {
            std::chrono::nanoseconds vlt = std::chrono::nanoseconds(
                (static_cast<std::chrono::nanoseconds::rep>(localTimePtr->nanosecondsHi) << 32) |
                static_cast<std::chrono::nanoseconds::rep>(localTimePtr->nanosecondsLo));

            res = timeBaseMetaData->writer->Write(vlt);
        } else {
            res = AlignedSkip<ITimeBaseWriter, std::chrono::nanoseconds>(*(timeBaseMetaData->writer));
        }

        if (!res) {
            return E_NOT_OK;
        }

        if (userDataPtr) {
            UserData ud;
            ud[0U] = std::byte{userDataPtr->userDataLength};
            ud[1U] = std::byte{userDataPtr->userByte0};
            ud[2U] = std::byte{userDataPtr->userByte1};
            ud[3U] = std::byte{userDataPtr->userByte2};

            res = timeBaseMetaData->writer->Write(ud);
        }
    }

    return res ? E_OK : E_NOT_OK;
}

// clang-format off
// RULECHECKER_comment(1, 1, check_max_parameters, "The function signature reflects a pre-existing AUTOSAR Classic API.", true)
TSync_ReturnType TSync_BusGetGlobalTime(TSync_TimeBaseHandleType timeBaseHandle, TSync_TimeStampType* globalTimePtr,
                                        TSync_UserDataType* userDataPtr, TSync_MeasurementType* measureDataPtr,
                                        TSync_VirtualLocalTimeType* localTimePtr) {
    // clang-format on
    (void)measureDataPtr;
    bool res = false;

    if (timeBaseHandle == TSYNC_INVALID_HANDLE) {
        return E_NOT_OK;
    }

    TSync_DB_Timebase_Meta_Data* timeBaseMetaData = static_cast<TSync_DB_Timebase_Meta_Data*>(timeBaseHandle);
    if (!timeBaseMetaData->isInitialized) {
        return E_NOT_OK;
    }
    timeBaseMetaData->reader->GetAccessor().Open();
    std::lock_guard<ITimeBaseReader> lock(*timeBaseMetaData->reader);

    if (globalTimePtr) {
        score::time::TimestampWithStatus ts_w_status;
        res = timeBaseMetaData->reader->Read(ts_w_status);
        if (res) {
            globalTimePtr->seconds = static_cast<uint32_t>(ts_w_status.seconds.count() & 0xffffffff);
            globalTimePtr->secondsHi = static_cast<uint16_t>((ts_w_status.seconds.count() >> 32) & 0xffff);
            globalTimePtr->nanoseconds = static_cast<uint32_t>(ts_w_status.nanoseconds.count() & 0xffffffff);
            globalTimePtr->timeBaseStatus = GetTimeStampStatusFromSyncStatus(ts_w_status.status);
        }
    } else {
        res = AlignedSkip<ITimeBaseReader, score::time::TimestampWithStatus>(*(timeBaseMetaData->reader));
    }

    if (!res) {
        return E_NOT_OK;
    }

    if (localTimePtr) {
        std::chrono::nanoseconds vlt;
        res = timeBaseMetaData->reader->Read(vlt);
        if (res) {
            localTimePtr->nanosecondsHi = static_cast<uint32_t>((vlt.count() & 0xffffffff00000000) >> 32);
            localTimePtr->nanosecondsLo = static_cast<uint32_t>(vlt.count() & 0xffffffff);
        }
    } else {
        res = AlignedSkip<ITimeBaseReader, std::chrono::nanoseconds>(*(timeBaseMetaData->reader));
    }

    if (!res) {
        return E_NOT_OK;
    }

    if (userDataPtr) {
        UserDataView udv{};
        res = timeBaseMetaData->reader->Read(udv);
        if (!res) {
            return E_NOT_OK;
        }
        userDataPtr->userDataLength = static_cast<uint8_t>(udv.size());
        if (userDataPtr->userDataLength > 0) {
            // todo: Replace index access via udv.data() by index op on udv direcly once score::span supports it.
            userDataPtr->userByte0 = static_cast<uint8_t>(udv.data()[0U]);
            if (userDataPtr->userDataLength > 1) {
                userDataPtr->userByte1 = static_cast<uint8_t>(udv.data()[1U]);
                if (userDataPtr->userDataLength > 2) {
                    userDataPtr->userByte2 = static_cast<uint8_t>(udv.data()[2U]);
                }
            }
        }
    }

    return E_OK;
}

TSync_ReturnType TSync_GetCurrentVirtualLocalTime(TSync_TimeBaseHandleType timeBaseHandle,
                                                  TSync_VirtualLocalTimeType* localTimePtr) {
    if ((timeBaseHandle != TSYNC_INVALID_HANDLE) && (localTimePtr != nullptr)) {
        TSync_DB_Timebase_Meta_Data* timeBaseMetaData = static_cast<TSync_DB_Timebase_Meta_Data*>(timeBaseHandle);
        if (timeBaseMetaData->isInitialized) {
            auto ns = score::time::TsyncSharedUtils::GetCurrentVirtualLocalTime();

            if (ns) {
                localTimePtr->nanosecondsLo = static_cast<uint32_t>(ns->count() & 0xffffffff);
                localTimePtr->nanosecondsHi = static_cast<uint32_t>((ns->count() & 0xffffffff00000000) >> 32);
                return E_OK;
            }
        }
    }

    return E_NOT_OK;
}

// coverity[autosar_cpp14_a8_4_10_violation] This function is used in an OS-provided function.
void* TSync_TransmitGlobalTimeRunner(void* arg) {
    if (arg == nullptr) {
        logFatalAndAbort("TSync_TransmitGlobalTimeRunner called with nullptr argument");
    }
    TSync_DB_Timebase_Meta_Data* timeBaseMetaData = static_cast<TSync_DB_Timebase_Meta_Data*>(arg);
    if (!timeBaseMetaData->isInitialized) {
        return nullptr;
    }

    mode_t temp_umask = OsUmask(0111U);
    auto sem = TsyncSharedUtils::CreateTransmissionSemaphore(timeBaseMetaData->mappingData.timeBaseId, false);
    (void)OsUmask(temp_umask);

    while (timeBaseMetaData->transmitGlobalTime.threadSignal.load() != TSYNC_DB_THREAD_SIGNAL_EXIT &&
           timeBaseMetaData->isInitialized) {
        sem.lock();

        TSync_TransmitGlobalTimeCallback transmitGlobalTimeCallback = timeBaseMetaData->transmitGlobalTime.callback;
        if (transmitGlobalTimeCallback != nullptr &&
            timeBaseMetaData->transmitGlobalTime.threadSignal.load() != TSYNC_DB_THREAD_SIGNAL_EXIT) {
            transmitGlobalTimeCallback(timeBaseMetaData->mappingData.timeBaseId);
        }
    }

    timeBaseMetaData->transmitGlobalTime.threadStatus.store(TSYNC_DB_THREAD_STATUS_DEAD);
    return nullptr;
}

void TSync_HandleCallBackThreadCleanup(TSync_TimeBaseHandleType timeBaseHandle) {
    if (timeBaseHandle != TSYNC_INVALID_HANDLE) {
        TSync_DB_Timebase_Meta_Data* timeBaseMetaData = static_cast<TSync_DB_Timebase_Meta_Data*>(timeBaseHandle);

        if (timeBaseMetaData->transmitGlobalTime.threadStatus.load() == TSYNC_DB_THREAD_STATUS_ALIVE) {
            timeBaseMetaData->transmitGlobalTime.threadSignal.store(TSYNC_DB_THREAD_SIGNAL_EXIT);

            // wake the thread
            auto sem = TsyncSharedUtils::CreateTransmissionSemaphore(timeBaseMetaData->mappingData.timeBaseId, false);
            sem.unlock();

            auto res = OsThreadJoin(timeBaseMetaData->transmitGlobalTime.threadHandle, nullptr);
            if (res != 0) {
                std::perror("TSync_HandleCallBackThreadCleanup - pthread_join failed");
                logFatalAndAbort("TSync_HandleCallBackThreadCleanup - pthread_join failed");
            }

            timeBaseMetaData->transmitGlobalTime.callback = nullptr;
            timeBaseMetaData->transmitGlobalTime.threadSignal.store(TSYNC_DB_THREAD_SIGNAL_NONE);
        }
    }
}

TSync_ReturnType TSync_RegisterTransmitGlobalTimeCallback(TSync_TimeBaseHandleType timeBaseHandle,
                                                          TSync_TransmitGlobalTimeCallback cb) {
    if (timeBaseHandle != TSYNC_INVALID_HANDLE) {
        TSync_DB_Timebase_Meta_Data* timeBaseMetaData = static_cast<TSync_DB_Timebase_Meta_Data*>(timeBaseHandle);
        if (!timeBaseMetaData->isInitialized) {
            return E_NOT_OK;
        }

        TSync_TransmitGlobalTimeCallback old_callback = timeBaseMetaData->transmitGlobalTime.callback;

        if (cb == old_callback) {
            return E_OK;
        }

        if (cb == nullptr) {
            TSync_HandleCallBackThreadCleanup(timeBaseHandle);
        }

        if (cb != nullptr && old_callback == nullptr) {  // register callback once
            timeBaseMetaData->transmitGlobalTime.callback = cb;
            timeBaseMetaData->transmitGlobalTime.threadStatus.store(TSYNC_DB_THREAD_STATUS_ALIVE);
            int res = OsThreadCreate(&timeBaseMetaData->transmitGlobalTime.threadHandle, nullptr,
                                     &TSync_TransmitGlobalTimeRunner, timeBaseHandle);
            if (res != 0) {
                std::perror("TSync_RegisterTransmitGlobalTimeCallback - pthread_create failed");
                logFatalAndAbort("TSync_RegisterTransmitGlobalTimeCallback - pthread_create failed");
            }
        }

        return E_OK;
    }

    return E_NOT_OK;
}

TSync_ReturnType TSync_SetTimeoutStatus(TSync_TimeBaseHandleType timeBaseHandle) {
    if (timeBaseHandle != TSYNC_INVALID_HANDLE) {
        TSync_DB_Timebase_Meta_Data* timeBaseMetaData = static_cast<TSync_DB_Timebase_Meta_Data*>(timeBaseHandle);
        if (!timeBaseMetaData->isInitialized) {
            return E_NOT_OK;
        }
        // Timeout bit must be set only when the timebase was synchronized atleast once
        {
            score::time::TimestampWithStatus ts;
            timeBaseMetaData->reader->GetAccessor().Open();
            timeBaseMetaData->reader->lock();
            if (timeBaseMetaData->reader->Read(ts)) {
                timeBaseMetaData->reader->unlock();
                if (ts.status == score::time::SynchronizationStatus::kSynchronized) {
                    ts.status = score::time::SynchronizationStatus::kTimeOut;
                    timeBaseMetaData->writer->GetAccessor().Open();
                    std::lock_guard<ITimeBaseWriter> lock(*(timeBaseMetaData->writer));
                    timeBaseMetaData->writer->Write(ts);
                }
            } else {
                timeBaseMetaData->reader->unlock();
                logFatalAndAbort("TSync_SetTimeoutStatus() - could not read timestamp for timebase");
            }
        }
        return E_OK;
    }
    return E_NOT_OK;
}
