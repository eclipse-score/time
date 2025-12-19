/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_FS_FILE_TYPE_H
#define FLATCFG_FS_FILE_TYPE_H

namespace flatcfg
{
namespace fs
{

/// @brief Represents the type of a file.
enum class FileType
{
    /// @brief The file type could not be determined.
    UNKNOWN = 0

    /// @brief This is a regular file.
    ,REGULAR

    /// @brief This is a directory.
    ,DIRECTORY

    /// @brief This is a character device.
    ,CHARACTER

    /// @brief This is a block device.
    ,BLOCK

    /// @brief This is a named pipe (FIFO).
    ,FIFO

    /// @brief This is a symbolic link.
    ,SYMLINK

    /// @brief This is a socket.
    ,SOCKET
};

}  // namespace fs
}  // namespace flatcfg

#endif  // FLATCFG_FS_FILE_TYPE_H
