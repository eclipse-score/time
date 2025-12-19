/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "log_stream.h"
#include "flatcfg/utils/ctc.h"
#include "flatcfg/utils/libc.h"

#include <cstdio>
#include <cstdint>
#include <iterator>
#include <limits>

#include "score/time/common/Abort.h"

namespace
{

using flatcfg::fs::_os::FileHandleT;
using score::time::common::logFatalAndAbort;

/// @brief Get the file handle for stdout.
FileHandleT
// coverity[autosar_cpp14_a0_1_3_violation] This method is being used in this module
// coverity[autosar_cpp14_a7_5_2_violation:Intentional] False positive.
openStdout() noexcept
{
#if FLATCFG_INTERNAL_HAS_POSIX
    auto fdRes = flatcfg::utils::Libc::fileno(stdout);
    FLATCFG_SKIPCOV_BEGIN();
    if (!fdRes)
    {
        logFatalAndAbort("Could not obtain underlying file descriptor from stdout");
    }
    FLATCFG_SKIPCOV_END();
    return fdRes.value();
#else
    #error Unsupported OS
#endif
}

/// @brief Write data to a file handle.
void
// coverity[autosar_cpp14_a7_5_2_violation:Intentional] False positive.
writeToHandle(FileHandleT file, const void *buf, std::size_t count) noexcept
{
#if FLATCFG_INTERNAL_HAS_POSIX
    static_cast<void>(flatcfg::utils::Libc::write(file, buf, count));
#else
    #error Unsupported OS
#endif
}

}  // namespace

namespace flatcfg
{
namespace log
{

LogStream::LogStream(LogLevel logLevel) noexcept
{
    // only try to get stdout if we will actually log something
    if (logLevel != LogLevel::kOff)
    {
        m_handleOpt = openStdout();
    }
}

LogStream::~LogStream() noexcept
{
    *this << '\n';
    if (m_handleOpt.has_value())
    {
        writeToHandle(*m_handleOpt, m_buffer.data(), m_pos);
    }
}

LogStream&
LogStream::operator<<(bool value) noexcept
{
    writeToBuffer(value ? "true" : "false");
    return *this;
}

LogStream&
LogStream::operator<<(unsigned char value) noexcept
{
    return this->operator<<(static_cast<unsigned long long>(value));
}

LogStream&
LogStream::operator<<(unsigned short value) noexcept
{
    return this->operator<<(static_cast<unsigned long long>(value));
}

LogStream&
LogStream::operator<<(unsigned int value) noexcept
{
    return this->operator<<(static_cast<unsigned long long>(value));
}

LogStream&
// coverity[autosar_cpp14_a7_5_2_violation:Intentional] False positive.
LogStream::operator<<(unsigned long value) noexcept
{
    return this->operator<<(static_cast<unsigned long long>(value));
}

LogStream&
LogStream::operator<<(unsigned long long value) noexcept
{
    // +2: 1 for null-terminator, 1 because log10(2) in digits10 rounds down
    // RULECHECKER_comment(1, 1, check_underlying_signedness_conversion, "Fine because issue would be caught at compile time.", true)
    char buf[std::numeric_limits<unsigned long long>::digits10 + 2U];

    // cannot fail
    // inputs are restricted to known values, so EILSEQ, EINVAL, and SIGSEGV
    //   would be caught in tests
    int res = std::snprintf(buf, sizeof(buf), "%llu", value);

    // coverity[cert_err33_c_violation:Intentional] Issue for core-types.
    std::string_view view { buf, static_cast<std::size_t>(res) };
    writeToBuffer(view);
    return *this;
}

LogStream&
LogStream::operator<<(signed char value) noexcept
{
    return this->operator<<(static_cast<signed long long>(value));
}

LogStream&
LogStream::operator<<(signed short value) noexcept
{
    return this->operator<<(static_cast<signed long long>(value));
}

LogStream&
// coverity[autosar_cpp14_a7_5_2_violation:Intentional] False positive.
LogStream::operator<<(signed int value) noexcept
{
    return this->operator<<(static_cast<signed long long>(value));
}

LogStream&
LogStream::operator<<(signed long value) noexcept
{
    return this->operator<<(static_cast<signed long long>(value));
}

LogStream&
LogStream::operator<<(signed long long value) noexcept
{
    // +3: 1 for null-terminator, 1 because log10(2) in digits10 rounds down,
    //     1 for minus sign when negative
    // RULECHECKER_comment(1, 1, check_underlying_signedness_conversion, "Fine because issue would be caught at compile time.", true)
    char buf[std::numeric_limits<signed long long>::digits10 + 3U];

    // cannot fail
    // inputs are restricted to known values, so EILSEQ, EINVAL, and SIGSEGV
    //   would be caught in tests
    int res = std::snprintf(buf, sizeof(buf), "%lld", value);

    // coverity[cert_err33_c_violation:Intentional] Issue for core-types.
    std::string_view view { buf, static_cast<std::size_t>(res) };
    writeToBuffer(view);
    return *this;
}

LogStream&
LogStream::operator<<(float value) noexcept
{
    return this->operator<<(static_cast<double>(value));
}

LogStream&
LogStream::operator<<(double value) noexcept
{
    // floating point types are difficult
    // be generous and test limits
    char buf[16U] {};

    // cannot fail
    // inputs are restricted to known values, so EILSEQ, EINVAL, and SIGSEGV
    //   would be caught in tests
    int res = std::snprintf(buf, sizeof(buf), "%e", value);

    // coverity[cert_err33_c_violation:Intentional] Issue for core-types.
    std::string_view view { buf, static_cast<std::size_t>(res) };
    writeToBuffer(view);
    return *this;
}

LogStream&
LogStream::operator<<(const void *value) noexcept
{
    // +2: for 0x prefix
    // +2: 1 for null-terminator, 1 because log10(2) in digits10 rounds down
    // RULECHECKER_comment(1, 1, check_underlying_signedness_conversion, "Fine because issue would be caught at compile time.", true)
    char buf[std::numeric_limits<unsigned long long>::digits10 + 4U];
    // RULECHECKER_comment(1, 1, check_reinterpret_cast_extended, "Required because we need pointer's address value.", true)
    unsigned long long addr = static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(value));

