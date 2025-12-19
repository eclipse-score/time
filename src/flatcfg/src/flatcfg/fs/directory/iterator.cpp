/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "flatcfg/fs/directory/iterator.h"
#include "flatcfg/utils/ctc.h"
#include "flatcfg/utils/logging.h"
#include "flatcfg/flatcfg_error.h"

#include "score/time/common/Abort.h"

#include <cstddef>
#include <iterator>

using score::cpp::span;
using score::time::common::logFatalAndAbort;

FLATCFG_SKIPCOV_BEGIN();

namespace flatcfg
{
namespace fs
{

score::Result<DirectoryIterator>
DirectoryIterator::FromPath(std::string_view directoryPath) noexcept
{
    score::ResultBlank res;
    DirectoryIterator it { directoryPath, &res };
    if (res)
    {
        return it;
    }
    else
    {
        return score::MakeUnexpected<DirectoryIterator>(res.error());
    }
}

DirectoryIterator::DirectoryIterator() noexcept = default;
DirectoryIterator::DirectoryIterator(DirectoryIterator&& other) noexcept = default;
DirectoryIterator& DirectoryIterator::operator=(DirectoryIterator&& other) noexcept = default;

DirectoryIterator&
DirectoryIterator::operator++() noexcept
{
    // undefined behaviour to increment an iterator in error state
    FLATCFG_SKIPCOV_BEGIN();
    if (isError())
    {
        logFatalAndAbort("Incremented flatcfg::fs::DirectoryIterator in error state");
    }
    FLATCFG_SKIPCOV_END();

    // skip if we're a sentinel
    if (isSentinel())
    {
        return *this;
    }

    // set position to next entry, or the end of the buffer
    auto oldEntryViewRes = getCurrentEntryView();
    if (!oldEntryViewRes)
    {
        m_handleOptRes = score::MakeUnexpected<std::optional<DirectoryHandle>>(oldEntryViewRes.error());
        return *this;
    }
    DirectoryEntryView oldEntryView = *oldEntryViewRes;
    // coverity[autosar_cpp14_a4_7_1_violation:Intentional] Will not wrap due to program design.
    m_bufferPos += oldEntryView.entrySize();

    // make sure the next entry is valid (or become a sentinel)
    do {

        // if we finished our buffer, refill it
        if (m_bufferPos == m_bufferSize)
        {
            // read data from our handle into tthe buffer
            span<char> span { m_buffer.data(), m_buffer.size() };
            auto refillRes = (*m_handleOptRes)->readEntriesIntoBuffer(span);
            if (!refillRes)
            {
                m_handleOptRes = score::MakeUnexpected<std::optional<DirectoryHandle>>(refillRes.error());
                return *this;
            }
            m_bufferPos = 0U;
            m_bufferSize = *refillRes;
        }

        // if no data was read, there's no entries left, so we become a sentinel
        if (m_bufferSize == 0U)
        {
            m_handleOptRes = std::nullopt;
            return *this;
        }

        // skip invalid entries in the current remaining buffer
        auto skipRes = skipInvalidEntries();
        if (!skipRes)
        {
            m_handleOptRes = score::MakeUnexpected<std::optional<DirectoryHandle>>(skipRes.error());
            return *this;
        }
    }
    // all entries in the buffer were invalid and we need a new buffer
    while (m_bufferPos == m_bufferSize);

    // we either got a valid entry, became a sentinel, or encountered an error
    return *this;
}

void
DirectoryIterator::operator++(int) noexcept
{
    // better to have the abort here for easier debugging
    // undefined behaviour to increment an iterator in error state
    FLATCFG_SKIPCOV_BEGIN();
    if (isError())
    {
        logFatalAndAbort("Incremented flatcfg::fs::DirectoryIterator in error state");
    }
    FLATCFG_SKIPCOV_END();

    // do increment
    ++(*this);
}

DirectoryIterator::value_type
DirectoryIterator::operator*() noexcept
{
    if (!m_handleOptRes)
    {
        // propagate error
        return score::MakeUnexpected<DirectoryEntry>(m_handleOptRes.error());
    }
    else if (!m_handleOptRes->has_value())
    {
        // undefined behaviour to dereference a sentinel iterator
        FLATCFG_SKIPCOV_BEGIN();
        logFatalAndAbort("Dereferenced sentinel flatcfg::fs::DirectoryIterator");
        FLATCFG_SKIPCOV_END();
    }
    else
    {
        // parse current entry view
        auto entryViewRes = getCurrentEntryView();
        if (!entryViewRes)
        {
            m_handleOptRes = score::MakeUnexpected<std::optional<DirectoryHandle>>(entryViewRes.error());
            return score::MakeUnexpected<DirectoryEntry>(entryViewRes.error());
        }
        DirectoryEntryView entryView = *entryViewRes;

        // convert entry view to entry and return
        auto entryRes = DirectoryEntry::FromView(entryView);
        if (!entryRes)
        {
            m_handleOptRes = score::MakeUnexpected<std::optional<DirectoryHandle>>(entryRes.error());
            return score::MakeUnexpected<DirectoryEntry>(entryRes.error());
        }

        // success
        return *entryRes;
    }
}

bool
DirectoryIterator::isError() const noexcept
{
    return !m_handleOptRes;
}

bool
DirectoryIterator::isSentinel() const noexcept
{
    return m_handleOptRes && !m_handleOptRes->has_value();
}

// RULECHECKER_comment(1, 1, check_non_explicit_conversion_function, "Non-explicit for easier usage.", true)
DirectoryIterator::operator bool() const noexcept
{
    return !(isError() || isSentinel());
}

// coverity[autosar_cpp14_a8_4_10_violation:Intentional] Pointer to mark an out parameter.
DirectoryIterator::DirectoryIterator(std::string_view directoryPath, score::ResultBlank *resOut) noexcept
{
    // open the directory handle
    auto handleRes = DirectoryHandle::FromPath(directoryPath);
    if (!handleRes)
    {
        *resOut = score::MakeUnexpected<score::Blank>(handleRes.error());
        return;
    }
    m_handleOptRes = *(std::move(handleRes));

    // make sure the next (a.k.a. first) entry is valid (or become a sentinel)
    do {

        // if we finished our buffer, refill it
        if (m_bufferPos == m_bufferSize)
        {
            // read data from handle into buffer
            span<char> span { m_buffer.data(), m_buffer.size() };
            auto refillRes = (*m_handleOptRes)->readEntriesIntoBuffer(span);
            if (!refillRes)
            {
                *resOut = score::MakeUnexpected<score::Blank>(refillRes.error());
                return;
            }
            m_bufferPos = 0U;
            m_bufferSize = *refillRes;
        }

        // if no data was read, there's no entries left so we become a sentinel
        if (m_bufferSize == 0U)
        {
            m_handleOptRes = std::nullopt;
            return;
        }

        // skip invalid entries in the current remaining buffer
        auto skipRes = skipInvalidEntries();
        if (!skipRes)
        {
            *resOut = score::MakeUnexpected<score::Blank>(skipRes.error());
            return;
        }
    }
    // all entries in the buffer were invalid and we need a new buffer
    while (m_bufferPos == m_bufferSize);

    // we either got a valid entry, or became a sentinel
    // if we encountered an error, we would have returned immediately
}

score::Result<DirectoryEntryView>
DirectoryIterator::getCurrentEntryView() const noexcept
{
    // create span from remaining buffer
    std::ptrdiff_t bufferSpos = static_cast<std::ptrdiff_t>(m_bufferPos);
    const char *ptr = std::next(m_buffer.data(), bufferSpos);
    // coverity[autosar_cpp14_a4_7_1_violation:Intentional] Will not wrap due to program design.
    std::size_t size = m_bufferSize - m_bufferPos;
    span<const char> span { ptr, size };

    // create entry view
    auto entryViewRes = (*m_handleOptRes)->parseEntryFromBuffer(span);
    if (!entryViewRes)
    {
        return score::MakeUnexpected<DirectoryEntryView>(entryViewRes.error());
    }
    else
    {
        return *entryViewRes;
    }
}

score::ResultBlank
DirectoryIterator::skipInvalidEntries() noexcept
{
    // go through all our entries
    while (m_handleOptRes &&
           m_handleOptRes->has_value() &&
           (m_bufferPos < m_bufferSize))
    {
        // get the current entry
        auto entryViewRes = getCurrentEntryView();
        if (!entryViewRes)
        {
            return score::MakeUnexpected<score::Blank>(entryViewRes.error());
        }
        DirectoryEntryView entryView = *entryViewRes;

        // break if the entry is valid
        // an id of 0 signifies an invalid or deleted entry
        if (entryView.id() != 0U)
        {
            break;
        }

        // skip to next entry
        m_bufferPos += entryView.entrySize();
    }

    // found valid entry or skipped all entries in buffer
    return {};
}

bool
operator==(const DirectoryIterator& lhs, const DirectoryIterator& rhs) noexcept
{
    // an iterator always compares equal to itself, even if it's an error
    if (&lhs == &rhs)
    {
        return true;
    }

    // errors always compare unequal to everything
    if (!lhs.m_handleOptRes || !rhs.m_handleOptRes)
    {
        return false;
    }

    // sentinels always compare equal to other sentinels
    if (!lhs.m_handleOptRes->has_value() && !rhs.m_handleOptRes->has_value())
    {
        return true;
    }

    // sentinels always compare unequal to non-sentinels
    if (!lhs.m_handleOptRes->has_value() || !rhs.m_handleOptRes->has_value())
    {
        return false;
    }

    // parse entry views
    auto lhsEntryViewRes = lhs.getCurrentEntryView();
    auto rhsEntryViewRes = rhs.getCurrentEntryView();

    // any error in the parsing compares unequal
    if (!lhsEntryViewRes || !rhsEntryViewRes)
    {
        return false;
    }

    // get values from results
    DirectoryEntryView lhsEntryView = *lhsEntryViewRes;
    DirectoryEntryView rhsEntryView = *rhsEntryViewRes;

    // compare members
    const bool sameId = lhsEntryView.id() == rhsEntryView.id();
    const bool sameName = lhsEntryView.name() == rhsEntryView.name();
    return sameId && sameName;
}

bool
operator!=(const DirectoryIterator& lhs, const DirectoryIterator& rhs) noexcept
{
    return !(lhs == rhs);
}

}  // namespace fs
}  // namespace flatcfg

FLATCFG_SKIPCOV_END();
