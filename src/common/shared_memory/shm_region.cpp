/********************************************************************************
* Copyright (c) 2025 Contributors to the Eclipse Foundation
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
#include "score_time/ipc/shm_region.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace score_time::ipc {

ShmRegion::~ShmRegion() { Close(); }

ShmRegion::ShmRegion(ShmRegion&& other) noexcept {
  *this = std::move(other);
}

ShmRegion& ShmRegion::operator=(ShmRegion&& other) noexcept {
  if (this == &other) return *this;
  Close();
  name_ = std::move(other.name_);
  fd_ = other.fd_;
  addr_ = other.addr_;
  size_ = other.size_;
  other.fd_ = -1;
  other.addr_ = nullptr;
  other.size_ = 0;
  return *this;
}

bool ShmRegion::Open(const std::string& name, std::size_t size, bool create_or_open) {
  Close();
  name_ = name;
  size_ = size;

  int oflag = O_RDWR;
  if (create_or_open) oflag |= O_CREAT;

  fd_ = ::shm_open(name.c_str(), oflag, 0660);
  if (fd_ < 0) {
    return false;
  }

  if (create_or_open) {
    if (::ftruncate(fd_, static_cast<off_t>(size_)) != 0) {
      Close();
      return false;
    }
  } else {
    // If open only, we can still accept a larger actual size. Read current size.
    struct stat st{};
    if (::fstat(fd_, &st) == 0) {
      constexpr std::size_t MAX_SHM_SIZE = 1024 * 1024 * 1024;  // 1GB
      if (st.st_size < 0 || static_cast<std::size_t>(st.st_size) > MAX_SHM_SIZE) {
        std::fprintf(stderr, "shm_region: invalid size %ld\n", static_cast<long>(st.st_size));
        Close();
        return false;
      }
      if (st.st_size > 0) {
        size_ = static_cast<std::size_t>(st.st_size);
      }
    }
  }

  addr_ = ::mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
  if (addr_ == MAP_FAILED) {
    addr_ = nullptr;
    Close();
    return false;
  }
  return true;
}

void ShmRegion::Close() {
  if (addr_) {
    ::munmap(addr_, size_);
    addr_ = nullptr;
  }
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
  size_ = 0;
  name_.clear();
}

} // namespace score_time::ipc
