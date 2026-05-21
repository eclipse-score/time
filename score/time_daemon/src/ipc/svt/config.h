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
#ifndef SCORE_TIMEDAEMON_CODE_IPC_SVT_SHMEM_PATH_H
#define SCORE_TIMEDAEMON_CODE_IPC_SVT_SHMEM_PATH_H

#include <string>

namespace score
{
namespace td
{

///
/// \brief Common path for svt publisher and receiver
///
const std::string kSvtShmemPath{"/svt_shmem_path"};

}  // namespace td
}  // namespace score

#endif  // #ifndef SCORE_TIMEDAEMON_CODE_IPC_SVT_SHMEM_PATH_H
