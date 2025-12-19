/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef TSYNC_DATA_BROKER_H_INCLUDED
#define TSYNC_DATA_BROKER_H_INCLUDED

#include <pthread.h>
#include <semaphore.h>
// coverity[autosar_cpp14_a1_1_1_violation] This library provides a C API, the use of C headers is acceptable.
#include <stddef.h>
// coverity[autosar_cpp14_a1_1_1_violation] This library provides a C API, the use of C headers is acceptable.
#include <stdint.h>

#include <atomic>

#include "score/time/utility/TimeBaseReaderFactory.h"
#include "score/time/utility/TimeBaseWriterFactory.h"
#include "tsync_ptp_lib_types.h"

#define TSYNC_DB_RESULT_OK 0
#define TSYNC_DB_RESULT_ERR_ACCESS_ID_MAPPINGS -1
#define TSYNC_DB_RESULT_ERR_ACCESS_TIMEBASE -2
#define TSYNC_DB_RESULT_ERR_ID_MAPPING_NOT_FOUND -3
#define TSYNC_DB_RESULT_ERR_ID_MAPPING_BUFFER_TOO_SMALL -4

/* maximum number of time bases allowed */
#define TSYNC_DB_MAX_TIMEBASES 16u

/* size of the prefix for the semaphore name /p_ or /c_ */
#define TSYNC_DB_SEMAPHORE_PREFIX_SIZE 3u

/* size of the semaphore name */
#define TSYNC_DB_SEMAPHORE_NAME_MAX_SIZE TSYNC_INSTANCE_SPECIFIER_MAX_SIZE + TSYNC_DB_SEMAPHORE_PREFIX_SIZE

typedef int32_t TSync_DB_Result;

typedef enum {
    TSYNC_DB_TIMEBASE_ROLE_NONE,
    TSYNC_DB_TIMEBASE_ROLE_CONSUMER,
    TSYNC_DB_TIMEBASE_ROLE_PROVIDER
} TSync_DB_Timebase_Role;

typedef struct {
    TSync_SynchronizedTimeBaseType timeBaseId;
    TSync_DB_Timebase_Role timeBaseRole;
} TSync_DB_Timebase_Mapping_Data;

typedef enum {
    TSYNC_DB_THREAD_SIGNAL_NONE,
    TSYNC_DB_THREAD_SIGNAL_EXIT,
    TSYNC_DB_THREAD_SIGNAL_CONTINUE
} TSync_DB_Thread_Signal;

typedef enum { TSYNC_DB_THREAD_STATUS_DEAD, TSYNC_DB_THREAD_STATUS_ALIVE } TSync_DB_Thread_Status;
typedef struct {
    pthread_t threadHandle;
    std::atomic<TSync_DB_Thread_Signal> threadSignal;
    std::atomic<TSync_DB_Thread_Status> threadStatus;
    TSync_TransmitGlobalTimeCallback callback;
} TSync_DB_Transmit_Global_Time_Data;

// clang-format off
// RULECHECKER_comment(1, 8, check_incomplete_data_member_construction, "Pimpl pattern does not allow construction of all members", true)
typedef struct {
    // clang-format on
    TSync_DB_Timebase_Mapping_Data mappingData;
    score::time::TimeBaseReaderFactory::PointerType reader;
    score::time::TimeBaseWriterFactory::PointerType writer;
    TSync_DB_Transmit_Global_Time_Data transmitGlobalTime;
    std::atomic<int32_t> refCounter;
    std::atomic<bool> isInitialized;
} TSync_DB_Timebase_Meta_Data;

/* Accesses the shared memory region holding the timeBaseIdentfier - InstanceSpecifier mappings */
void tsync_db_Open();
/* Rolls back tsync_db_Open() actions */
void tsync_db_Close();
/* Opens the shared memory region associated with the given timeBase identifier
and sets up the returned data structure */
TSync_DB_Timebase_Meta_Data* tsync_db_OpenTimebase(TSync_SynchronizedTimeBaseType timeBaseId);
/* Closes the shared memory region associated with the given timeBase handle */
void tsync_db_CloseTimebase(TSync_DB_Timebase_Meta_Data* timeBaseHandle);
/* required for UTs */
void tsync_db_lib_unlock();

#endif  // TSYNC_DATA_BROKER_H_INCLUDED
