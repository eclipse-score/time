/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "SysCallsShMemMock.h"

namespace score {
namespace time {

int OsShmOpen(const char* __name, int __oflag, mode_t __mode) {
    return shared_mem_mock->OsShmOpen(__name, __oflag, __mode);
}

int OsShmUnlink(const char* name) {
    return shared_mem_mock->OsShmUnlink(name);
}

int OsFtruncate(int __fd, off_t __length) {
    return shared_mem_mock->OsFtruncate(__fd, __length);
}

void* OsMmap(void* __addr, size_t __len, int __prot, int __flags, int __fd, off_t __offset) {
    return shared_mem_mock->OsMmap(__addr, __len, __prot, __flags, __fd, __offset);
}

int OsMunmap(void* addr, size_t len) {
    return shared_mem_mock->OsMunmap(addr, len);
}

int OsClose(int __fd) {
    if (shared_mem_mock) {
        return shared_mem_mock->OsClose(__fd);
    } else {
        return 0;
    }
}

}  // namespace time
}  // namespace score
