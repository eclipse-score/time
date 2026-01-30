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
#include <cstddef>
#include <string>

namespace score_time::ipc
{

    class ShmRegion final
    {
    public:
        ShmRegion() = default;
        ~ShmRegion();

        ShmRegion(const ShmRegion &) = delete;
        ShmRegion &operator=(const ShmRegion &) = delete;

        ShmRegion(ShmRegion &&other) noexcept;
        ShmRegion &operator=(ShmRegion &&other) noexcept;

        // create_or_open: true -> create if missing, set size; false -> open only.
        bool Open(const std::string &name, std::size_t size, bool create_or_open);
        void Close();

        void *Addr() const { return addr_; }
        std::size_t Size() const { return size_; }
        int Fd() const { return fd_; }
        bool Valid() const { return addr_ != nullptr; }

    private:
        std::string name_;
        int fd_ = -1;
        void *addr_ = nullptr;
        std::size_t size_ = 0;
    };

}
