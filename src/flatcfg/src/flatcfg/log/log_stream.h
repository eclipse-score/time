/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_LOG_LOG_STREAM_H
#define FLATCFG_LOG_LOG_STREAM_H

#include "common.h"

#include "flatcfg/fs/_os.h"

#include "score/result/error.h"

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

namespace flatcfg
{
namespace log
{

/// @brief Equivalent to score::mw::log::LogStream but with zero allocations.
class LogStream
{
public:
    /// @brief Construct a stream.
    LogStream(LogLevel logLevel) noexcept;

    /// @brief Destructor.
    ~LogStream() noexcept;

    /// @brief Move (but not copy) constructible.
    LogStream(LogStream&&) = default;

    /// @brief Move (but not copy) assignable
    LogStream&
    operator=(LogStream&&) = default;

    /// @brief Log bool.
    LogStream&
    operator<<(bool value) noexcept;

    /// @brief Log unsigned char.
    LogStream&
    operator<<(unsigned char value) noexcept;

    /// @brief Log unsigned short.
    LogStream&
    operator<<(unsigned short value) noexcept;

    /// @brief Log unsigned int.
    LogStream&
    operator<<(unsigned int value) noexcept;

    /// @brief Log unsigned long.
    LogStream&
    operator<<(unsigned long value) noexcept;

    /// @brief Log unsigned long long.
    LogStream&
    operator<<(unsigned long long value) noexcept;

    /// @brief Log signed char.
    LogStream&
    operator<<(signed char value) noexcept;

    /// @brief Log signed short.
    LogStream&
    operator<<(signed short value) noexcept;

    /// @brief Log signed int.
    LogStream&
    operator<<(signed int value) noexcept;

    /// @brief Log signed long.
    LogStream&
    operator<<(signed long value) noexcept;

    /// @brief Log signed long long.
    LogStream&
    operator<<(signed long long value) noexcept;

    /// @brief Log float.
    LogStream&
    operator<<(float value) noexcept;

    /// @brief Log double.
    LogStream&
    operator<<(double value) noexcept;

    /// @brief Log memory address.
    LogStream&
    operator<<(const void *value) noexcept;

    /// @brief Log null-terminated C string.
    LogStream&
    operator<<(const char *value) noexcept;

    /// @brief Log a character.
    LogStream&
    operator<<(char value) noexcept;

    /// @brief Log std::string_view.
    LogStream&
    operator<<(std::string_view value) noexcept;

    /// @brief Log ErrorCode.
    LogStream&
    operator<<(const score::result::Error& value) noexcept;

private:
    /// @brief Write data to buffer. If buffer is full, write to file handle
    ///        and refill.
    void
    writeToBuffer(std::string_view data) noexcept;

    /// @brief Write buffer.
    std::array<char, 256> m_buffer {};

    /// @brief Position in write buffer.
    std::size_t m_pos {};

    /// @brief File handle to which to write logs.
    std::optional<fs::_os::FileHandleT> m_handleOpt {};
};

}  // namespace log
}  // namespace flatcfg

#endif  // FLATCFG_LOG_LOG_STREAM_H
