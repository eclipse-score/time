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

#ifdef __cplusplus
extern "C" {
#endif

int linux_raw_open(const char *ifname);
int linux_raw_recv(int fd, void *buf, int buf_len, struct timespec *hwts, int flag);
int linux_raw_send(int fd, void *buf, int len, struct timespec *hwts);

#ifdef __cplusplus
}
#endif
