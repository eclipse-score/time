/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "flatcfg/fs/directory/entry.h"
#include "flatcfg/utils/logging.h"
#include "flatcfg/flatcfg_error.h"

#include "score/result/error.h"
#include "score/result/result.h"

#include <algorithm>
#include <iterator>

using score::Result;
using score::ResultBlank;

namespace flatcfg
{
namespace fs
{

Result<DirectoryEntry>
DirectoryEntry::FromView(DirectoryEntryView view) noexcept
{
    ResultBlank res;
    DirectoryEntry entry { view, &res };
    if (res)
    {
        return entry;
    }
    else
    {
        return score::MakeUnexpected<DirectoryEntry>(res.error());
    }
}

std::string_view
DirectoryEntry::name() const noexcept
{
    return std::string_view { m_nameBuffer.data(), m_nameSize };
}

FileType
DirectoryEntry::type() const noexcept
{
    return m_type;
}

// coverity[autosar_cpp14_a8_4_10_violation:Intentional] Pointer to mark an out parameter.
DirectoryEntry::DirectoryEntry(DirectoryEntryView view, ResultBlank *resOut) noexcept
    : m_nameSize(view.name().size()),
      m_type(view.type())
{
    // check name size
    if (m_nameSize > _os::MAX_NAME_LENGTH)
    {
        FLATCFG_LOG_WARN() <<
            "directory entry name size (" << view.name().size() << ") exceeds"
            " limit of " << _os::MAX_NAME_LENGTH << " bytes: " << view.name();
        *resOut = score::MakeUnexpected<score::Blank>(FlatCfgErrorCode::kBadLength);
        return;
    }

    // copy over name
    // character type might be wchar_t and not char on non-posix platforms
#if FLATCFG_INTERNAL_HAS_POSIX
    std::ptrdiff_t nameSsize = static_cast<std::ptrdiff_t>(m_nameSize);
    const char *namePtrBegin = view.name().data();
    const char *namePtrEnd = std::next(namePtrBegin, nameSsize);
    char *namePtrOut = m_nameBuffer.data();
    namePtrOut = std::copy(namePtrBegin, namePtrEnd, namePtrOut);
    *namePtrOut = '\0';
#else
    #error Unsupported OS
#endif
}

}  // namespace fs
}  // namespace flatcfg
