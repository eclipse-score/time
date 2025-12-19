/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "tsync_data_broker.h"

#include <fcntl.h>
#include <sys/stat.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>

#include "score/time/common/Abort.h"
#include "score/time/common/ExcludeCoverageAdapter.h"

#include "score/time/utility/TsyncIdMappingsHandler.h"

using score::time::ITimeBaseReader;
using score::time::ITimeBaseWriter;
using score::time::TimeBaseReaderFactory;
using score::time::TimeBaseWriterFactory;
using score::time::TsyncIdMappingsHandler;
using score::time::TsyncTimeDomainConfig;
using score::time::common::logFatalAndAbort;

static std::unique_ptr<TsyncIdMappingsHandler> mappings_handler;

/* contains the meta data of a timebase */
TSync_DB_Timebase_Meta_Data tsync_db_timebase_meta_data[TSYNC_DB_MAX_TIMEBASES] = {};
/* this flag is atomic for immidiate visibility of state changes in multithreaded contexts */
/* Note: The current ptpd implementation is single threaded, so this not really required */
static std::atomic<bool> is_initialized(false);

// forward declarations
void tsync_db_OpenIdMappings(void);
void tsync_db_PrepareTimeBaseMetaData(void);
TSync_DB_Timebase_Meta_Data* tsync_db_GetTimeBaseMetaDataElement(TSync_SynchronizedTimeBaseType timeBaseId);
void tsync_db_InitTimeBaseMetaData(TSync_DB_Timebase_Meta_Data& meta_data);

void tsync_db_Open() {
    bool init_state_expected = false;
    bool init_state_desired = true;
    bool state_changed = is_initialized.compare_exchange_strong(init_state_expected, init_state_desired);
    if (state_changed) {
        tsync_db_OpenIdMappings();
        tsync_db_PrepareTimeBaseMetaData();
    }
}

void tsync_db_Close() {
    bool init_state_expected = true;
    bool init_state_desired = false;
    bool state_changed = is_initialized.compare_exchange_strong(init_state_expected, init_state_desired);
    if (state_changed) {
        mappings_handler.reset();
        for (uint16_t i = 0u; i < TSYNC_DB_MAX_TIMEBASES; ++i) {
            tsync_db_InitTimeBaseMetaData(tsync_db_timebase_meta_data[i]);
        }
    }
}

// TODO: Make sure that only one Meta_Data element exist for the provider use case.
// TODO: Print error is a provider ID is opened more then once in a process
// TODO: For the consumer use case, multiple openTimebase calls will return a
// distinct meta_pointers for the same timeBaseId. Increase the size of tsync_db_timebase_meta_data
// add a is used flag in TSync_DB_Timebase_Meta_Data and initialize all elements with flag set to unused.
// Unique semaphore names for signalling
// are needed, the address of the meta data element can be used in the name (with an offset).
TSync_DB_Timebase_Meta_Data* tsync_db_OpenTimebase(TSync_SynchronizedTimeBaseType timeBaseId) {
    // TODO: Whether a time base has the role of a consumer or a provider will be encoded in the id mappings in the
    // future.

    if (!is_initialized) {
        return nullptr;
    }

    auto timebase_meta_data = tsync_db_GetTimeBaseMetaDataElement(timeBaseId);
    if (timebase_meta_data == nullptr) {
        std::cerr << "tsync_db_OpenTimebase - timeBaseMetaDataElement == nullptr." << std::endl;
        return nullptr;
    }

    int32_t ref_count = timebase_meta_data->refCounter++;
    if (ref_count > 0) {
        return timebase_meta_data;
    } else {
        // in case another thread is still busy with closing this timebase
        // clang-format off
        while (timebase_meta_data->isInitialized);
        // clang-format on
        // We only need to do this once
        auto domain_name = mappings_handler->GetDomainName(timeBaseId);
        if (!domain_name) {
            std::cerr << "tsync_db_OpenTimebase - could not determine domain name for timebase " << timeBaseId
                      << std::endl;
            return nullptr;
        }

        bool res = false;
        auto reader = TimeBaseReaderFactory::Create(*domain_name);
        if (!reader) {
            std::cerr << "tsync_db_OpenTimebase - could not open reader for time domain " << *domain_name << std::endl;
            logFatalAndAbort("tsync_db_OpenTimebase - could not open reader for time domain");
        }

        TsyncTimeDomainConfig cfg;
        reader->GetAccessor().Open();
        {
            std::lock_guard<ITimeBaseReader> lock(*reader);
            res = reader->Read(cfg);
        }

        if (!res) {
            reader.reset();
            std::cerr << "tsync_db_OpenTimebase - could not read config of time domain " << *domain_name << std::endl;
            logFatalAndAbort("tsync_db_OpenTimebase - could not read config of time domain");
        }

        if (!cfg.provider_config.time_master_config.is_valid && !cfg.consumer_config.time_slave_config.is_valid) {
            reader.reset();
            std::cerr << "tsync_db_OpenTimebase - time domain " << *domain_name
                      << " has neither a valid provider nor consumer configuration" << std::endl;
            logFatalAndAbort(
                "tsync_db_OpenTimebase - time domain has neither a valid provider nor consumer configuration");
        }
        auto writer = TimeBaseWriterFactory::Create(*domain_name);

        timebase_meta_data->reader = std::move(reader);
        timebase_meta_data->writer = std::move(writer);

        timebase_meta_data->isInitialized = true;
        return timebase_meta_data;
    }
}

