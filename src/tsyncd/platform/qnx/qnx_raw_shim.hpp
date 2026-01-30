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

int qnx_raw_open(const char *ifname);
int qnx_raw_recv(int fd, void *buf, int buf_len, struct timespec *hwts, int nonblock);
int qnx_raw_send(int fd, const void *buf, int len, struct timespec *hwts);

int qnx_phc_open(const char *phc_dev);
int qnx_phc_adjtime_step(int phc_fd, long long offset_ns);
int qnx_phc_adjfreq_ppb(int phc_fd, long long freq_ppb);

#ifdef __cplusplus
}
#endif
