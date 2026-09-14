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
#include "score/time_daemon/src/ipc/core/receiver_impl.h"
#include "score/time_daemon/src/ipc/svt/config.h"
#include "score/time_daemon/src/ipc/svt/receiver/svt_receiver.h"
#include "score/time_daemon/src/ipc/svt/svt_time_info.h"
#include <memory>

namespace score::td
{

// Declared in factory.h; this is one of two alternate definitions (see factory_stub.cpp)
// selected via Bazel target, so it must stay externally linked.
// NOLINTNEXTLINE(misc-use-internal-linkage)
auto CreateSvtReceiver() -> std::shared_ptr<SvtReceiver>
{
    return std::make_shared<ReceiverImpl<svt::TimeBaseSnapshot>>(kSvtShmemPath);
}

}  // namespace score::td
