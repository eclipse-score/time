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
#include <time.h>

namespace tsyncd
{
    int setup_raw_socket(int &sockFd, const char *interface_name);
    int raw_sendMsg(int sockFd, void *send_buffer, int bufLen, ::timespec *hwts);
    int raw_recvMsg(int sockFd, void *recv_buffer, ::timespec *hwts, int flag); // flag:0 blocking,1 errqueue
}
