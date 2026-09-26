#include "config.h"
#include "AXObjectCache.h"

#if defined(BUILDING_WIN98MINI__)

#include "AccessibilityObject.h"

namespace WebCore {

void AXObjectCache::attachWrapper(AccessibilityObject& object)
{
    auto wrapper = adoptRef(*new AccessibilityObjectWrapper());
    object.setWrapper(wrapper.ptr());
}

void AXObjectCache::detachWrapper(AXCoreObject*, AccessibilityDetachmentType)
{
}

void AXObjectCache::postPlatformNotification(AccessibilityObject&, AXNotification)
{
}

void AXObjectCache::nodeTextChangePlatformNotification(AccessibilityObject*, AXTextChange, unsigned, const String&)
{
}

void AXObjectCache::frameLoadingEventPlatformNotification(RenderView*, AXLoadingEvent)
{
}

void AXObjectCache::handleScrolledToAnchor(const Node&)
{
}

void AXObjectCache::platformHandleFocusedUIElementChanged(Element*, Element*)
{
}

void AXObjectCache::platformPerformDeferredCacheUpdate()
{
}

} // namespace WebCore

#endif // defined(BUILDING_WIN98MINI__)
