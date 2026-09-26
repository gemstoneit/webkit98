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
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PlatformLocale.h"

#include <wtf/DateMath.h>

namespace WebCore {

class LocaleWin98Mini final : public Locale {
public:
    ~LocaleWin98Mini() final = default;

private:
    void initializeLocaleData() final { }
    String dateFormat() final { return "yyyy-MM-dd"_s; }
    String monthFormat() final { return "yyyy-MM"_s; }
    String shortMonthFormat() final { return "yyyy-MM"_s; }
    String timeFormat() final { return "HH:mm:ss"_s; }
    String shortTimeFormat() final { return "HH:mm"_s; }
    String dateTimeFormatWithSeconds() final { return "yyyy-MM-dd'T'HH:mm:ss"_s; }
    String dateTimeFormatWithoutSeconds() final { return "yyyy-MM-dd'T'HH:mm"_s; }
    const Vector<String>& monthLabels() final;
    const Vector<String>& shortMonthLabels() final;
    const Vector<String>& standAloneMonthLabels() final { return monthLabels(); }
    const Vector<String>& shortStandAloneMonthLabels() final { return shortMonthLabels(); }
    const Vector<String>& timeAMPMLabels() final;

    Vector<String> m_timeAMPMLabels;
    Vector<String> m_shortMonthLabels;
    Vector<String> m_monthLabels;
};

std::unique_ptr<Locale> Locale::create(const AtomString&)
{
    return makeUnique<LocaleWin98Mini>();
}

const Vector<String>& LocaleWin98Mini::monthLabels()
{
    if (m_monthLabels.isEmpty()) {
        for (auto& month : WTF::monthFullName)
            m_monthLabels.append(month);
    }
    return m_monthLabels;
}

const Vector<String>& LocaleWin98Mini::shortMonthLabels()
{
    if (m_shortMonthLabels.isEmpty()) {
        for (auto& month : WTF::monthName)
            m_shortMonthLabels.append(month);
    }
    return m_shortMonthLabels;
}

const Vector<String>& LocaleWin98Mini::timeAMPMLabels()
{
    if (m_timeAMPMLabels.isEmpty()) {
        m_timeAMPMLabels.append("AM"_s);
        m_timeAMPMLabels.append("PM"_s);
    }
    return m_timeAMPMLabels;
}

} // namespace WebCore