void tsync_db_CloseTimebase(TSync_DB_Timebase_Meta_Data* timeBaseMetaData) {
    if (is_initialized && timeBaseMetaData && mappings_handler) {
        int32_t ref_counter = --timeBaseMetaData->refCounter;
        // No more reference count to timebase meta data proceed to close time base
        if (ref_counter == 0) {
            auto domain_name = mappings_handler->GetDomainName(timeBaseMetaData->mappingData.timeBaseId);

            if (!domain_name) {
                logFatalAndAbort("tsync_db_CloseTimebase - could not determine domain name for timebase.");
            }

            timeBaseMetaData->reader.reset();
            timeBaseMetaData->writer.reset();

            // check transmitGlobalTime state
            if (timeBaseMetaData->transmitGlobalTime.threadStatus != TSYNC_DB_THREAD_STATUS_DEAD) {
                logFatalAndAbort(
                    "tsync_db_CloseTimebase - transmitGlobalTime.threadStatus != TSYNC_DB_THREAD_STATUS_DEAD.");
            }
            if (timeBaseMetaData->transmitGlobalTime.threadSignal != TSYNC_DB_THREAD_SIGNAL_NONE) {
                logFatalAndAbort(
                    "tsync_db_CloseTimebase - failed transmitGlobalTime.threadSignal != TSYNC_DB_THREAD_SIGNAL_NONE.");
            }
            if (timeBaseMetaData->transmitGlobalTime.callback != nullptr) {
                logFatalAndAbort("tsync_db_CloseTimebase -  callback != nullptr");
            }
            timeBaseMetaData->isInitialized = false;
        } else {
            if (ref_counter < 0) {
                logFatalAndAbort("tsync_db_CloseTimebase -  refCounter is negative");
            }
            // There still reference count to timebase meta data no need for more action
            return;
        }
    }
}

void tsync_db_OpenIdMappings(void) {
    mappings_handler = std::make_unique<TsyncIdMappingsHandler>();
    mappings_handler->DoSharedMemoryRead();
}

void tsync_db_PrepareTimeBaseMetaData() {
    uint32_t i = 0u;
    for (auto it : *mappings_handler) {
        tsync_db_InitTimeBaseMetaData(tsync_db_timebase_meta_data[i]);
        tsync_db_timebase_meta_data[i].mappingData.timeBaseId =
            static_cast<TSync_SynchronizedTimeBaseType>(it.first & 0x0000ffff);
        tsync_db_timebase_meta_data[i].mappingData.timeBaseRole =
            TSYNC_DB_TIMEBASE_ROLE_PROVIDER;  // TODO: ml->mappings[i].timeBaseRole;
        ++i;
        if (i == TSYNC_DB_MAX_TIMEBASES) {
            break;
        }
    }
}

void tsync_db_InitTimeBaseMetaData(TSync_DB_Timebase_Meta_Data& meta_data) {
    meta_data.reader.reset();
    meta_data.writer.reset();
    meta_data.mappingData.timeBaseId = TSYNC_INVALID_TIME_BASE_ID;
    meta_data.mappingData.timeBaseRole = TSYNC_DB_TIMEBASE_ROLE_NONE;
    meta_data.transmitGlobalTime.callback = nullptr;
    meta_data.transmitGlobalTime.threadSignal = TSYNC_DB_THREAD_SIGNAL_NONE;
    meta_data.transmitGlobalTime.threadStatus = TSYNC_DB_THREAD_STATUS_DEAD;
    meta_data.refCounter = 0;
    meta_data.isInitialized = false;
}

TSync_DB_Timebase_Meta_Data* tsync_db_GetTimeBaseMetaDataElement(TSync_SynchronizedTimeBaseType timeBaseId) {
    TSync_DB_Timebase_Meta_Data* timeBaseMetaDataElement = nullptr;

    for (uint16_t i = 0u; i < TSYNC_DB_MAX_TIMEBASES; ++i) {
        if (tsync_db_timebase_meta_data[i].mappingData.timeBaseId == timeBaseId) {
            timeBaseMetaDataElement = &tsync_db_timebase_meta_data[i];
            break;
        }
    }

    return timeBaseMetaDataElement;
}
