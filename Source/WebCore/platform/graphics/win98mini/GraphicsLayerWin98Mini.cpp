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
#include "GraphicsLayer.h"

#include "GraphicsLayerFactory.h"

namespace WebCore {

class GraphicsLayerWin98Mini final : public GraphicsLayer {
public:
    GraphicsLayerWin98Mini(Type type, GraphicsLayerClient& client)
        : GraphicsLayer(type, client)
    {
    }

    ~GraphicsLayerWin98Mini() final
    {
        willBeDestroyed();
    }

    void setNeedsDisplay() final
    {
        if (drawsContent())
            addRepaintRect({ { }, size() });
    }

    void setNeedsDisplayInRect(const FloatRect& rect, ShouldClipToLayer) final
    {
        if (drawsContent())
            addRepaintRect(rect);
    }
};

Ref<GraphicsLayer> GraphicsLayer::create(GraphicsLayerFactory* factory, GraphicsLayerClient& client, Type layerType)
{
    if (factory)
        return factory->createGraphicsLayer(layerType, client);

    return adoptRef(*new GraphicsLayerWin98Mini(layerType, client));
}

} // namespace WebCore
