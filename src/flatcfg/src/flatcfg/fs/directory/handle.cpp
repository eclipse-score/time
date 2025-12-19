/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "flatcfg/fs/directory/handle.h"
#include "flatcfg/internal/fs/file/type.h"
#include "flatcfg/utils/libc.h"
#include "flatcfg/utils/logging.h"
#include "flatcfg/flatcfg_error.h"
#include "score/result/result.h"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <limits>

using score::Result;
using score::ResultBlank;
using score::cpp::span;

namespace flatcfg
{
namespace fs
{

Result<DirectoryHandle>
DirectoryHandle::FromPath(std::string_view directoryPath) noexcept
{
    ResultBlank res;
    DirectoryHandle handle { directoryPath, &res };
    if (res)
    {
        return handle;
    }
    else
    {
        return score::MakeUnexpected<DirectoryHandle>(res.error());
    }
}

Result<bool>
DirectoryHandle::isDirectoryPath(std::string_view path) noexcept
{
#if FLATCFG_INTERNAL_HAS_POSIX
    auto statRes = utils::Libc::lstat(path.data());
    if (!statRes)
    {
        if (*(statRes.error()) == ENOENT)
        {
            return false;
        }
        else
        {
            return score::MakeUnexpected(FlatCfgErrorCode::kOsError, statRes.error().UserMessage());
        }
    }
    // RULECHECKER_comment(1, 1, check_union_object, "Union is part of struct ::stat, nothing we can do.", true)
    struct stat st = *statRes;
    // RULECHECKER_comment(1, 1, check_underlying_signedness_conversion, "Conversion is part of S_ISDIR.", true)
    return S_ISDIR(st.st_mode);
#else
    #error Unsupported OS
#endif
}

DirectoryHandle::~DirectoryHandle() noexcept
{
    if (m_handleOpt)
    {
#if FLATCFG_INTERNAL_HAS_POSIX
        utils::Libc::close(*m_handleOpt);
#else
        #error Unsupported OS
#endif
    }
}

DirectoryHandle::DirectoryHandle(DirectoryHandle&& other) noexcept
    : m_handleOpt(std::move(other.m_handleOpt)),
      m_directoryPath(std::move(other.m_directoryPath))
{
    // "close" other handle
    other.m_handleOpt = std::nullopt;
}

DirectoryHandle&
DirectoryHandle::operator=(DirectoryHandle&& other) noexcept
{
    // skip self assignment
    if (&other == this)
    {
        return *this;
    }

    // steal handle and "close" other handle
    m_handleOpt = std::move(other.m_handleOpt);
    m_directoryPath = std::move(other.m_directoryPath);
    other.m_handleOpt = std::nullopt;
    return *this;
}

Result<std::size_t>
DirectoryHandle::readEntriesIntoBuffer(span<char> outputBuffer) const noexcept
{
    // check that we have a handle
    if (!m_handleOpt.has_value())
    {
        FLATCFG_LOG_WARN() <<
            "cannot read entries from a closed directory handle";
        return score::MakeUnexpected(FlatCfgErrorCode::kHandleClosed);
    }

    // limit buffer size to INT_MAX
    // don't want to this to exceed the value of ssize_t, and since that's
    //   platform dependent, just use int as a stand-in
    constexpr int intMax = std::numeric_limits<int>::max();
    const auto bufferSize = std::min(outputBuffer.size(), static_cast<std::size_t>(intMax));

    // read entries
#if FLATCFG_INTERNAL_HAS_LINUX
    auto sizeRes = utils::Libc::sys_getdents64(*m_handleOpt, outputBuffer.data(), bufferSize);
#elif FLATCFG_INTERNAL_HAS_QNX
    // RULECHECKER_comment(1, 1, check_enum_usage, "Required by OS.", true)
    constexpr unsigned int xtype = static_cast<unsigned int>(_IO_XTYPE_READDIR | _IO_XFLAG_DIR_STAT_FORM_T64_2008);
    auto sizeRes = utils::Libc::_readx(*m_handleOpt, outputBuffer.data(), bufferSize, xtype, nullptr, 0U);
#elif FLATCFG_INTERNAL_HAS_MACOS
    long unusedBaseP {};
    auto sizeRes = utils::Libc::sys_getdirentries64(*m_handleOpt, outputBuffer.data(), bufferSize, &unusedBaseP);
#else
    #error Unsupported OS
#endif

    // return number of bytes read
    if (!sizeRes)
    {
        return score::MakeUnexpected(FlatCfgErrorCode::kOsError, sizeRes.error().UserMessage());
    }
    std::size_t bytesRead = *sizeRes;
    return static_cast<std::size_t>(bytesRead);
}

Result<DirectoryEntryView>
DirectoryHandle::parseEntryFromBuffer(span<const char> buffer) const noexcept
{
    // check that we have a handle
    if (!m_handleOpt.has_value())
    {
        FLATCFG_LOG_WARN() <<
            "cannot parse entries with a closed directory handle";
        return score::MakeUnexpected<DirectoryEntryView>(FlatCfgErrorCode::kHandleClosed);
    }

    // check that the buffer can hold a complete entry (except the name)
#if FLATCFG_INTERNAL_HAS_POSIX
    // +2 for smallest possible null-terminated name
    constexpr std::size_t minEntrySize = offsetof(_os::DirEntryKernelT, d_name) + 2U;
#else
    #error Unsupported OS
#endif
    if (buffer.size() < minEntrySize)
    {
        FLATCFG_LOG_WARN() <<
            "buffer size (" << buffer.size() << ") is smaller than the minimum"
            " size of an entry (" << minEntrySize << ")";
        return score::MakeUnexpected<DirectoryEntryView>(FlatCfgErrorCode::kBadLength);
    }

    // parse file identifier
#if FLATCFG_INTERNAL_HAS_POSIX
    decltype(_os::DirEntryKernelT::d_ino) ino {};
    const char *inoPtr = std::next(buffer.data(), static_cast<std::ptrdiff_t>(offsetof(_os::DirEntryKernelT, d_ino)));
#else
    #error Unsupported OS
#endif
    std::memcpy(&ino, inoPtr, sizeof(ino));
    std::uint64_t id = static_cast<std::uint64_t>(ino);

    // parse entry size
#if FLATCFG_INTERNAL_HAS_POSIX
    decltype(_os::DirEntryKernelT::d_reclen) reclen {};
    const char *recPtr = std::next(buffer.data(), static_cast<std::ptrdiff_t>(offsetof(_os::DirEntryKernelT, d_reclen)));
#else
    #error Unsupported OS
#endif
    std::memcpy(&reclen, recPtr, sizeof(reclen));
    std::uint64_t entrySize = static_cast<std::uint64_t>(reclen);

    // parse file name size
#if FLATCFG_INTERNAL_HAS_LINUX
    // need to manually check size because reclen may include alignment padding
    const std::uint64_t remainingEntrySize = static_cast<std::uint64_t>(
        static_cast<std::int64_t>(entrySize) -
        static_cast<std::int64_t>(offsetof(_os::DirEntryKernelT, d_name)));
    const std::uint64_t remainingBufferSize =
        buffer.size() - offsetof(_os::DirEntryKernelT, d_name);
    const auto nameSizeUpperBound = std::min(
        remainingEntrySize, remainingBufferSize);
    auto nameBegin = std::next(
        buffer.data(),
        static_cast<std::ptrdiff_t>(offsetof(_os::DirEntryKernelT, d_name)));
    auto nameEnd = std::next(
        nameBegin, static_cast<std::ptrdiff_t>(nameSizeUpperBound));
    auto nullIt = std::find(nameBegin, nameEnd, '\0');
    if (nullIt == nameEnd)
    {
        std::string_view nameStart { nameBegin, nameSizeUpperBound };
        FLATCFG_LOG_WARN() << "entry name for entry id " << id
                           << " with entry size " << entrySize
                           << " extends past end of entry or is not "
                              "null-terminated;"
                              " name starts with: "
                           << nameStart;
        return score::MakeUnexpected<DirectoryEntryView>(FlatCfgErrorCode::kBadLength);
    }
    auto nameSize = std::distance(nameBegin, nullIt);
#elif FLATCFG_INTERNAL_HAS_QNX
    decltype(_os::DirEntryKernelT::d_namelen) nameSize {};
    const char *namelenPtr = std::next(buffer.data(), static_cast<std::ptrdiff_t>(offsetof(_os::DirEntryKernelT, d_namelen)));
    std::memcpy(&nameSize, namelenPtr, sizeof(nameSize));
#elif FLATCFG_INTERNAL_HAS_MACOS
    decltype(_os::DirEntryKernelT::d_namlen) nameSize {};
    const char *namelenPtr = std::next(buffer.data(), static_cast<std::ptrdiff_t>(offsetof(_os::DirEntryKernelT, d_namlen)));
    std::memcpy(&nameSize, namelenPtr, sizeof(nameSize));
#else
    #error Unsupported OS
#endif

    // parse file name
#if FLATCFG_INTERNAL_HAS_POSIX
    // RULECHECKER_comment(1, 1, check_underlying_signedness_conversion, "Conversion is correct.", true)
    const char *namePtr = std::next(buffer.data(), static_cast<std::ptrdiff_t>(offsetof(_os::DirEntryKernelT, d_name)));
#else
    #error Unsupported OS
#endif
    auto name = std::string_view { namePtr, static_cast<std::size_t>(nameSize) };

    // check that string is not empty
    if (name.empty())
    {
        FLATCFG_LOG_WARN() <<
            "entry name for entry id " << id << " is empty; paths cannot be"
            " empty";
        return score::MakeUnexpected(FlatCfgErrorCode::kBadPathName);
    }

    // check that the entry is big enough to hold the whole name including null-terminator
    // this is an idiot check, do not rely on this for correctness
    if (entrySize < name.size())
    {
        FLATCFG_LOG_WARN() <<
            "entry size " << entrySize << " for entry id " << id << " is"
            " smaller than the entry name (" << name.size() << "): " << name;
        return score::MakeUnexpected(FlatCfgErrorCode::kBadLength);
    }

    // check that the buffer actually holds the whole name including null-terminator
    // DO NOT create a pointer that points outside the buffer (undefined behaviour)
    auto bytesRemaining = buffer.size();
    bytesRemaining -= static_cast<std::size_t>(std::distance(buffer.data(), name.data()));
    // size doesn't include required null-terminator
    if (bytesRemaining < (name.size() + 1U))
    {
        FLATCFG_LOG_WARN() <<
            "entry name extends past end of buffer; buffer has " <<
            bytesRemaining << " bytes but null-terminated entry name would"
            " require: " << (name.size() + 1U) << " bytes";
        return score::MakeUnexpected(FlatCfgErrorCode::kBadLength);
    }

    // check that the name does not contain a null byte
    if (std::find(name.begin(), name.end(), '\0') != name.end())
    {
        FLATCFG_LOG_WARN() <<
            "entry name for id " << id << " contains null bytes; paths cannot"
            " contain null bytes; entry name (" << name.size() << "): " << name;
        return score::MakeUnexpected(FlatCfgErrorCode::kBadPathName);
    }

    // check that the name is null-terminated
    // this is safe because we checked above that the buffer extends at least
    //   far enough to hold a null terminator
    std::ptrdiff_t nameSsize = static_cast<std::ptrdiff_t>(name.size());
    const char *it = std::next(name.data(), nameSsize);
    if (*it != '\0')
    {
        FLATCFG_LOG_WARN() <<
            "entry name for id " << id << " is not null-terminated; entry"
            " name (" << name.size() << "): " << name;
        return score::MakeUnexpected(FlatCfgErrorCode::kBadPathName);
    }

    // parse file type
#if FLATCFG_INTERNAL_HAS_LINUX || FLATCFG_INTERNAL_HAS_MACOS
    decltype(_os::DirEntryKernelT::d_type) type {};
    const char *typePtr = std::next(buffer.data(), static_cast<std::ptrdiff_t>(offsetof(_os::DirEntryKernelT, d_type)));
    std::memcpy(&type, typePtr, sizeof(type));
    FileType fileType = fileTypeFromDirentType(type);
#elif FLATCFG_INTERNAL_HAS_QNX
    // TODO: qnx needs stat or dirent_extra
    // annoyingly qnx does not support fstatat properly
    // fall back to lstat
    // cannot lstat "." or ".."
    auto fileType = FileType::DIRECTORY;
    if (name != "." && name != "..")
    {
        std::string path = m_directoryPath;
        if (!path.empty() && path.back() != '/')
        {
            path += '/';
        }

        path += name;
        auto statRes = utils::Libc::lstat(path.c_str());
        if (!statRes)
        {
            return score::MakeUnexpected(FlatCfgErrorCode::kOsError, statRes.error().UserMessage());
        }
        // RULECHECKER_comment(1, 1, check_union_object, "Union is part of struct ::stat, nothing we can do.", true)
        struct stat st = *statRes;
        fileType = fileTypeFromStatMode(st.st_mode);
    }
#else
    #error Unsupported OS
#endif

    // success
    return DirectoryEntryView::FromValues(id, entrySize, fileType, name);
}

// coverity[autosar_cpp14_a8_4_10_violation:Intentional] Pointer to mark an out parameter.
DirectoryHandle::DirectoryHandle(std::string_view directoryPath, ResultBlank *resOut) noexcept
    : m_directoryPath(directoryPath)
{
#if FLATCFG_INTERNAL_HAS_POSIX
    Result<int> fdRes {};
#if FLATCFG_INTERNAL_HAS_QNX
    fdRes = utils::Libc::open(directoryPath.data(), O_RDONLY | O_EXCL | O_LARGEFILE);
#else
    fdRes = utils::Libc::open(directoryPath.data(), O_RDONLY | O_DIRECTORY);
#endif
    if (!fdRes)
    {
        *resOut = score::MakeUnexpected(FlatCfgErrorCode::kOsError, fdRes.error().UserMessage());
        return;
    }
    m_handleOpt = *fdRes;
#else
    #error Unsupported OS
#endif
}

}  // namespace fs
}  // namespace flatcfg
