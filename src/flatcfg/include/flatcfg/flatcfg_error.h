/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_FLATCFG_ERROR_H
#define FLATCFG_FLATCFG_ERROR_H

#include "_export.h"

#include "score/result/error.h"
#include "score/result/error_domain.h"

namespace flatcfg
{

/// @brief Error codes for FlatCfg.
enum class FlatCfgErrorCode : score::result::ErrorCode
{
    // old error codes
    kNotInitialized = 256,
    kGeneric = 257,
    kParameter = 258,
    kInvalidConfigPath = 259,
    kInvalidFileName = 260,
    kConfigFiles = 261,
    kMultipleProcessesInPath = 262,
    kMemoryAllocationFailed = 263,
    kMemoryAlreadyAllocated = 264,
    kInvalidSWCLName = 265,
    kFileNotFound = 266,
    kFileEmpty = 267,
    kFileAccessError = 268,
    kInvalidBinaryFormat = 269,
    kBadName = 270,
    kBadVersionMajor = 271,
    kBadVersionMinor = 272,
    kInvalidDirectory = 273,
    kUnknown = 511,

    /// new error codes
    kBadLength,
    kBadPathName,
    kBadArgument,
    kHandleClosed,
    kBadAlloc,
    kOsError,
    kBadFormat = kInvalidBinaryFormat,
    kVersionMismatch = kBadVersionMajor,
};

class FlatCfgErrorDomain final : public score::result::ErrorDomain
{
public:
    /// @brief Returns the message corresponding to a FlatCfg error code .
    inline std::string_view MessageFor(const score::result::ErrorCode& errorCode) const noexcept override;
};

namespace global
{

static constexpr FlatCfgErrorDomain g_flatcfgErrorDomain {};

}  // namespace global

/// @brief Get a reference to a FlatCfg error domain object with static lifetime.
inline constexpr const score::result::ErrorDomain&
GetFlatCfgDomain() noexcept
{
    return global::g_flatcfgErrorDomain;
}

inline score::result::Error MakeError(const FlatCfgErrorCode code, const std::string_view message = "")
{
    return {static_cast<score::result::ErrorCode>(code), GetFlatCfgDomain(), message};
}

std::string_view
FlatCfgErrorDomain::MessageFor(const score::result::ErrorCode& errorCode) const noexcept
{
    const auto code { static_cast<FlatCfgErrorCode>(errorCode) };
    switch (code)
    {
        case FlatCfgErrorCode::kNotInitialized:
        {
            return "FlatCfg was not initialized before another function was called";
        }
        case FlatCfgErrorCode::kGeneric:
        {
            return "Generic internal error";
        }
        case FlatCfgErrorCode::kParameter:
        {
            return "Parameter error";
        }
        case FlatCfgErrorCode::kInvalidConfigPath:
        {
            return "Root path to the configuration files is invalid";
        }
        case FlatCfgErrorCode::kInvalidFileName:
        {
            return "Found filename that is not conform to the rules";
        }
        case FlatCfgErrorCode::kConfigFiles:
        {
            return "Handling the config files resulted in an error";
        }
        case FlatCfgErrorCode::kMultipleProcessesInPath:
        {
            return "Configuration folder contains configurations for more than one process";
        }
        case FlatCfgErrorCode::kMemoryAllocationFailed:
        {
            return "Failed to allocate memory";
        }
        case FlatCfgErrorCode::kMemoryAlreadyAllocated:
        {
            return "Memory is already allocated";
        }
        case FlatCfgErrorCode::kInvalidSWCLName:
        {
            return "Invalid SWCluster name";
        }
        case FlatCfgErrorCode::kFileNotFound:
        {
            return "File not found";
        }
        case FlatCfgErrorCode::kFileEmpty:
        {
            return "File is empty";
        }
        case FlatCfgErrorCode::kFileAccessError:
        {
            return "File access error";
        }
        case FlatCfgErrorCode::kInvalidBinaryFormat:
        {
            return "Invalid binary format";
        }
        case FlatCfgErrorCode::kBadName:
        {
            return "Unexpected function cluster name";
        }
        case FlatCfgErrorCode::kBadVersionMajor:
        {
            return "Unexpected major version";
        }
        case FlatCfgErrorCode::kBadVersionMinor:
        {
            return "Unexpected minor version";
        }
        case FlatCfgErrorCode::kInvalidDirectory:
        {
            return "Invalid directory";
        }
        case FlatCfgErrorCode::kBadLength:
        {
            return "The size of a buffer was too small to complete the operation";
        }
        case FlatCfgErrorCode::kBadPathName:
        {
            return "The path (or a component of the path) is empty, contains a"
                   " null byte, or is not null-terminated when it is required"
                   " to be";
        }
        case FlatCfgErrorCode::kBadArgument:
        {
            return "The value of a function argument did not meet the expected"
                   " pre-conditions";
        }
        case FlatCfgErrorCode::kHandleClosed:
        {
            return "Attempted to call a function on a closed file or directory"
                   " handle";
        }
        case FlatCfgErrorCode::kBadAlloc:
        {
            return "Failed to allocate memory";
        }
        case FlatCfgErrorCode::kOsError:
        {
            return "Calling an OS function resulted in an error that was not EINTR";
        }
        case FlatCfgErrorCode::kUnknown:
        default:
        {
            return "An unknown error occurred";
        }
    }
}

}  // namespace flatcfg

#endif  // FLATCFG_FLATCFG_ERROR_H
