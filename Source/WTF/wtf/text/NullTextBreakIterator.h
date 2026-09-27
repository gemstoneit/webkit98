/*
 * Copyright (C) 2017-2023 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#pragma once

#include <wtf/text/StringView.h>

namespace WTF {

class NullTextBreakIterator {
    WTF_DEPRECATED_MAKE_FAST_ALLOCATED(NullTextBreakIterator);
public:
    NullTextBreakIterator() = default;
#if defined(BUILDING_WIN98MINI__)
    NullTextBreakIterator(StringView string, std::span<const char16_t> priorContext) { setText(string, priorContext); }
#endif
    NullTextBreakIterator(const NullTextBreakIterator&) = delete;
    NullTextBreakIterator(NullTextBreakIterator&&) = default;
    NullTextBreakIterator& operator=(const NullTextBreakIterator&) = delete;
    NullTextBreakIterator& operator=(NullTextBreakIterator&&) = default;

    std::optional<unsigned> preceding(unsigned location) const
    {
#if defined(BUILDING_WIN98MINI__)
        if (!location)
            return { };
        return std::min(location - 1, m_length);
#else
        ASSERT_NOT_REACHED();
        return { };
#endif
    }

    std::optional<unsigned> following(unsigned location) const
    {
#if defined(BUILDING_WIN98MINI__)
        if (location >= m_length)
            return { };
        return location + 1;
#else
        ASSERT_NOT_REACHED();
        return { };
#endif
    }

    bool isBoundary(unsigned location) const
    {
#if defined(BUILDING_WIN98MINI__)
        return location <= m_length;
#else
        ASSERT_NOT_REACHED();
        return false;
#endif
    }

    void setText(StringView string, std::span<const char16_t>)
    {
#if defined(BUILDING_WIN98MINI__)
        m_length = string.length();
#else
        ASSERT_NOT_REACHED();
#endif
    }

#if defined(BUILDING_WIN98MINI__)
private:
    unsigned m_length { 0 };
#endif
};

}
