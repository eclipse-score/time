/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_INTERNAL_FS_FILE_TYPE_H
#define FLATCFG_INTERNAL_FS_FILE_TYPE_H

#include "flatcfg/fs/file/type.h"

namespace flatcfg
{
namespace fs
{

/// @brief Convert the value of a dirent.d_type to FileType.
FileType
fileTypeFromDirentType(unsigned int dtype) noexcept;

/// @brief Convert the value of a stat.st_mode to FileType.
FileType
fileTypeFromStatMode(unsigned int mode) noexcept;

}  // namespace fs
}  // namespace flatcfg

#endif  // FLATCFG_INTERNAL_FS_FILE_TYPE_H
