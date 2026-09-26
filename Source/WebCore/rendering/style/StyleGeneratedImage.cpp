/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003-2021 Apple Inc. All rights reserved.
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

#include "config.h"
#include "StyleGeneratedImage.h"

#include "Document.h"
#include "GeneratedImage.h"
#include "RenderElement.h"
#include "RenderObjectInlines.h"
#include "StyleResolver.h"
#include "Timer.h"
#include <wtf/TZoneMallocInlines.h>

namespace WebCore {

static const Seconds timeToKeepCachedGeneratedImages { 3_s };

#if (defined(WIN98MINI_WINDOW_HOST_SMOKE) && WIN98MINI_WINDOW_HOST_SMOKE) || (defined(BUILDING_WIN98MINI__) && BUILDING_WIN98MINI__)
static const TimerBase* s_win98MiniCachedGeneratedImageTimers[128];
static unsigned s_win98MiniCachedGeneratedImageTimerCount;

static void registerWin98MiniCachedGeneratedImageTimer(const TimerBase* timer)
{
    if (!timer)
        return;
    for (unsigned i = 0; i < s_win98MiniCachedGeneratedImageTimerCount; ++i) {
        if (s_win98MiniCachedGeneratedImageTimers[i] == timer)
            return;
    }
    if (s_win98MiniCachedGeneratedImageTimerCount >= sizeof(s_win98MiniCachedGeneratedImageTimers) / sizeof(s_win98MiniCachedGeneratedImageTimers[0]))
        return;
    s_win98MiniCachedGeneratedImageTimers[s_win98MiniCachedGeneratedImageTimerCount++] = timer;
}

static void unregisterWin98MiniCachedGeneratedImageTimer(const TimerBase* timer)
{
    if (!timer)
        return;
    for (unsigned i = 0; i < s_win98MiniCachedGeneratedImageTimerCount; ++i) {
        if (s_win98MiniCachedGeneratedImageTimers[i] != timer)
            continue;
        s_win98MiniCachedGeneratedImageTimers[i] = s_win98MiniCachedGeneratedImageTimers[--s_win98MiniCachedGeneratedImageTimerCount];
        s_win98MiniCachedGeneratedImageTimers[s_win98MiniCachedGeneratedImageTimerCount] = nullptr;
        return;
    }
}
#endif

// MARK: - CachedGeneratedImage

class StyleGeneratedImage::CachedGeneratedImage {
    WTF_MAKE_TZONE_ALLOCATED_INLINE(CachedGeneratedImage);
public:
    CachedGeneratedImage(StyleGeneratedImage&, FloatSize, GeneratedImage&);
    ~CachedGeneratedImage();
    GeneratedImage& image() const { return m_image; }
    void puntEvictionTimer() { m_evictionTimer.restart(); }

private:
    void evictionTimerFired();

