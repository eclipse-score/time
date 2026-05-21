/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/
#ifndef SCORE_TIMEDAEMON_CODE_IPC_SVT_SVT_PUBLISHER_H
#define SCORE_TIMEDAEMON_CODE_IPC_SVT_SVT_PUBLISHER_H

#include "score/time_daemon/src/common/data_types/ptp_time_info.h"
#include "score/time_daemon/src/ipc/core/publisher_impl.h"
#include "score/time_daemon/src/ipc/svt/svt_time_info.h"

namespace score
{
namespace td
{

/**
 * @brief Publisher implementation alias
 */
using SvtPublisher = PublisherImpl<PtpTimeInfo, svt::TimeBaseSnapshot>;

}  // namespace td
}  // namespace score

#endif  // SCORE_TIMEDAEMON_CODE_IPC_SVT_SVT_PUBLISHER_H
