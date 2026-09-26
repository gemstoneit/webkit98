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
#include "NetworkStorageSession.h"

#include "Cookie.h"
#include "CookieRequestHeaderFieldProxy.h"
#include "HTTPCookieAcceptPolicy.h"
#include "NetworkingContext.h"
#include <pal/SessionID.h>

namespace WebCore {

NetworkStorageSession::NetworkStorageSession(PAL::SessionID sessionID, NetworkingContext* context)
    : m_sessionID(sessionID)
    , m_context(context)
{
}

NetworkStorageSession::~NetworkStorageSession() = default;

NetworkingContext* NetworkStorageSession::context() const
{
    return m_context.get();
}

HTTPCookieAcceptPolicy NetworkStorageSession::cookieAcceptPolicy() const
{
    return HTTPCookieAcceptPolicy::Never;
}

void NetworkStorageSession::setCookie(const Cookie&)
{
}

void NetworkStorageSession::setCookie(const Cookie&, const URL&, const URL&)
{
}

void NetworkStorageSession::setCookiesFromDOM(const URL&, const SameSiteInfo&, const URL&, std::optional<FrameIdentifier>, std::optional<PageIdentifier>, ApplyTrackingPrevention, RequiresScriptTrackingPrivacy, const String&, ShouldRelaxThirdPartyCookieBlocking, IsKnownCrossSiteTracker) const
{
}

void NetworkStorageSession::deleteCookie(const Cookie&, CompletionHandler<void()>&& completionHandler)
{
    completionHandler();
}

void NetworkStorageSession::deleteCookie(const URL&, const URL&, const String&, CompletionHandler<void()>&& completionHandler) const
{
    completionHandler();
}

void NetworkStorageSession::deleteCookiesForHostnames(const Vector<String>&, IncludeHttpOnlyCookies, ScriptWrittenCookiesOnly, CompletionHandler<void()>&& completionHandler)
{
    completionHandler();
}

bool NetworkStorageSession::getRawCookies(const URL&, const SameSiteInfo&, const URL&, std::optional<FrameIdentifier>, std::optional<PageIdentifier>, ApplyTrackingPrevention, ShouldRelaxThirdPartyCookieBlocking, Vector<Cookie>& rawCookies) const
{
    rawCookies.clear();
    return false;
}

std::pair<String, bool> NetworkStorageSession::cookiesForDOM(const URL&, const SameSiteInfo&, const URL&, std::optional<FrameIdentifier>, std::optional<PageIdentifier>, IncludeSecureCookies, ApplyTrackingPrevention, ShouldRelaxThirdPartyCookieBlocking, IsKnownCrossSiteTracker) const
{
    return { emptyString(), false };
}

std::pair<String, bool> NetworkStorageSession::cookieRequestHeaderFieldValue(const URL&, const SameSiteInfo&, const URL&, std::optional<FrameIdentifier>, std::optional<PageIdentifier>, IncludeSecureCookies, ApplyTrackingPrevention, ShouldRelaxThirdPartyCookieBlocking, IsKnownCrossSiteTracker) const
{
    return { emptyString(), false };
}

std::pair<String, bool> NetworkStorageSession::cookieRequestHeaderFieldValue(const CookieRequestHeaderFieldProxy&) const
{
    return { emptyString(), false };
}

} // namespace WebCore
