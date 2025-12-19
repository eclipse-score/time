/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "flatcfg/fs/directory/entry_view.h"
#include "flatcfg/utils/logging.h"
#include "flatcfg/flatcfg_error.h"

#include "score/result/result.h"

#include <algorithm>

using score::Result;
using score::ResultBlank;

namespace flatcfg
{
namespace fs
{

Result<DirectoryEntryView>
// RULECHECKER_comment(1, 1, check_max_parameters, "4 parameters is acceptable here.", true)
DirectoryEntryView::FromValues(std::uint64_t id, std::uint64_t entrySize,
                               FileType type, ViewT name) noexcept
{
    ResultBlank res;
    DirectoryEntryView entryView { id, entrySize, type, name, &res };
    if (res)
    {
        return entryView;
    }
    else
    {
        return score::MakeUnexpected<DirectoryEntryView>(res.error());
    }
}

std::uint64_t
DirectoryEntryView::id() const noexcept
{
    return m_id;
}

std::uint64_t
DirectoryEntryView::entrySize() const noexcept
{
    return m_entrySize;
}

FileType
DirectoryEntryView::type() const noexcept
{
    return m_type;
}

DirectoryEntryView::ViewT
DirectoryEntryView::name() const noexcept
{
    return m_name;
}

// RULECHECKER_comment(1, 1, check_max_parameters, "5 parameters is acceptable here.", true)
DirectoryEntryView::DirectoryEntryView(std::uint64_t id, std::uint64_t entrySize,
                                       FileType type, ViewT name,
// coverity[autosar_cpp14_a8_4_10_violation:Intentional] Pointer to mark an out parameter.
                                       ResultBlank *resOut) noexcept
    : m_id(id),
      m_entrySize(entrySize),
      m_type(type),
      m_name(name)
{
    // check that the name is not empty
    // file paths are never empty
    if (name.empty())
    {
        FLATCFG_LOG_WARN() <<
            "entry name for entry id " << id << " is empty; paths cannot be"
            " empty";
        *resOut = score::MakeUnexpected(FlatCfgErrorCode::kBadPathName);
        return;
    }

    // check that the name does not contain null bytes
    // file paths cannot contain null bytes
    if (std::find(name.begin(), name.end(), '\0') != name.end())
    {
        FLATCFG_LOG_WARN() <<
            "entry name for entry id " << id << " contains null bytes; paths"
            " cannot contain null bytes; entry name (" << name.size() <<
            "): " << name;
        *resOut = score::MakeUnexpected(FlatCfgErrorCode::kBadPathName);
        return;
    }

    // check that the entry size is bigger than the name
    // this acts as a secondary check to prevent the entry from acting as a handle to
    //   heap-allocated memory instead of owning its data
    if (entrySize < name.size())
    {
        FLATCFG_LOG_WARN() <<
            "entry size " << entrySize << " for entry id " << id << " is"
            " smaller than the entry name (" << name.size() << "): " << name;
        *resOut = score::MakeUnexpected(FlatCfgErrorCode::kBadLength);
        return;
    }
}

}  // namespace fs
}  // namespace flatcfg
