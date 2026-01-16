/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "program_info.h"
#include "libc.h"
#include "logging.h"

#include "flatcfg/fs/_os.h"
#include "flatcfg/flatcfg_error.h"

#include "score/result/result.h"

#include <algorithm>

using score::Result;
using score::cpp::span;

namespace flatcfg
{
namespace utils
{

Result<std::size_t>
ProgramInfo::executablePath(span<char> outputBuffer) noexcept
{
    // unsupported buffer size -> nullopt
    if (outputBuffer.empty())
    {
        FLATCFG_LOG_WARN() <<
            "output buffer is empty";
        return score::MakeUnexpected(FlatCfgErrorCode::kBadLength);
    }

#if FLATCFG_INTERNAL_HAS_LINUX
    // read path of executable
    auto sizeRes = utils::Libc::readlink("/proc/self/exe", outputBuffer);
    if (!sizeRes)
    {
        return score::MakeUnexpected(FlatCfgErrorCode::kOsError, sizeRes.error().UserMessage());
    }
    std::size_t size = *sizeRes;

    // check that size is smaller than output buffer, so we can null-terminate
    // have to do it this way because readlink silently truncates
    if (size >= outputBuffer.size())
    {
        FLATCFG_LOG_WARN() <<
            "the size of the executable path (" << size << ") exceeds the size"
            " of the output buffer (" << outputBuffer.size() << ")";
        return score::MakeUnexpected<std::size_t>(FlatCfgErrorCode::kBadLength);
    }

    // null-terminate and return
    outputBuffer[size] = '\0';
    return size;

#elif FLATCFG_INTERNAL_HAS_QNX
    // open file to read
    auto fdRes = utils::Libc::open("/proc/self/exefile", O_RDONLY);
    if (!fdRes)
    {
        return score::MakeUnexpected<std::size_t>(FlatCfgErrorCode::kOsError, fdRes.error().UserMessage());
    }
    int fd = *fdRes;

    // read contents of file
    auto sizeRes = utils::Libc::read(fd, outputBuffer.data(), outputBuffer.size() - 1U);
    if (!sizeRes))
    {
        utils::Libc::close(fd);
        return score::MakeUnexpected<std::size_t>(FlatCfgErrorCode::kOsError, sizeRes.error().UserMessage());
    }
    std::size_t size = *sizeRes;

    // check we're at the end of the file
    char buf[1] {};
    auto eofSizeRes = utils::Libc::read(fd, buf, 1U);
    utils::Libc::close(fd);
    if (!eofSizeRes)
    {
        return score::MakeUnexpected<std::size_t>(FlatCfgErrorCode::kOsError, eofSizeRes.error().UserMessage());
    }
    std::size_t eofSize = *eofSizeRes;
    if (eofSize != 0U)
    {
        FLATCFG_LOG_WARN() <<
            "the executable path cannot be completely read into the output"
            " buffer of size: " << outputBuffer.size();
        return score::MakeUnexpected<std::size_t>(FlatCfgErrorCode::kBadLength);
    }

    // null-terminate and return
    outputBuffer[size] = '\0';
    return size;

#elif FLATCFG_INTERNAL_HAS_MACOS
    // read process path
    char pathBuf[PROC_PIDPATHINFO_MAXSIZE] {};
    const pid_t pid = utils::Libc::getpid();
    auto sizeRes = utils::Libc::proc_pidpath(pid, pathBuf, sizeof(pathBuf));
    if (!sizeRes))
    {
        return return score::MakeUnexpected<std::size_t>(FlatCfgErrorCode::kOsError, sizeRes.error().UserMessage());
    }
    std::size_t size = *sizeRes;

    // check path isn't too big
    if (size >= outputBuffer.size())
    {
        FLATCFG_LOG_WARN() <<
            "the size of the executable path (" << size << ") exceeds the size"
            " of the output buffer (" << outputBuffer.size() << ")";
        return score::MakeUnexpected<std::size_t>(FlatCfgErrorCode::kBadLength);
    }

    // copy over with null-terminator and return
    span<char> pathSpan { pathBuf, size };
    auto it = std::copy(pathSpan.begin(), pathSpan.end(), outputBuffer.begin());
    *it = '\0';
    return size;

#else
    #error Unsupported OS
#endif
}

}  // namespace utils
}  // namespace flatcfg
