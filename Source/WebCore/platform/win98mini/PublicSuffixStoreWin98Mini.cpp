/*
 * Copyright (C) 2026 Gemstone IT Services Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "config.h"
#include "PublicSuffixStore.h"

namespace WebCore {

bool PublicSuffixStore::platformIsPublicSuffix(StringView domain) const
{
    return !domain.isEmpty() && !domain.contains('.');
}

String PublicSuffixStore::platformTopPrivatelyControlledDomain(StringView domain) const
{
    unsigned position = 0;
    while (position < domain.length() && domain[position] == '.')
        position++;

    auto trimmedDomain = domain.substring(position);
    if (trimmedDomain.isEmpty())
        return { };

    auto cursor = trimmedDomain.length();
    bool foundDot = false;
    while (cursor-- > 0) {
        if (trimmedDomain[cursor] != '.')
            continue;
        if (foundDot)
            return trimmedDomain.substring(cursor + 1).toString();
        foundDot = true;
    }

    return foundDot ? trimmedDomain.toString() : String();
}

} // namespace WebCore
