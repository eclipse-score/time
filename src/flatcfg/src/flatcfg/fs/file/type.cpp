/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "flatcfg/fs/_os.h"
#include "flatcfg/internal/fs/file/type.h"
#include "flatcfg/utils/logging.h"

#if FLATCFG_INTERNAL_HAS_POSIX
    #include <sys/stat.h>
#endif

#if FLATCFG_INTERNAL_HAS_LINUX || FLATCFG_INTERNAL_HAS_MACOS
    #include <dirent.h>
#endif

namespace flatcfg
{
namespace fs
{

FileType
fileTypeFromDirentType(unsigned int dtype) noexcept
{
#if FLATCFG_INTERNAL_HAS_LINUX || FLATCFG_INTERNAL_HAS_MACOS
    switch (dtype)
    {
        case DT_REG:
            return FileType::REGULAR;
        case DT_DIR:
            return FileType::DIRECTORY;
        case DT_CHR:
            return FileType::CHARACTER;
        case DT_BLK:
            return FileType::BLOCK;
        case DT_FIFO:
            return FileType::FIFO;
        case DT_LNK:
            return FileType::SYMLINK;
        case DT_SOCK:
            return FileType::SOCKET;
        default:
        {
            FLATCFG_LOG_DEBUG() <<
                "struct dirent.d_type value " << dtype << " does not match any"
                " known file type constant";
            return FileType::UNKNOWN;
        }
    }
#else
    FLATCFG_LOG_ERROR() <<
        "attempting to treat dtype value " << dtype << " as if it came from"
        " struct dirent.d_type on a platform which either does not support this"
        " field or does not support dirent";
    return FileType::UNKNOWN;
#endif
}

FileType
// RULECHECKER_comment(1, 1, check_max_control_nesting_depth, "Checking all enum labels.", true)
fileTypeFromStatMode(unsigned int mode) noexcept
{
#if FLATCFG_INTERNAL_HAS_POSIX
    // RULECHECKER_comment(1, 1, check_underlying_signedness_conversion, "Conversion is part of S_ISREG.", true)
    if (S_ISREG(mode))
    {
        return FileType::REGULAR;
    }
    // RULECHECKER_comment(1, 1, check_underlying_signedness_conversion, "Conversion is part of S_ISDIR.", true)
    else if (S_ISDIR(mode))
    {
        return FileType::DIRECTORY;
    }
    // RULECHECKER_comment(1, 1, check_underlying_signedness_conversion, "Conversion is part of S_ISCHR.", true)
    else if (S_ISCHR(mode))
    {
        return FileType::CHARACTER;
    }
    // RULECHECKER_comment(1, 1, check_underlying_signedness_conversion, "Conversion is part of S_ISBLK.", true)
    else if (S_ISBLK(mode))
    {
        return FileType::BLOCK;
    }
    // RULECHECKER_comment(1, 1, check_underlying_signedness_conversion, "Conversion is part of S_ISFIFO.", true)
    else if (S_ISFIFO(mode))
    {
        return FileType::FIFO;
    }
    // RULECHECKER_comment(1, 1, check_underlying_signedness_conversion, "Conversion is part of S_ISLINK.", true)
    else if (S_ISLNK(mode))
    {
        return FileType::SYMLINK;
    }
    // RULECHECKER_comment(1, 1, check_underlying_signedness_conversion, "Conversion is part of S_ISSOCK.", true)
    else if (S_ISSOCK(mode))
    {
        return FileType::SOCKET;
    }
    else
    {
        FLATCFG_LOG_DEBUG() <<
            "struct stat.st_mode value " << mode << " is not set to any known"
            " file type";
        return FileType::UNKNOWN;
    }
#else
    FLATCFG_LOG_ERROR() <<
        "attempting to treat mode value " << mode << " as of it came from"
        " struct stat.st_mode on a platform which does not support stat";
    return FileType::UNKNOWN;
#endif
}

}  // namespace fs
}  // namespace flatcfg