    // cannot fail
    // inputs are restricted to known values, so EILSEQ, EINVAL, and SIGSEGV
    //   would be caught in tests
    int res = std::snprintf(buf, sizeof(buf), "0x%llx", addr);

    // coverity[cert_err33_c_violation:Intentional] Issue for core-types.
    std::string_view view { buf, static_cast<std::size_t>(res) };
    writeToBuffer(view);
    return *this;
}

LogStream&
// coverity[autosar_cpp14_a7_5_2_violation:Intentional] False positive.
LogStream::operator<<(const char *value) noexcept
{
    writeToBuffer(value);
    return *this;
}

LogStream&
// coverity[autosar_cpp14_a7_5_2_violation:Intentional] False positive.
LogStream::operator<<(char value) noexcept
{
    std::string_view view { &value, 1u };
    writeToBuffer(view);
    return *this;
}

LogStream&
LogStream::operator<<(std::string_view value) noexcept
{
    writeToBuffer(value);
    return *this;
}

LogStream&
LogStream::operator<<(const score::result::Error& value) noexcept
{
    writeToBuffer(value.Message().data());
    return *this;
}

void
LogStream::writeToBuffer(std::string_view data) noexcept
{
    // skip if no handle (because LogLevel is kOff)
    if (!m_handleOpt.has_value())
    {
        return;
    }

    // write all data
    while (!data.empty())
    {
        // flush buffer if full
        if (m_pos == m_buffer.size())
        {
            writeToHandle(*m_handleOpt, m_buffer.data(), m_buffer.size());
            m_pos = 0u;
        }

        // get data we will write
        std::size_t bufRemaining = m_buffer.size() - m_pos;
        std::size_t writeSize = std::min(data.size(), bufRemaining);
        std::string_view dataToWrite = data.substr(0U, writeSize);

        // write as much data as fits in buffer
        auto it = std::next(m_buffer.data(), static_cast<std::ptrdiff_t>(m_pos));
        std::copy(dataToWrite.begin(), dataToWrite.end(), it);

        // update variables
        data.remove_prefix(writeSize);
        m_pos += writeSize;
    }
}

}  // namespace log
}  // namespace flatcfg
