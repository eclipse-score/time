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
#include "score/time_daemon/src/ipc/receiver_mock.h"
#include "score/time_daemon/src/ipc/svt/receiver/svt_receiver.h"
#include "score/time_daemon/src/ipc/svt/svt_time_info.h"

namespace score
{
namespace td
{

std::shared_ptr<SvtReceiver> CreateSvtReceiver()
{
    static auto receiver = std::make_shared<ReceiverMock<svt::TimeBaseSnapshot>>();
    return receiver;
}

}  // namespace td
}  // namespace score
