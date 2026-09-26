include(platform/Cairo.cmake)
include(platform/FreeType.cmake)
include(platform/GCrypt.cmake)
include(platform/ImageDecoders.cmake)

list(APPEND WebCore_UNIFIED_SOURCE_LIST_FILES
    "SourcesWin98Mini.txt"
)

list(APPEND WebCore_PRIVATE_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/platform/network/win98mini"
    "${WEBCORE_DIR}/platform/text/icu"
    "${WEBCORE_DIR}/platform/video-codecs"
)

list(APPEND WebCore_PRIVATE_FRAMEWORK_HEADERS
    platform/network/win98mini/AuthenticationChallenge.h
    platform/network/win98mini/CertificateInfo.h
    platform/network/win98mini/ResourceError.h
    platform/network/win98mini/ResourceRequest.h
    platform/network/win98mini/ResourceResponse.h
    platform/win98mini/Win98MiniEditorClient.h
)

list(APPEND WebCore_LIBRARIES
    LibXml2::LibXml2
    SQLite::SQLite3
    ZLIB::ZLIB
)

list(APPEND WebCore_PRIVATE_DEFINITIONS
    BITMAP_TEXTURE_POOL_MAX_SIZE_IN_MB=16
)
