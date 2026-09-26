#include "config.h"
#include "MIMETypeRegistry.h"

#if defined(BUILDING_WIN98MINI__)

namespace WebCore {

static bool equalExtensionIgnoringASCIICase(StringView extension, ASCIILiteral literal)
{
    return equalIgnoringASCIICase(extension, literal);
}

String MIMETypeRegistry::mimeTypeForExtension(StringView extension)
{
    if (equalExtensionIgnoringASCIICase(extension, "html"_s) || equalExtensionIgnoringASCIICase(extension, "htm"_s))
        return "text/html"_s;
    if (equalExtensionIgnoringASCIICase(extension, "css"_s))
        return "text/css"_s;
    if (equalExtensionIgnoringASCIICase(extension, "js"_s) || equalExtensionIgnoringASCIICase(extension, "mjs"_s))
        return "text/javascript"_s;
    if (equalExtensionIgnoringASCIICase(extension, "json"_s))
        return "application/json"_s;
    if (equalExtensionIgnoringASCIICase(extension, "txt"_s))
        return "text/plain"_s;
    if (equalExtensionIgnoringASCIICase(extension, "png"_s))
        return "image/png"_s;
    if (equalExtensionIgnoringASCIICase(extension, "jpg"_s) || equalExtensionIgnoringASCIICase(extension, "jpeg"_s))
        return "image/jpeg"_s;
    if (equalExtensionIgnoringASCIICase(extension, "gif"_s))
        return "image/gif"_s;
    if (equalExtensionIgnoringASCIICase(extension, "webp"_s))
        return "image/webp"_s;
    if (equalExtensionIgnoringASCIICase(extension, "svg"_s))
        return "image/svg+xml"_s;
    return { };
}

bool MIMETypeRegistry::isApplicationPluginMIMEType(const String&)
{
    return false;
}

String MIMETypeRegistry::preferredExtensionForMIMEType(const String&)
{
    return { };
}

Vector<String> MIMETypeRegistry::extensionsForMIMEType(const String&)
{
    return { };
}

} // namespace WebCore

#endif // defined(BUILDING_WIN98MINI__)
