#include "config.h"
#include "PlatformScreen.h"

#if defined(BUILDING_WIN98MINI__)

#include "DestinationColorSpace.h"
#include "FloatRect.h"

namespace WebCore {

int screenDepth(Widget*)
{
    return 24;
}

int screenDepthPerComponent(Widget*)
{
    return 8;
}

bool screenIsMonochrome(Widget*)
{
    return false;
}

bool screenHasInvertedColors()
{
    return false;
}

FloatRect screenRect(Widget*)
{
    return { 0, 0, 800, 600 };
}

FloatRect screenAvailableRect(Widget*)
{
    return screenRect(nullptr);
}

bool screenSupportsExtendedColor(Widget*)
{
    return false;
}

DestinationColorSpace screenColorSpace(Widget*)
{
    return DestinationColorSpace::SRGB();
}

} // namespace WebCore

#endif // defined(BUILDING_WIN98MINI__)