    StyleGeneratedImage& m_owner;
    const FloatSize m_size;
    const Ref<GeneratedImage> m_image;
    DeferrableOneShotTimer m_evictionTimer;
};

inline StyleGeneratedImage::CachedGeneratedImage::CachedGeneratedImage(StyleGeneratedImage& owner, FloatSize size, GeneratedImage& image)
    : m_owner(owner)
    , m_size(size)
    , m_image(image)
    , m_evictionTimer(*this, &StyleGeneratedImage::CachedGeneratedImage::evictionTimerFired, timeToKeepCachedGeneratedImages)
{
    m_evictionTimer.restart();
#if (defined(WIN98MINI_WINDOW_HOST_SMOKE) && WIN98MINI_WINDOW_HOST_SMOKE) || (defined(BUILDING_WIN98MINI__) && BUILDING_WIN98MINI__)
    registerWin98MiniCachedGeneratedImageTimer(m_evictionTimer.win98MiniTimerBaseForDiagnostics());
#endif
}

StyleGeneratedImage::CachedGeneratedImage::~CachedGeneratedImage()
{
#if (defined(WIN98MINI_WINDOW_HOST_SMOKE) && WIN98MINI_WINDOW_HOST_SMOKE) || (defined(BUILDING_WIN98MINI__) && BUILDING_WIN98MINI__)
    unregisterWin98MiniCachedGeneratedImageTimer(m_evictionTimer.win98MiniTimerBaseForDiagnostics());
#endif
}

void StyleGeneratedImage::CachedGeneratedImage::evictionTimerFired()
{
    // NOTE: This is essentially a "delete this", the object is no longer valid after this line.
    m_owner.evictCachedGeneratedImage(m_size);
}

// MARK: - StyleGeneratedImage.

StyleGeneratedImage::StyleGeneratedImage(StyleImage::Type type, bool fixedSize)
    : StyleImage { type }
    , m_fixedSize { fixedSize }
{
}

StyleGeneratedImage::~StyleGeneratedImage() = default;

#if (defined(WIN98MINI_WINDOW_HOST_SMOKE) && WIN98MINI_WINDOW_HOST_SMOKE) || (defined(BUILDING_WIN98MINI__) && BUILDING_WIN98MINI__)
void StyleGeneratedImage::win98MiniForEachCachedGeneratedImageTimerForDiagnostics(void (*callback)(const TimerBase*, void*), void* context)
{
    if (!callback)
        return;
    for (unsigned i = 0; i < s_win98MiniCachedGeneratedImageTimerCount; ++i) {
        if (auto* timer = s_win98MiniCachedGeneratedImageTimers[i])
            callback(timer, context);
    }
}
#endif

GeneratedImage* StyleGeneratedImage::cachedImageForSize(FloatSize size)
{
    if (size.isEmpty())
        return nullptr;

    auto* cachedGeneratedImage = m_images.get(size);
    if (!cachedGeneratedImage)
        return nullptr;

    cachedGeneratedImage->puntEvictionTimer();
    return &cachedGeneratedImage->image();
}

void StyleGeneratedImage::saveCachedImageForSize(FloatSize size, GeneratedImage& image)
{
    ASSERT(!m_images.contains(size));
    m_images.add(size, makeUnique<CachedGeneratedImage>(*this, size, image));
}

void StyleGeneratedImage::evictCachedGeneratedImage(FloatSize size)
{
    ASSERT(m_images.contains(size));
    m_images.remove(size);
}

FloatSize StyleGeneratedImage::imageSize(const RenderElement* renderer, float multiplier) const
{
    if (!m_fixedSize)
        return m_containerSize;

    if (!renderer)
        return { };

    FloatSize fixedSize = this->fixedSize(*renderer);
    if (multiplier == 1.0f)
        return fixedSize;

    float width = fixedSize.width() * multiplier;
    float height = fixedSize.height() * multiplier;

    // Don't let images that have a width/height >= 1 shrink below 1 device pixel when zoomed.
    float deviceScaleFactor = renderer->document().deviceScaleFactor();
    if (fixedSize.width() > 0)
        width = std::max<float>(1 / deviceScaleFactor, width);
    if (fixedSize.height() > 0)
        height = std::max<float>(1 / deviceScaleFactor, height);

    return { width, height };
}

void StyleGeneratedImage::computeIntrinsicDimensions(const RenderElement* renderer, float& intrinsicWidth, float& intrinsicHeight, FloatSize& intrinsicRatio)
{
    // At a zoom level of 1 the image is guaranteed to have a device pixel size.
    FloatSize size = floorSizeToDevicePixels(LayoutSize(this->imageSize(renderer, 1)), renderer ? renderer->document().deviceScaleFactor() : 1);
    intrinsicWidth = size.width();
    intrinsicHeight = size.height();
    intrinsicRatio = size;
}

// MARK: Client support.

void StyleGeneratedImage::addClient(RenderElement& renderer)
{
    if (m_clients.isEmptyIgnoringNullReferences())
        ref();

    m_clients.add(renderer);

    this->didAddClient(renderer);
}

void StyleGeneratedImage::removeClient(RenderElement& renderer)
{
    ASSERT(m_clients.contains(renderer));
    if (!m_clients.remove(renderer))
        return;

    this->didRemoveClient(renderer);

    if (m_clients.isEmptyIgnoringNullReferences())
        deref();
}

bool StyleGeneratedImage::hasClient(RenderElement& renderer) const
{
    return m_clients.contains(renderer);
}

} // namespace WebCore
