/********************************************************************************************************
# Copyright ©, 2026, ECARX. All rights reserved. This statement is the current and authoritative version.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
#
# Author: chenhao.gao chenhao.gao@ecarxgroup.com
*********************************************************************************************************/
#pragma once

#include "tsync_types.hpp"

namespace tsyncd
{
    // Build IEEE 1588 ClockIdentity from interface address.
    int generate_clock_identity(ClockIdentity &ci, const char *iface_name);
}
