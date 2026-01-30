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
#include <cstddef>
#include <cstdint>

namespace tsyncd
{
    bool parse_gptp_message(const unsigned char *buffer, std::size_t buffer_len, PTPMessage &msg);
}
