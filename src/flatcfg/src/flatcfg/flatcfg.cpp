/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "flatcfg/flatcfg.h"
#include "flatcfg/fs/_os.h"
#include "flatcfg/utils/ctc.h"
#include "flatcfg/utils/environment.h"
#include "flatcfg/utils/fb_verifier.h"
#include "flatcfg/utils/libc.h"
#include "flatcfg/utils/logging.h"
#include "flatcfg/utils/program_info.h"
#include "flatcfg/flatcfg_error.h"

#include <algorithm>
#include <array>
#include <iterator>

using score::cpp::span;

namespace
{

// use constants in case of unexpected character set (i.e. non-ascii)
// cannot use setlocale + tolower/islower because not thread safe
// coverity[autosar_cpp14_m3_4_1_violation:Intentional] Defined here for better maintainability.
constexpr std::string_view ALPHA_UPPER { "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 26U };
// coverity[autosar_cpp14_m3_4_1_violation:Intentional] Defined here for better maintainability.
constexpr std::string_view ALPHA_LOWER { "abcdefghijklmnopqrstuvwxyz", 26U };
// coverity[autosar_cpp14_m3_4_1_violation:Intentional] Defined here for better maintainability.
constexpr std::string_view ALPHA_CHARS {
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
        52U
};

}  // namespace

FLATCFG_SKIPCOV_BEGIN();

namespace flatcfg
{

FlatCfg::Deleter::Deleter(std::pmr::polymorphic_allocator<void> alloc,
                          std::size_t size) noexcept
    : m_alloc(std::move(alloc)),
      m_size(size)
{}

void
FlatCfg::Deleter::operator()(unsigned char *ptr) noexcept
{
    // rebind allocator
    using AllocVoid = std::pmr::polymorphic_allocator<void>;
    using AllocVoidTraits = std::allocator_traits<AllocVoid>;
    using Alloc = AllocVoidTraits::rebind_alloc<unsigned char>;
    using AllocTraits = AllocVoidTraits::rebind_traits<unsigned char>;
    Alloc reboundAlloc { m_alloc };

    // unsigned char[] has implicit lifetime
    // no need to destroy anything, only deallocate
    AllocTraits::deallocate(reboundAlloc, ptr, m_size);
}

// RULECHECKER_comment(1, 3, check_incomplete_data_member_construction, "All data members are constructed.", false)
// coverity[autosar_cpp14_a0_1_3_violation:Intentional] Function is called by template function in header, so appears unused but is actually used.
// coverity[autosar_cpp14_a8_4_10_violation:Intentional] Pointer to mark an out parameter.
FlatCfg::FlatCfg(std::string_view name, config::Version version, score::ResultBlank *resOut) noexcept
    : m_functionClusterMajorVersion(version.major),
      m_functionClusterMinorVersion(version.minor)
{
    // check that name isn't too large or empty
    if (name.empty())
    {
        FLATCFG_LOG_ERROR() <<
            "functionCluster name cannot be empty";
        *resOut = score::MakeUnexpected<score::Blank>(FlatCfgErrorCode::kBadArgument);
    }
    if (name.size() > 16U)
    {
        FLATCFG_LOG_ERROR() <<
            "functionCluster name size (" << name.size() << ") cannot exceed"
            " 16 bytes";
        *resOut = score::MakeUnexpected<score::Blank>(FlatCfgErrorCode::kBadArgument);
        return;
    }

    // check that name matches [a-zA-Z]+
    bool ok = std::all_of(name.begin(), name.end(), [](char c) noexcept -> bool {
        return ALPHA_CHARS.find(c) != std::string_view::npos;
    });
    if (!ok)
    {
        FLATCFG_LOG_ERROR() <<
            "functionCluster name does not match '[a-zA-Z]+': " << name;
        *resOut = score::MakeUnexpected<score::Blank>(FlatCfgErrorCode::kBadArgument);
        return;
    }

    // save as original and lower case
    FLATCFG_LOG_INFO() << "Initializing FlatCfg instance with version { .major="
        << version.major << ", .minor=" << version.minor << " } and function "
        "cluster: " << name;
    m_functionClusterName = name;
    m_functionClusterNameLowerCase.resize(name.size());
    std::transform(m_functionClusterName.begin(), m_functionClusterName.end(),
                   m_functionClusterNameLowerCase.begin(), [](char c) noexcept -> char {
        auto upperPos = ALPHA_UPPER.find(c);
        return (upperPos != std::string_view::npos) ? ALPHA_LOWER[upperPos] : c;
    });
}

score::Result<config::DaemonIterator>
FlatCfg::daemonConfig() const noexcept
{
    // try to read PROCESSIDENTIFIER environment variable
    std::array<char, fs::_os::MAX_NAME_LENGTH + 1U> procBuffer {};
    span<char> procSpan { procBuffer.data(), procBuffer.size() };
    auto procSsizeOpt = utils::Environment::processIdentifier(procSpan);

    // parse result
    std::optional<std::string_view> procOpt;
    if (procSsizeOpt.has_value())
    {
        std::ptrdiff_t ssize = *procSsizeOpt;
        if (ssize < 0)
        {
            // value was too large to store in buffer
            FLATCFG_LOG_ERROR() <<
                "environment variable PROCESSIDENTIFIER could not be read"
                " because it exceeds the size of the buffer: " <<
                procBuffer.size();
            // coverity[stack_use_local_overflow:Intentional] Store large object on stack to avoid dynamic allocation.
            return score::MakeUnexpected<config::DaemonIterator>(FlatCfgErrorCode::kBadLength);
        }
        else
        {
            std::size_t size = static_cast<std::size_t>(ssize);
            procOpt = std::string_view { procBuffer.data(), size };
        }
    }

    // try to read ECUCFG_ENV_VAR_ROOTFOLDER environment variable
    std::array<char, fs::_os::MAX_PATH_LENGTH + 1U> rootBuffer {};
    span<char> rootSpan { rootBuffer.data(), rootBuffer.size() };
    auto rootSsizeOpt = utils::Environment::ecuCfgRootFolder(rootSpan);

    // parse result
    std::optional<std::string_view> rootOpt;
    if (rootSsizeOpt.has_value())
    {
        std::ptrdiff_t ssize = *rootSsizeOpt;
        if (ssize < 0)
        {
            // value was too large to store in buffer
            FLATCFG_LOG_ERROR() <<
                "environment variable ECUCFG_ENV_VAR_ROOTFOLDER could not be"
                " read because it exceeds the size of the buffer: " <<
                procBuffer.size();
            // coverity[stack_use_local_overflow:Intentional] Store large object on stack to avoid dynamic allocation.
            return score::MakeUnexpected<config::DaemonIterator>(FlatCfgErrorCode::kBadLength);
        }
        else
        {
            std::size_t size = static_cast<std::size_t>(ssize);
            rootOpt = std::string_view { rootBuffer.data(), size };
        }
    }

    // make lookup info
    std::string_view fc { m_functionClusterNameLowerCase };
    config::Version version { m_functionClusterMajorVersion, m_functionClusterMinorVersion };
    auto infoRes = config::LookupInfo::FromValues(fc, procOpt, version);
    if (!infoRes)
    {
        infoRes = config::LookupInfo::FromValues(fc, std::nullopt, version);
        if (!infoRes)
        {
            // coverity[stack_use_local_overflow:Intentional] Store large object on stack to avoid dynamic allocation.
            return score::MakeUnexpected<config::DaemonIterator>(infoRes.error());
        }
    }
    const config::LookupInfo& info = *infoRes;

    // create iterator
    // coverity[autosar_cpp14_a18_5_8_violation:Intentional] Large object on heap to avoid dynamic memory allocation.
    // coverity[stack_use_local_overflow:Intentional] Store large object on stack to avoid dynamic allocation.
    score::Result<config::DaemonIterator> itRes;
    if (rootOpt.has_value())
    {
        // coverity[stack_use_local_overflow:Intentional] Store large object on stack to avoid dynamic allocation.
        return config::DaemonIterator::FromPathAndInfo(*rootOpt, info);
    }
    else
    {
        // coverity[stack_use_local_overflow:Intentional] Store large object on stack to avoid dynamic allocation.
        return config::DaemonIterator::FromInfo(info);
    }
}

score::Result<config::ProcessIterator>
FlatCfg::processConfig() const noexcept
{
    // scope to limit size of stack frame
    std::optional<config::LookupInfo> infoOpt;
    {
        // try to read PROCESSIDENTIFIER environment variable
        std::array<char, fs::_os::MAX_NAME_LENGTH + 1U> procBuffer {};
        span<char> procSpan { procBuffer.data(), procBuffer.size() };
        auto procSsizeOpt = utils::Environment::processIdentifier(procSpan);

        // parse result
        std::optional<std::string_view> procOpt;
        if (procSsizeOpt.has_value())
        {
            std::ptrdiff_t ssize = *procSsizeOpt;
            if (ssize < 0)
            {
                // value was too large to store in buffer
                FLATCFG_LOG_ERROR() <<
                    "environment variable PROCESSIDENTIFIER could not be read"
                    " because it exceeds the size of the buffer: " <<
                    procBuffer.size();
                return score::MakeUnexpected<config::ProcessIterator>(FlatCfgErrorCode::kBadLength);
            }
            else
            {
                std::size_t size = static_cast<std::size_t>(ssize);
                procOpt = std::string_view { procBuffer.data(), size };
            }
        }

        // try to read ECUCFG_ENV_VAR_ROOTFOLDER environment variable
        std::array<char, fs::_os::MAX_PATH_LENGTH + 1U> rootBuffer {};
        span<char> rootSpan { rootBuffer.data(), rootBuffer.size() };
        auto rootSsizeOpt = utils::Environment::ecuCfgRootFolder(rootSpan);

        // parse result
        std::optional<std::string_view> rootOpt;
        if (rootSsizeOpt.has_value())
        {
            std::ptrdiff_t ssize = *rootSsizeOpt;
            if (ssize < 0)
            {
                // value was too large to store in buffer
                FLATCFG_LOG_ERROR() <<
                    "environment variable ECUCFG_ENV_VAR_ROOTFOLDER could not be"
                    " read because it exceeds the size of the buffer: " <<
                    procBuffer.size();
                return score::MakeUnexpected<config::ProcessIterator>(FlatCfgErrorCode::kBadLength);
            }
            else
            {
                std::size_t size = static_cast<std::size_t>(ssize);
                rootOpt = std::string_view { rootBuffer.data(), size };
            }
        }

        // make lookup info
        std::string_view fc { m_functionClusterNameLowerCase };
        config::Version version { m_functionClusterMajorVersion, m_functionClusterMinorVersion };
        auto infoRes = config::LookupInfo::FromValues(fc, procOpt, version);
        if (!infoRes)
        {
            infoRes = config::LookupInfo::FromValues(fc, std::nullopt, version);
            if (!infoRes)
            {
                return score::MakeUnexpected<config::ProcessIterator>(infoRes.error());
            }
        }
        infoOpt = *infoRes;

        // create iterator from environment variable if set
        if (rootOpt.has_value())
        {
            return config::ProcessIterator::FromPathAndInfo(*rootOpt, *infoOpt);
        }
    }

    // try to read the executable path
    // coverity[autosar_cpp14_a18_5_8_violation] The allocation does not lead to a foreseeable safety risk
    std::array<char, fs::_os::MAX_PATH_LENGTH + 1U> exeBuffer {};
    span<char> exeSpan { exeBuffer.data(), exeBuffer.size() };
    auto exeSizeRes = utils::ProgramInfo::executablePath(exeSpan);

    // parse result
    if (!exeSizeRes)
    {
        return score::MakeUnexpected<config::ProcessIterator>(exeSizeRes.error());
    }
    std::string_view exePath { exeBuffer.data(), *exeSizeRes };
    if (exePath.empty())
    {
        return score::MakeUnexpected<config::ProcessIterator>(FlatCfgErrorCode::kBadPathName);
    }

    // go to parent directory
    auto lastSlashPos = exePath.find_last_of('/');
    if (lastSlashPos == std::string_view::npos)
    {
        lastSlashPos = exePath.size();
    }
    std::string_view parentPath { exeBuffer.data(), lastSlashPos };

    // check we can append "/../etc" to parent directory (plus null-terminator)
    constexpr std::string_view etc { "/../etc", 7U };
    // coverity[autosar_cpp14_a4_7_1_violation:Intentional] Will not wrap due to program design.
    if ((parentPath.size() + etc.size() + 1U) > exeBuffer.size())
    {
        // value is too large to store in buffer
        FLATCFG_LOG_ERROR() <<
            "process executable directory path appended with '" << etc <<
            "' and null-terminated exceeds the size of the buffer (" <<
            exeBuffer.size() << "), would be path (" << (parentPath.size() + etc.size()) <<
            "): " << parentPath << etc << '\n';
        return score::MakeUnexpected<config::ProcessIterator>(FlatCfgErrorCode::kBadLength);
    }

    // append "/../etc" to parent directory (plus null-terminator)
    auto it = std::next(exeBuffer.begin(), static_cast<std::ptrdiff_t>(lastSlashPos));
    // coverity[autosar_cpp14_m5_0_15_violation:Intentional] No arithmetic performed.
    it = std::copy(etc.begin(), etc.end(), it);
    *it = '\0';
    std::size_t etcSize = static_cast<std::size_t>(std::distance(exeBuffer.begin(), it));
    std::string_view etcPath { exeBuffer.data(), etcSize };

    // create iterator from etc path
    return config::ProcessIterator::FromPathAndInfo(etcPath, *infoOpt);
}

score::Result<std::size_t>
FlatCfg::size(const config::Identifier& id) noexcept
{
#if FLATCFG_INTERNAL_HAS_POSIX
    // get file size
    auto statRes = utils::Libc::stat(id.path().data());
    if (!statRes)
    {
        return score::MakeUnexpected(FlatCfgErrorCode::kOsError, statRes.error().UserMessage());
    }

    // return size
    return static_cast<std::size_t>(statRes->st_size);
#else
    #error Unsupported OS
#endif
}


score::Result<std::unique_ptr<unsigned char[], FlatCfg::Deleter>>
FlatCfg::load(const config::Identifier& id,
    std::pmr::polymorphic_allocator<void> alloc) noexcept
{
    // rebind allocator
    using AllocVoid = std::pmr::polymorphic_allocator<void>;
    using AllocVoidTraits = std::allocator_traits<AllocVoid>;
    using Alloc = AllocVoidTraits::rebind_alloc<unsigned char>;
    using AllocTraits = AllocVoidTraits::rebind_traits<unsigned char>;
    Alloc reboundAlloc { alloc };

#if FLATCFG_INTERNAL_HAS_POSIX
    // open the file
    auto fdRes = utils::Libc::open(id.path().data(), O_RDONLY);
    if (!fdRes)
    {
        return score::MakeUnexpected(FlatCfgErrorCode::kOsError, fdRes.error().UserMessage());
    }
    int fd = *fdRes;

    // get file size
    auto statRes = utils::Libc::fstat(fd);
    if (!statRes)
    {
        return score::MakeUnexpected(FlatCfgErrorCode::kOsError, statRes.error().UserMessage());
    }

    // RULECHECKER_comment(1, 1, check_union_object, "Union is part of struct ::stat, nothing we can do.", true)
    struct stat st = *statRes;

    // allocate buffer
    unsigned char *ptr = AllocTraits::allocate(reboundAlloc, static_cast<std::size_t>(st.st_size));
    if (ptr == nullptr)
    {
        FLATCFG_LOG_ERROR() <<
            "failed to allocate " << st.st_size << " bytes";
        return score::MakeUnexpected<std::unique_ptr<unsigned char[], Deleter>>(FlatCfgErrorCode::kBadAlloc);
    }

    // save into unique ptr here so we don't have to worry about memory leaks
    Deleter del { std::move(alloc), static_cast<std::size_t>(st.st_size) };
    std::unique_ptr<unsigned char[], Deleter> up { ptr, del };

    // read file into buffer
    auto sizeRes = utils::Libc::read(fd, ptr, static_cast<std::size_t>(st.st_size));
    if (!sizeRes)
    {
        return score::MakeUnexpected(FlatCfgErrorCode::kOsError, sizeRes.error().UserMessage());
    }
    std::size_t binSize = *sizeRes;

    // verify binary data
    // cheat and do reinterpret_cast with double static_cast to avoid SCA warning
    char *binPtr = static_cast<char *>(static_cast<void *>(ptr));
    std::string_view bin { binPtr, binSize };
    auto verifyRes = utils::FbVerifier::verifyBinary(bin, id.functionCluster(),
                                                     id.version().major,
                                                     id.version().minor);
    if (!verifyRes)
    {
        return score::MakeUnexpected<std::unique_ptr<unsigned char[], Deleter>>(verifyRes.error());
    }

    // success
    return up;

#else
    #error Unsupported OS
#endif
}

}  // namespace flatcfg

FLATCFG_SKIPCOV_END();
