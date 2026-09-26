#include "config.h"

#if defined(WEBKIT_WINDOWS_LEGACY_TARGET) && OS(WINDOWS)

#include <cerrno>
#include <time.h>
#include <windows.h>
#include <wincrypt.h>

extern "C" {

static size_t win98TraceLength(const char* message)
{
    size_t length = 0;
    while (message[length])
        ++length;
    return length;
}

static bool win98TraceEnabled()
{
    static int enabled = -1;
    if (enabled >= 0)
        return enabled;

    char value[16];
    DWORD valueLength = GetEnvironmentVariableA("WIN98_TRACE", value, sizeof(value));
    enabled = valueLength && !(valueLength == 1 && value[0] == '0');
    return enabled;
}

void win98Trace(const char* message)
{
    if (!win98TraceEnabled())
        return;

    DWORD written = 0;
    HANDLE stderrHandle = GetStdHandle(STD_ERROR_HANDLE);
    if (stderrHandle != INVALID_HANDLE_VALUE && stderrHandle)
        WriteFile(stderrHandle, message, static_cast<DWORD>(win98TraceLength(message)), &written, nullptr);
    if (stderrHandle != INVALID_HANDLE_VALUE && stderrHandle)
        WriteFile(stderrHandle, "\r\n", 2, &written, nullptr);

    auto appendToFile = [&] (const char* path) {
        HANDLE file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE)
            return;

        SetFilePointer(file, 0, nullptr, FILE_END);
        WriteFile(file, message, static_cast<DWORD>(win98TraceLength(message)), &written, nullptr);
        WriteFile(file, "\r\n", 2, &written, nullptr);
        FlushFileBuffers(file);
        CloseHandle(file);
    };

    appendToFile("C:\\JSC\\JSCWIN98.LOG");
    appendToFile("JSCWIN98.LOG");
}

void win98TraceNoConsole(const char* message)
{
    if (!win98TraceEnabled())
        return;

    HANDLE file = CreateFileA("C:\\JSC\\JSCWIN98.LOG", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return;

    DWORD written = 0;
    SetFilePointer(file, 0, nullptr, FILE_END);
    WriteFile(file, message, static_cast<DWORD>(win98TraceLength(message)), &written, nullptr);
    WriteFile(file, "\r\n", 2, &written, nullptr);
    FlushFileBuffers(file);
    CloseHandle(file);
}

struct Win98TraceStartup {
    Win98TraceStartup()
    {
        win98Trace("shim: static constructor");
    }
};

static Win98TraceStartup win98TraceStartup;

static BOOL WINAPI win98CreateHardLinkWUnavailable(LPCWSTR, LPCWSTR, LPSECURITY_ATTRIBUTES)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    errno = ENOSYS;
    return FALSE;
}

void* win98CreateHardLinkWImportPointer asm("__imp__CreateHardLinkW@12") = reinterpret_cast<void*>(win98CreateHardLinkWUnavailable);

static HMODULE WINAPI win98LoadLibraryW(LPCWSTR path)
{
    char narrowPath[MAX_PATH];

    if (!path)
        return nullptr;

    if (!WideCharToMultiByte(CP_ACP, 0, path, -1, narrowPath, sizeof(narrowPath), nullptr, nullptr))
        return nullptr;

    return LoadLibraryA(narrowPath);
}

HMODULE WINAPI win98LoadLibraryWFunction(LPCWSTR path) asm("_LoadLibraryW@4");
HMODULE WINAPI win98LoadLibraryWFunction(LPCWSTR path)
{
    return win98LoadLibraryW(path);
}

void* win98LoadLibraryWImportPointer asm("__imp__LoadLibraryW@4") = reinterpret_cast<void*>(win98LoadLibraryW);

static BOOL win98WideToAnsi(LPCWSTR wide, char* narrow, DWORD narrowLength)
{
    if (!wide) {
        if (narrow && narrowLength)
            narrow[0] = '\0';
        return TRUE;
    }

    if (!narrow || !narrowLength) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    return WideCharToMultiByte(CP_ACP, 0, wide, -1, narrow, narrowLength, nullptr, nullptr) != 0;
}

static BOOL win98AnsiToWide(LPCSTR narrow, LPWSTR wide, DWORD wideLength)
{
    if (!narrow) {
        if (wide && wideLength)
            wide[0] = L'\0';
        return TRUE;
    }

    if (!wide || !wideLength) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    return MultiByteToWideChar(CP_ACP, 0, narrow, -1, wide, wideLength) != 0;
}

static DWORD win98WideLength(LPCWSTR text)
{
    DWORD length = 0;
    if (!text)
        return 0;
    while (text[length])
        ++length;
    return length;
}

static void win98FindDataAToW(const WIN32_FIND_DATAA& source, WIN32_FIND_DATAW& target)
{
    target.dwFileAttributes = source.dwFileAttributes;
    target.ftCreationTime = source.ftCreationTime;
    target.ftLastAccessTime = source.ftLastAccessTime;
    target.ftLastWriteTime = source.ftLastWriteTime;
    target.nFileSizeHigh = source.nFileSizeHigh;
    target.nFileSizeLow = source.nFileSizeLow;
    target.dwReserved0 = source.dwReserved0;
    target.dwReserved1 = source.dwReserved1;
    win98AnsiToWide(source.cFileName, target.cFileName, MAX_PATH);
    win98AnsiToWide(source.cAlternateFileName, target.cAlternateFileName, 14);
}

static HANDLE WINAPI win98CreateEventW(LPSECURITY_ATTRIBUTES attributes, BOOL manualReset, BOOL initialState, LPCWSTR name)
{
    char narrowName[MAX_PATH];
    LPCSTR nameA = nullptr;
    if (name) {
        if (!win98WideToAnsi(name, narrowName, sizeof(narrowName)))
            return nullptr;
        nameA = narrowName;
    }
    return CreateEventA(attributes, manualReset, initialState, nameA);
}

static HANDLE WINAPI win98CreateFileMappingW(HANDLE file, LPSECURITY_ATTRIBUTES attributes, DWORD protect, DWORD maximumSizeHigh, DWORD maximumSizeLow, LPCWSTR name)
{
    char narrowName[MAX_PATH];
    LPCSTR nameA = nullptr;
    if (name) {
        if (!win98WideToAnsi(name, narrowName, sizeof(narrowName)))
            return nullptr;
        nameA = narrowName;
    }
    return CreateFileMappingA(file, attributes, protect, maximumSizeHigh, maximumSizeLow, nameA);
}

static HANDLE WINAPI win98CreateFileW(LPCWSTR path, DWORD desiredAccess, DWORD shareMode, LPSECURITY_ATTRIBUTES attributes, DWORD creationDisposition, DWORD flagsAndAttributes, HANDLE templateFile)
{
    char narrowPath[MAX_PATH];
    if (!win98WideToAnsi(path, narrowPath, sizeof(narrowPath)))
        return INVALID_HANDLE_VALUE;
    return CreateFileA(narrowPath, desiredAccess, shareMode, attributes, creationDisposition, flagsAndAttributes, templateFile);
}

static HANDLE WINAPI win98CreateMutexW(LPSECURITY_ATTRIBUTES attributes, BOOL initialOwner, LPCWSTR name)
{
    char narrowName[MAX_PATH];
    LPCSTR nameA = nullptr;
    if (name) {
        if (!win98WideToAnsi(name, narrowName, sizeof(narrowName)))
            return nullptr;
        nameA = narrowName;
    }
    return CreateMutexA(attributes, initialOwner, nameA);
}

static HANDLE WINAPI win98CreateSemaphoreW(LPSECURITY_ATTRIBUTES attributes, LONG initialCount, LONG maximumCount, LPCWSTR name)
{
    char narrowName[MAX_PATH];
    LPCSTR nameA = nullptr;
    if (name) {
        if (!win98WideToAnsi(name, narrowName, sizeof(narrowName)))
            return nullptr;
        nameA = narrowName;
    }
    return CreateSemaphoreA(attributes, initialCount, maximumCount, nameA);
}

static BOOL WINAPI win98DeleteFileW(LPCWSTR path)
{
    char narrowPath[MAX_PATH];
    if (!win98WideToAnsi(path, narrowPath, sizeof(narrowPath)))
        return FALSE;
    return DeleteFileA(narrowPath);
}

static HANDLE WINAPI win98FindFirstFileW(LPCWSTR path, LPWIN32_FIND_DATAW data)
{
    char narrowPath[MAX_PATH];
    WIN32_FIND_DATAA dataA;
    if (!data || !win98WideToAnsi(path, narrowPath, sizeof(narrowPath)))
        return INVALID_HANDLE_VALUE;
    HANDLE handle = FindFirstFileA(narrowPath, &dataA);
    if (handle != INVALID_HANDLE_VALUE)
        win98FindDataAToW(dataA, *data);
    return handle;
}

static BOOL WINAPI win98FindNextFileW(HANDLE handle, LPWIN32_FIND_DATAW data)
{
    WIN32_FIND_DATAA dataA;
    if (!data) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    BOOL result = FindNextFileA(handle, &dataA);
    if (result)
        win98FindDataAToW(dataA, *data);
    return result;
}

static DWORD WINAPI win98FormatMessageW(DWORD flags, LPCVOID source, DWORD messageID, DWORD languageID, LPWSTR buffer, DWORD size, va_list* arguments)
{
    if ((flags & FORMAT_MESSAGE_ALLOCATE_BUFFER) || (flags & FORMAT_MESSAGE_FROM_STRING)) {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return 0;
    }

    char narrowBuffer[1024];
    DWORD result = FormatMessageA(flags, source, messageID, languageID, narrowBuffer, sizeof(narrowBuffer), arguments);
    if (!result)
        return 0;
    if (!buffer || !size)
        return result;
    return win98AnsiToWide(narrowBuffer, buffer, size) ? win98WideLength(buffer) : 0;
}

static int WINAPI win98GetLocaleInfoW(LCID locale, LCTYPE type, LPWSTR data, int dataLength)
{
    if (type & LOCALE_RETURN_NUMBER)
        return GetLocaleInfoA(locale, type, reinterpret_cast<LPSTR>(data), dataLength);

    char narrowData[256];
    int result = GetLocaleInfoA(locale, type, data ? narrowData : nullptr, data ? sizeof(narrowData) : 0);
    if (!data || !dataLength)
        return result;
    if (!result)
        return 0;
    return win98AnsiToWide(narrowData, data, dataLength) ? win98WideLength(data) + 1 : 0;
}

static int WINAPI win98GetNumberFormatW(LCID locale, DWORD flags, LPCWSTR value, const NUMBERFMTW*, LPWSTR numberString, int numberStringLength)
{
    char valueA[128];
    char outputA[256];
    if (!win98WideToAnsi(value, valueA, sizeof(valueA)))
        return 0;
    int result = GetNumberFormatA(locale, flags, valueA, nullptr, numberString ? outputA : nullptr, numberString ? sizeof(outputA) : 0);
    if (!numberString || !numberStringLength)
        return result;
    if (!result)
        return 0;
    return win98AnsiToWide(outputA, numberString, numberStringLength) ? win98WideLength(numberString) + 1 : 0;
}

static int WINAPI win98GetCurrencyFormatW(LCID locale, DWORD flags, LPCWSTR value, const CURRENCYFMTW*, LPWSTR currencyString, int currencyStringLength)
{
    char valueA[128];
    char outputA[256];
    if (!win98WideToAnsi(value, valueA, sizeof(valueA)))
        return 0;
    int result = GetCurrencyFormatA(locale, flags, valueA, nullptr, currencyString ? outputA : nullptr, currencyString ? sizeof(outputA) : 0);
    if (!currencyString || !currencyStringLength)
        return result;
    if (!result)
        return 0;
    return win98AnsiToWide(outputA, currencyString, currencyStringLength) ? win98WideLength(currencyString) + 1 : 0;
}

static int WINAPI win98GetDateFormatW(LCID locale, DWORD flags, const SYSTEMTIME* date, LPCWSTR format, LPWSTR dateString, int dateStringLength)
{
    char formatA[128];
    char outputA[256];
    LPCSTR formatPtr = nullptr;
    if (format) {
        if (!win98WideToAnsi(format, formatA, sizeof(formatA)))
            return 0;
        formatPtr = formatA;
    }
    int result = GetDateFormatA(locale, flags, date, formatPtr, dateString ? outputA : nullptr, dateString ? sizeof(outputA) : 0);
    if (!dateString || !dateStringLength)
        return result;
    if (!result)
        return 0;
    return win98AnsiToWide(outputA, dateString, dateStringLength) ? win98WideLength(dateString) + 1 : 0;
}

static int WINAPI win98GetTimeFormatW(LCID locale, DWORD flags, const SYSTEMTIME* time, LPCWSTR format, LPWSTR timeString, int timeStringLength)
{
    char formatA[128];
    char outputA[256];
    LPCSTR formatPtr = nullptr;
    if (format) {
        if (!win98WideToAnsi(format, formatA, sizeof(formatA)))
            return 0;
        formatPtr = formatA;
    }
    int result = GetTimeFormatA(locale, flags, time, formatPtr, timeString ? outputA : nullptr, timeString ? sizeof(outputA) : 0);
    if (!timeString || !timeStringLength)
        return result;
    if (!result)
        return 0;
    return win98AnsiToWide(outputA, timeString, timeStringLength) ? win98WideLength(timeString) + 1 : 0;
}

static DWORD WINAPI win98GetCurrentDirectoryW(DWORD length, LPWSTR buffer)
{
    char narrowPath[MAX_PATH];
    DWORD result = GetCurrentDirectoryA(sizeof(narrowPath), narrowPath);
    if (!result)
        return 0;
    if (!buffer || !length)
        return result;
    return win98AnsiToWide(narrowPath, buffer, length) ? win98WideLength(buffer) : 0;
}

static BOOL WINAPI win98GetDiskFreeSpaceW(LPCWSTR root, LPDWORD sectorsPerCluster, LPDWORD bytesPerSector, LPDWORD freeClusters, LPDWORD clusters)
{
    char rootA[MAX_PATH];
    LPCSTR rootPtr = nullptr;
    if (root) {
        if (!win98WideToAnsi(root, rootA, sizeof(rootA)))
            return FALSE;
        rootPtr = rootA;
    }
    return GetDiskFreeSpaceA(rootPtr, sectorsPerCluster, bytesPerSector, freeClusters, clusters);
}

static BOOL WINAPI win98GetDiskFreeSpaceExW(LPCWSTR root, PULARGE_INTEGER freeBytesAvailable, PULARGE_INTEGER totalBytes, PULARGE_INTEGER totalFreeBytes)
{
    DWORD sectorsPerCluster = 0;
    DWORD bytesPerSector = 0;
    DWORD freeClusters = 0;
    DWORD clusters = 0;
    if (!win98GetDiskFreeSpaceW(root, &sectorsPerCluster, &bytesPerSector, &freeClusters, &clusters))
        return FALSE;

    ULONGLONG bytesPerCluster = static_cast<ULONGLONG>(sectorsPerCluster) * bytesPerSector;
    if (freeBytesAvailable)
        freeBytesAvailable->QuadPart = static_cast<ULONGLONG>(freeClusters) * bytesPerCluster;
    if (totalBytes)
        totalBytes->QuadPart = static_cast<ULONGLONG>(clusters) * bytesPerCluster;
    if (totalFreeBytes)
        totalFreeBytes->QuadPart = static_cast<ULONGLONG>(freeClusters) * bytesPerCluster;
    return TRUE;
}

static DWORD WINAPI win98GetFileAttributesW(LPCWSTR path)
{
    char narrowPath[MAX_PATH];
    if (!win98WideToAnsi(path, narrowPath, sizeof(narrowPath)))
        return INVALID_FILE_ATTRIBUTES;
    return GetFileAttributesA(narrowPath);
}

static BOOL WINAPI win98GetFileAttributesExW(LPCWSTR path, GET_FILEEX_INFO_LEVELS level, LPVOID info)
{
    if (level != GetFileExInfoStandard || !info) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    char narrowPath[MAX_PATH];
    WIN32_FIND_DATAA findData;
    if (!win98WideToAnsi(path, narrowPath, sizeof(narrowPath)))
        return FALSE;
    HANDLE handle = FindFirstFileA(narrowPath, &findData);
    if (handle == INVALID_HANDLE_VALUE)
        return FALSE;
    FindClose(handle);

    auto* attributes = static_cast<WIN32_FILE_ATTRIBUTE_DATA*>(info);
    attributes->dwFileAttributes = findData.dwFileAttributes;
    attributes->ftCreationTime = findData.ftCreationTime;
    attributes->ftLastAccessTime = findData.ftLastAccessTime;
    attributes->ftLastWriteTime = findData.ftLastWriteTime;
    attributes->nFileSizeHigh = findData.nFileSizeHigh;
    attributes->nFileSizeLow = findData.nFileSizeLow;
    return TRUE;
}

static DWORD WINAPI win98GetFullPathNameW(LPCWSTR path, DWORD bufferLength, LPWSTR buffer, LPWSTR* filePart)
{
    char pathA[MAX_PATH];
    char fullPathA[MAX_PATH];
    char* filePartA = nullptr;
    if (filePart)
        *filePart = nullptr;
    if (!win98WideToAnsi(path, pathA, sizeof(pathA)))
        return 0;
    DWORD result = GetFullPathNameA(pathA, sizeof(fullPathA), fullPathA, &filePartA);
    if (!result)
        return 0;
    if (!buffer || !bufferLength)
        return result;
    if (!win98AnsiToWide(fullPathA, buffer, bufferLength))
        return result;
    if (filePart && filePartA)
        *filePart = buffer + (filePartA - fullPathA);
    return win98WideLength(buffer);
}

static HMODULE WINAPI win98GetModuleHandleW(LPCWSTR name)
{
    char nameA[MAX_PATH];
    LPCSTR namePtr = nullptr;
    if (name) {
        if (!win98WideToAnsi(name, nameA, sizeof(nameA)))
            return nullptr;
        namePtr = nameA;
    }
    return GetModuleHandleA(namePtr);
}

static DWORD WINAPI win98GetTempPathW(DWORD bufferLength, LPWSTR buffer)
{
    char pathA[MAX_PATH];
    DWORD result = GetTempPathA(sizeof(pathA), pathA);
    if (!result)
        return 0;
    if (!buffer || !bufferLength)
        return result;
    return win98AnsiToWide(pathA, buffer, bufferLength) ? win98WideLength(buffer) : 0;
}

static BOOL WINAPI win98GetVersionExW(LPOSVERSIONINFOW versionInformation)
{
    if (!versionInformation) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    OSVERSIONINFOA versionA;
    ZeroMemory(&versionA, sizeof(versionA));
    versionA.dwOSVersionInfoSize = sizeof(versionA);
    if (!GetVersionExA(&versionA))
        return FALSE;

    versionInformation->dwMajorVersion = versionA.dwMajorVersion;
    versionInformation->dwMinorVersion = versionA.dwMinorVersion;
    versionInformation->dwBuildNumber = versionA.dwBuildNumber;
    versionInformation->dwPlatformId = versionA.dwPlatformId;
    win98AnsiToWide(versionA.szCSDVersion, versionInformation->szCSDVersion, sizeof(versionInformation->szCSDVersion) / sizeof(versionInformation->szCSDVersion[0]));
    return TRUE;
}

static BOOL WINAPI win98MoveFileExW(LPCWSTR existingName, LPCWSTR newName, DWORD flags)
{
    char existingNameA[MAX_PATH];
    char newNameA[MAX_PATH];
    if (!win98WideToAnsi(existingName, existingNameA, sizeof(existingNameA)))
        return FALSE;
    if (!newName)
        return DeleteFileA(existingNameA);
    if (!win98WideToAnsi(newName, newNameA, sizeof(newNameA)))
        return FALSE;
    if (flags & MOVEFILE_REPLACE_EXISTING)
        DeleteFileA(newNameA);
    return MoveFileA(existingNameA, newNameA);
}

static void WINAPI win98OutputDebugStringW(LPCWSTR message)
{
    char messageA[1024];
    if (!win98WideToAnsi(message, messageA, sizeof(messageA)))
        return;
    OutputDebugStringA(messageA);
}

static BOOL WINAPI win98RemoveDirectoryW(LPCWSTR path)
{
    char pathA[MAX_PATH];
    if (!win98WideToAnsi(path, pathA, sizeof(pathA)))
        return FALSE;
    return RemoveDirectoryA(pathA);
}

static BOOL WINAPI win98IsDebuggerPresent()
{
    return FALSE;
}

static BOOL WINAPI win98IsProcessorFeaturePresent(DWORD)
{
    return FALSE;
}

static BOOL WINAPI win98LockFileEx(HANDLE file, DWORD flags, DWORD, DWORD bytesToLockLow, DWORD bytesToLockHigh, LPOVERLAPPED overlapped)
{
    if (!overlapped) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (flags & LOCKFILE_FAIL_IMMEDIATELY)
        return LockFile(file, overlapped->Offset, overlapped->OffsetHigh, bytesToLockLow, bytesToLockHigh);
    while (!LockFile(file, overlapped->Offset, overlapped->OffsetHigh, bytesToLockLow, bytesToLockHigh)) {
        if (GetLastError() != ERROR_LOCK_VIOLATION)
            return FALSE;
        Sleep(1);
    }
    return TRUE;
}

static BOOL WINAPI win98CryptAcquireContextW(HCRYPTPROV* provider, LPCWSTR container, LPCWSTR providerName, DWORD providerType, DWORD flags)
{
    char containerA[MAX_PATH];
    char providerNameA[MAX_PATH];
    LPCSTR containerPtr = nullptr;
    LPCSTR providerNamePtr = nullptr;
    if (container) {
        if (!win98WideToAnsi(container, containerA, sizeof(containerA)))
            return FALSE;
        containerPtr = containerA;
    }
    if (providerName) {
        if (!win98WideToAnsi(providerName, providerNameA, sizeof(providerNameA)))
            return FALSE;
        providerNamePtr = providerNameA;
    }
    return CryptAcquireContextA(provider, containerPtr, providerNamePtr, providerType, flags);
}

static LSTATUS WINAPI win98RegOpenKeyExW(HKEY key, LPCWSTR subKey, DWORD options, REGSAM desired, PHKEY result)
{
    char subKeyA[MAX_PATH];
    LPCSTR subKeyPtr = nullptr;
    if (subKey) {
        if (!win98WideToAnsi(subKey, subKeyA, sizeof(subKeyA)))
            return ERROR_INVALID_PARAMETER;
        subKeyPtr = subKeyA;
    }
    return RegOpenKeyExA(key, subKeyPtr, options, desired, result);
}

static LSTATUS WINAPI win98RegEnumKeyExW(HKEY key, DWORD index, LPWSTR name, LPDWORD nameLength, LPDWORD reserved, LPWSTR className, LPDWORD classLength, PFILETIME lastWriteTime)
{
    char nameA[MAX_PATH];
    char classA[MAX_PATH];
    DWORD nameLengthA = sizeof(nameA);
    DWORD classLengthA = sizeof(classA);
    LSTATUS status = RegEnumKeyExA(key, index, nameA, &nameLengthA, reserved, className ? classA : nullptr, className ? &classLengthA : nullptr, lastWriteTime);
    if (status != ERROR_SUCCESS)
        return status;
    if (name && nameLength) {
        if (!win98AnsiToWide(nameA, name, *nameLength))
            return ERROR_MORE_DATA;
        *nameLength = win98WideLength(name);
    }
    if (className && classLength) {
        if (!win98AnsiToWide(classA, className, *classLength))
            return ERROR_MORE_DATA;
        *classLength = win98WideLength(className);
    }
    return ERROR_SUCCESS;
}

static LSTATUS WINAPI win98RegQueryInfoKeyW(HKEY key, LPWSTR className, LPDWORD classLength, LPDWORD reserved, LPDWORD subKeys, LPDWORD maxSubKeyLength, LPDWORD maxClassLength, LPDWORD values, LPDWORD maxValueNameLength, LPDWORD maxValueLength, LPDWORD securityDescriptor, PFILETIME lastWriteTime)
{
    char classA[MAX_PATH];
    DWORD classLengthA = sizeof(classA);
    LSTATUS status = RegQueryInfoKeyA(key, className ? classA : nullptr, className ? &classLengthA : nullptr, reserved, subKeys, maxSubKeyLength, maxClassLength, values, maxValueNameLength, maxValueLength, securityDescriptor, lastWriteTime);
    if (status != ERROR_SUCCESS)
        return status;
    if (className && classLength) {
        if (!win98AnsiToWide(classA, className, *classLength))
            return ERROR_MORE_DATA;
        *classLength = win98WideLength(className);
    }
    return ERROR_SUCCESS;
}

static LSTATUS WINAPI win98RegQueryValueExW(HKEY key, LPCWSTR valueName, LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD dataLength)
{
    char valueNameA[MAX_PATH];
    LPCSTR valueNamePtr = nullptr;
    if (valueName) {
        if (!win98WideToAnsi(valueName, valueNameA, sizeof(valueNameA)))
            return ERROR_INVALID_PARAMETER;
        valueNamePtr = valueNameA;
    }

    DWORD localType = 0;
    DWORD dataLengthA = dataLength ? *dataLength : 0;
    if (!data)
        return RegQueryValueExA(key, valueNamePtr, reserved, type, nullptr, dataLength);

    char dataA[2048];
    if (dataLengthA > sizeof(dataA))
        dataLengthA = sizeof(dataA);
    LSTATUS status = RegQueryValueExA(key, valueNamePtr, reserved, &localType, reinterpret_cast<LPBYTE>(dataA), &dataLengthA);
    if (status != ERROR_SUCCESS)
        return status;
    if (type)
        *type = localType;
    if ((localType == REG_SZ || localType == REG_EXPAND_SZ) && dataLength) {
        DWORD wideCapacity = *dataLength / sizeof(WCHAR);
        if (!win98AnsiToWide(dataA, reinterpret_cast<LPWSTR>(data), wideCapacity))
            return ERROR_MORE_DATA;
        *dataLength = (win98WideLength(reinterpret_cast<LPWSTR>(data)) + 1) * sizeof(WCHAR);
        return ERROR_SUCCESS;
    }
    if (dataLength && *dataLength >= dataLengthA) {
        CopyMemory(data, dataA, dataLengthA);
        *dataLength = dataLengthA;
        return ERROR_SUCCESS;
    }
    if (dataLength)
        *dataLength = dataLengthA;
    return ERROR_MORE_DATA;
}

HANDLE WINAPI win98CreateFileMappingWFunction(HANDLE file, LPSECURITY_ATTRIBUTES attributes, DWORD protect, DWORD maximumSizeHigh, DWORD maximumSizeLow, LPCWSTR name) asm("_CreateFileMappingW@24");
HANDLE WINAPI win98CreateFileMappingWFunction(HANDLE file, LPSECURITY_ATTRIBUTES attributes, DWORD protect, DWORD maximumSizeHigh, DWORD maximumSizeLow, LPCWSTR name)
{
    return win98CreateFileMappingW(file, attributes, protect, maximumSizeHigh, maximumSizeLow, name);
}

HANDLE WINAPI win98CreateFileWFunction(LPCWSTR path, DWORD desiredAccess, DWORD shareMode, LPSECURITY_ATTRIBUTES attributes, DWORD creationDisposition, DWORD flagsAndAttributes, HANDLE templateFile) asm("_CreateFileW@28");
HANDLE WINAPI win98CreateFileWFunction(LPCWSTR path, DWORD desiredAccess, DWORD shareMode, LPSECURITY_ATTRIBUTES attributes, DWORD creationDisposition, DWORD flagsAndAttributes, HANDLE templateFile)
{
    return win98CreateFileW(path, desiredAccess, shareMode, attributes, creationDisposition, flagsAndAttributes, templateFile);
}

HANDLE WINAPI win98CreateMutexWFunction(LPSECURITY_ATTRIBUTES attributes, BOOL initialOwner, LPCWSTR name) asm("_CreateMutexW@12");
HANDLE WINAPI win98CreateMutexWFunction(LPSECURITY_ATTRIBUTES attributes, BOOL initialOwner, LPCWSTR name)
{
    return win98CreateMutexW(attributes, initialOwner, name);
}

BOOL WINAPI win98DeleteFileWFunction(LPCWSTR path) asm("_DeleteFileW@4");
BOOL WINAPI win98DeleteFileWFunction(LPCWSTR path)
{
    return win98DeleteFileW(path);
}

DWORD WINAPI win98FormatMessageWFunction(DWORD flags, LPCVOID source, DWORD messageID, DWORD languageID, LPWSTR buffer, DWORD size, va_list* arguments) asm("_FormatMessageW@28");
DWORD WINAPI win98FormatMessageWFunction(DWORD flags, LPCVOID source, DWORD messageID, DWORD languageID, LPWSTR buffer, DWORD size, va_list* arguments)
{
    return win98FormatMessageW(flags, source, messageID, languageID, buffer, size, arguments);
}

BOOL WINAPI win98GetDiskFreeSpaceWFunction(LPCWSTR root, LPDWORD sectorsPerCluster, LPDWORD bytesPerSector, LPDWORD freeClusters, LPDWORD clusters) asm("_GetDiskFreeSpaceW@20");
BOOL WINAPI win98GetDiskFreeSpaceWFunction(LPCWSTR root, LPDWORD sectorsPerCluster, LPDWORD bytesPerSector, LPDWORD freeClusters, LPDWORD clusters)
{
    return win98GetDiskFreeSpaceW(root, sectorsPerCluster, bytesPerSector, freeClusters, clusters);
}

BOOL WINAPI win98GetFileAttributesExWFunction(LPCWSTR path, GET_FILEEX_INFO_LEVELS level, LPVOID info) asm("_GetFileAttributesExW@12");
BOOL WINAPI win98GetFileAttributesExWFunction(LPCWSTR path, GET_FILEEX_INFO_LEVELS level, LPVOID info)
{
    return win98GetFileAttributesExW(path, level, info);
}

DWORD WINAPI win98GetFileAttributesWFunction(LPCWSTR path) asm("_GetFileAttributesW@4");
DWORD WINAPI win98GetFileAttributesWFunction(LPCWSTR path)
{
    return win98GetFileAttributesW(path);
}

DWORD WINAPI win98GetFullPathNameWFunction(LPCWSTR path, DWORD bufferLength, LPWSTR buffer, LPWSTR* filePart) asm("_GetFullPathNameW@16");
DWORD WINAPI win98GetFullPathNameWFunction(LPCWSTR path, DWORD bufferLength, LPWSTR buffer, LPWSTR* filePart)
{
    return win98GetFullPathNameW(path, bufferLength, buffer, filePart);
}

HMODULE WINAPI win98GetModuleHandleWFunction(LPCWSTR name) asm("_GetModuleHandleW@4");
HMODULE WINAPI win98GetModuleHandleWFunction(LPCWSTR name)
{
    return win98GetModuleHandleW(name);
}

DWORD WINAPI win98GetTempPathWFunction(DWORD bufferLength, LPWSTR buffer) asm("_GetTempPathW@8");
DWORD WINAPI win98GetTempPathWFunction(DWORD bufferLength, LPWSTR buffer)
{
    return win98GetTempPathW(bufferLength, buffer);
}

BOOL WINAPI win98GetVersionExWFunction(LPOSVERSIONINFOW versionInformation) asm("_GetVersionExW@4");
BOOL WINAPI win98GetVersionExWFunction(LPOSVERSIONINFOW versionInformation)
{
    return win98GetVersionExW(versionInformation);
}

BOOL WINAPI win98IsProcessorFeaturePresentFunction(DWORD feature) asm("_IsProcessorFeaturePresent@4");
BOOL WINAPI win98IsProcessorFeaturePresentFunction(DWORD feature)
{
    return win98IsProcessorFeaturePresent(feature);
}

BOOL WINAPI win98LockFileExFunction(HANDLE file, DWORD flags, DWORD reserved, DWORD bytesToLockLow, DWORD bytesToLockHigh, LPOVERLAPPED overlapped) asm("_LockFileEx@24");
BOOL WINAPI win98LockFileExFunction(HANDLE file, DWORD flags, DWORD reserved, DWORD bytesToLockLow, DWORD bytesToLockHigh, LPOVERLAPPED overlapped)
{
    return win98LockFileEx(file, flags, reserved, bytesToLockLow, bytesToLockHigh, overlapped);
}

void WINAPI win98OutputDebugStringWFunction(LPCWSTR message) asm("_OutputDebugStringW@4");
void WINAPI win98OutputDebugStringWFunction(LPCWSTR message)
{
    win98OutputDebugStringW(message);
}

int ftruncate64(int, long long)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    errno = ENOSYS;
    return -1;
}

static int WINAPI win98GetLocaleInfoEx(LPCWSTR, LCTYPE localeType, LPWSTR localeData, int localeDataLength)
{
    return GetLocaleInfoW(LOCALE_USER_DEFAULT, localeType, localeData, localeDataLength);
}

static int WINAPI win98GetNumberFormatEx(LPCWSTR, DWORD flags, LPCWSTR value, const NUMBERFMTW* format, LPWSTR numberString, int numberStringLength)
{
    return GetNumberFormatW(LOCALE_USER_DEFAULT, flags, value, format, numberString, numberStringLength);
}

static int WINAPI win98GetCurrencyFormatEx(LPCWSTR, DWORD flags, LPCWSTR value, const CURRENCYFMTW* format, LPWSTR currencyString, int currencyStringLength)
{
    return GetCurrencyFormatW(LOCALE_USER_DEFAULT, flags, value, format, currencyString, currencyStringLength);
}

static int WINAPI win98GetDateFormatEx(LPCWSTR, DWORD flags, const SYSTEMTIME* date, LPCWSTR format, LPWSTR dateString, int dateStringLength, LPCWSTR)
{
    return GetDateFormatW(LOCALE_USER_DEFAULT, flags, date, format, dateString, dateStringLength);
}

static int WINAPI win98GetTimeFormatEx(LPCWSTR, DWORD flags, const SYSTEMTIME* time, LPCWSTR format, LPWSTR timeString, int timeStringLength)
{
    return GetTimeFormatW(LOCALE_USER_DEFAULT, flags, time, format, timeString, timeStringLength);
}

static int WINAPI win98ResolveLocaleName(LPCWSTR, LPWSTR, int)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

static int WINAPI win98LCIDToLocaleName(LCID, LPWSTR, int, DWORD)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

static LCID WINAPI win98LocaleNameToLCID(LPCWSTR, DWORD)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

static DWORD WINAPI win98GetDynamicTimeZoneInformation(void*)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return TIME_ZONE_ID_INVALID;
}

static LONG WINAPI win98GetUserGeoID(DWORD)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

static int WINAPI win98GetGeoInfoW(LONG, DWORD, LPWSTR, int, LANGID)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

static BOOL WINAPI win98SystemTimeToTzSpecificLocalTime(const TIME_ZONE_INFORMATION* timeZoneInformation, const SYSTEMTIME* universalTime, LPSYSTEMTIME localTime)
{
    if (!universalTime || !localTime) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    TIME_ZONE_INFORMATION localTimeZoneInformation;
    const TIME_ZONE_INFORMATION* timeZone = timeZoneInformation;
    if (!timeZone) {
        if (GetTimeZoneInformation(&localTimeZoneInformation) == TIME_ZONE_ID_INVALID)
            return FALSE;
        timeZone = &localTimeZoneInformation;
    }

    FILETIME universalFileTime;
    if (!SystemTimeToFileTime(universalTime, &universalFileTime))
        return FALSE;

    ULARGE_INTEGER localFileTime;
    localFileTime.LowPart = universalFileTime.dwLowDateTime;
    localFileTime.HighPart = universalFileTime.dwHighDateTime;

    const LONGLONG intervalsPerMinute = 60LL * 1000LL * 1000LL * 10LL;
    LONGLONG bias = static_cast<LONGLONG>(timeZone->Bias) * intervalsPerMinute;
    if (bias > 0 && localFileTime.QuadPart < static_cast<ULONGLONG>(bias)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    localFileTime.QuadPart -= bias;

    FILETIME adjustedFileTime;
    adjustedFileTime.dwLowDateTime = localFileTime.LowPart;
    adjustedFileTime.dwHighDateTime = localFileTime.HighPart;
    return FileTimeToSystemTime(&adjustedFileTime, localTime);
}

static BOOL WINAPI win98GetModuleHandleExW(DWORD, LPCWSTR moduleName, HMODULE* module)
{
    if (!module) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (moduleName) {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        *module = nullptr;
        return FALSE;
    }

    *module = GetModuleHandleA(nullptr);
    return *module != nullptr;
}

static LANGID WINAPI win98GetUserDefaultUILanguage()
{
    return MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
}

static BOOL WINAPI win98GetHandleInformation(HANDLE, LPDWORD flags)
{
    if (flags)
        *flags = 0;
    return TRUE;
}

static BOOL WINAPI win98GetProcessAffinityMask(HANDLE, PDWORD_PTR processAffinityMask, PDWORD_PTR systemAffinityMask)
{
    if (processAffinityMask)
        *processAffinityMask = 1;
    if (systemAffinityMask)
        *systemAffinityMask = 1;
    return TRUE;
}

static BOOL WINAPI win98SetProcessAffinityMask(HANDLE, DWORD_PTR)
{
    return TRUE;
}

static BOOL WINAPI win98GetProcessTimes(HANDLE, LPFILETIME creationTime, LPFILETIME exitTime, LPFILETIME kernelTime, LPFILETIME userTime)
{
    if (creationTime)
        *creationTime = {};
    if (exitTime)
        *exitTime = {};
    if (kernelTime)
        *kernelTime = {};
    if (userTime)
        *userTime = {};
    return TRUE;
}

static BOOL WINAPI win98GetThreadTimes(HANDLE, LPFILETIME creationTime, LPFILETIME exitTime, LPFILETIME kernelTime, LPFILETIME userTime)
{
    return win98GetProcessTimes(nullptr, creationTime, exitTime, kernelTime, userTime);
}

static BOOL WINAPI win98GetSystemTimeAdjustment(PDWORD adjustment, PDWORD increment, PBOOL disabled)
{
    if (adjustment)
        *adjustment = 0;
    if (increment)
        *increment = 10000;
    if (disabled)
        *disabled = TRUE;
    return TRUE;
}

static BOOL WINAPI win98SetSystemTime(const SYSTEMTIME*)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

static BOOL WINAPI win98SetThreadContext(HANDLE, const CONTEXT*)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

static BOOL WINAPI win98TryEnterCriticalSection(LPCRITICAL_SECTION)
{
    return FALSE;
}

static void WINAPI win98GetNativeSystemInfo(LPSYSTEM_INFO systemInfo)
{
    GetSystemInfo(systemInfo);
}

void WINAPI win98GetNativeSystemInfoFunction(LPSYSTEM_INFO systemInfo) asm("_GetNativeSystemInfo@4");
void WINAPI win98GetNativeSystemInfoFunction(LPSYSTEM_INFO systemInfo)
{
    win98GetNativeSystemInfo(systemInfo);
}

static BOOL WINAPI win98InitializeCriticalSectionEx(LPCRITICAL_SECTION criticalSection, DWORD, DWORD)
{
    InitializeCriticalSection(criticalSection);
    return TRUE;
}

BOOL WINAPI win98InitializeCriticalSectionExFunction(LPCRITICAL_SECTION criticalSection, DWORD spinCount, DWORD flags) asm("_InitializeCriticalSectionEx@12");
BOOL WINAPI win98InitializeCriticalSectionExFunction(LPCRITICAL_SECTION criticalSection, DWORD spinCount, DWORD flags)
{
    return win98InitializeCriticalSectionEx(criticalSection, spinCount, flags);
}

static void WINAPI win98InitializeConditionVariable(void* conditionVariable)
{
    if (conditionVariable)
        *static_cast<void**>(conditionVariable) = nullptr;
}

void WINAPI win98InitializeConditionVariableFunction(void* conditionVariable) asm("_InitializeConditionVariable@4");
void WINAPI win98InitializeConditionVariableFunction(void* conditionVariable)
{
    win98InitializeConditionVariable(conditionVariable);
}

static BOOL WINAPI win98SleepConditionVariableCS(void*, LPCRITICAL_SECTION criticalSection, DWORD milliseconds)
{
    if (criticalSection)
        LeaveCriticalSection(criticalSection);
    Sleep(milliseconds == INFINITE ? 1 : milliseconds);
    if (criticalSection)
        EnterCriticalSection(criticalSection);
    SetLastError(ERROR_TIMEOUT);
    return FALSE;
}

BOOL WINAPI win98SleepConditionVariableCSFunction(void* conditionVariable, LPCRITICAL_SECTION criticalSection, DWORD milliseconds) asm("_SleepConditionVariableCS@12");
BOOL WINAPI win98SleepConditionVariableCSFunction(void* conditionVariable, LPCRITICAL_SECTION criticalSection, DWORD milliseconds)
{
    return win98SleepConditionVariableCS(conditionVariable, criticalSection, milliseconds);
}

static void WINAPI win98WakeConditionVariable(void*)
{
}

void WINAPI win98WakeConditionVariableFunction(void* conditionVariable) asm("_WakeConditionVariable@4");
void WINAPI win98WakeConditionVariableFunction(void* conditionVariable)
{
    win98WakeConditionVariable(conditionVariable);
}

static BOOL WINAPI win98SwitchToThread()
{
    Sleep(0);
    return TRUE;
}

static HRESULT WINAPI win98SHGetFolderPathW(HWND, int, HANDLE, DWORD, LPWSTR path)
{
    if (!path)
        return E_INVALIDARG;

    if (!GetCurrentDirectoryW(MAX_PATH, path)) {
        path[0] = L'\0';
        return E_FAIL;
    }

    return S_OK;
}

void* win98GetLocaleInfoExImportPointer asm("__imp__GetLocaleInfoEx@16") = reinterpret_cast<void*>(win98GetLocaleInfoEx);
void* win98GetNumberFormatExImportPointer asm("__imp__GetNumberFormatEx@24") = reinterpret_cast<void*>(win98GetNumberFormatEx);
void* win98GetCurrencyFormatExImportPointer asm("__imp__GetCurrencyFormatEx@24") = reinterpret_cast<void*>(win98GetCurrencyFormatEx);
void* win98GetDateFormatExImportPointer asm("__imp__GetDateFormatEx@28") = reinterpret_cast<void*>(win98GetDateFormatEx);
void* win98GetTimeFormatExImportPointer asm("__imp__GetTimeFormatEx@24") = reinterpret_cast<void*>(win98GetTimeFormatEx);
void* win98ResolveLocaleNameImportPointer asm("__imp__ResolveLocaleName@12") = reinterpret_cast<void*>(win98ResolveLocaleName);
void* win98LCIDToLocaleNameImportPointer asm("__imp__LCIDToLocaleName@16") = reinterpret_cast<void*>(win98LCIDToLocaleName);
void* win98LocaleNameToLCIDImportPointer asm("__imp__LocaleNameToLCID@8") = reinterpret_cast<void*>(win98LocaleNameToLCID);
void* win98GetDynamicTimeZoneInformationImportPointer asm("__imp__GetDynamicTimeZoneInformation@4") = reinterpret_cast<void*>(win98GetDynamicTimeZoneInformation);
void* win98GetUserGeoIDImportPointer asm("__imp__GetUserGeoID@4") = reinterpret_cast<void*>(win98GetUserGeoID);
void* win98GetGeoInfoWImportPointer asm("__imp__GetGeoInfoW@20") = reinterpret_cast<void*>(win98GetGeoInfoW);
void* win98SystemTimeToTzSpecificLocalTimeImportPointer asm("__imp__SystemTimeToTzSpecificLocalTime@12") = reinterpret_cast<void*>(win98SystemTimeToTzSpecificLocalTime);
void* win98GetModuleHandleExWImportPointer asm("__imp__GetModuleHandleExW@12") = reinterpret_cast<void*>(win98GetModuleHandleExW);
void* win98GetUserDefaultUILanguageImportPointer asm("__imp__GetUserDefaultUILanguage@0") = reinterpret_cast<void*>(win98GetUserDefaultUILanguage);
void* win98GetHandleInformationImportPointer asm("__imp__GetHandleInformation@8") = reinterpret_cast<void*>(win98GetHandleInformation);
void* win98GetProcessAffinityMaskImportPointer asm("__imp__GetProcessAffinityMask@12") = reinterpret_cast<void*>(win98GetProcessAffinityMask);
void* win98SetProcessAffinityMaskImportPointer asm("__imp__SetProcessAffinityMask@8") = reinterpret_cast<void*>(win98SetProcessAffinityMask);
void* win98GetProcessTimesImportPointer asm("__imp__GetProcessTimes@20") = reinterpret_cast<void*>(win98GetProcessTimes);
void* win98GetThreadTimesImportPointer asm("__imp__GetThreadTimes@20") = reinterpret_cast<void*>(win98GetThreadTimes);
void* win98GetSystemTimeAdjustmentImportPointer asm("__imp__GetSystemTimeAdjustment@12") = reinterpret_cast<void*>(win98GetSystemTimeAdjustment);
void* win98SetSystemTimeImportPointer asm("__imp__SetSystemTime@4") = reinterpret_cast<void*>(win98SetSystemTime);
void* win98SetThreadContextImportPointer asm("__imp__SetThreadContext@8") = reinterpret_cast<void*>(win98SetThreadContext);
void* win98TryEnterCriticalSectionImportPointer asm("__imp__TryEnterCriticalSection@4") = reinterpret_cast<void*>(win98TryEnterCriticalSection);
void* win98GetNativeSystemInfoImportPointer asm("__imp__GetNativeSystemInfo@4") = reinterpret_cast<void*>(win98GetNativeSystemInfo);
void* win98InitializeCriticalSectionExImportPointer asm("__imp__InitializeCriticalSectionEx@12") = reinterpret_cast<void*>(win98InitializeCriticalSectionEx);
void* win98InitializeConditionVariableImportPointer asm("__imp__InitializeConditionVariable@4") = reinterpret_cast<void*>(win98InitializeConditionVariable);
void* win98SleepConditionVariableCSImportPointer asm("__imp__SleepConditionVariableCS@12") = reinterpret_cast<void*>(win98SleepConditionVariableCS);
void* win98WakeConditionVariableImportPointer asm("__imp__WakeConditionVariable@4") = reinterpret_cast<void*>(win98WakeConditionVariable);
void* win98SwitchToThreadImportPointer asm("__imp__SwitchToThread@0") = reinterpret_cast<void*>(win98SwitchToThread);
void* win98SHGetFolderPathWImportPointer asm("__imp__SHGetFolderPathW@20") = reinterpret_cast<void*>(win98SHGetFolderPathW);
void* win98CreateEventWImportPointer asm("__imp__CreateEventW@16") = reinterpret_cast<void*>(win98CreateEventW);
void* win98CreateFileMappingWImportPointer asm("__imp__CreateFileMappingW@24") = reinterpret_cast<void*>(win98CreateFileMappingW);
void* win98CreateFileWImportPointer asm("__imp__CreateFileW@28") = reinterpret_cast<void*>(win98CreateFileW);
void* win98CreateMutexWImportPointer asm("__imp__CreateMutexW@12") = reinterpret_cast<void*>(win98CreateMutexW);
void* win98CreateSemaphoreWImportPointer asm("__imp__CreateSemaphoreW@16") = reinterpret_cast<void*>(win98CreateSemaphoreW);
void* win98DeleteFileWImportPointer asm("__imp__DeleteFileW@4") = reinterpret_cast<void*>(win98DeleteFileW);
void* win98FindFirstFileWImportPointer asm("__imp__FindFirstFileW@8") = reinterpret_cast<void*>(win98FindFirstFileW);
void* win98FindNextFileWImportPointer asm("__imp__FindNextFileW@8") = reinterpret_cast<void*>(win98FindNextFileW);
void* win98FormatMessageWImportPointer asm("__imp__FormatMessageW@28") = reinterpret_cast<void*>(win98FormatMessageW);
void* win98GetCurrencyFormatWImportPointer asm("__imp__GetCurrencyFormatW@24") = reinterpret_cast<void*>(win98GetCurrencyFormatW);
void* win98GetCurrentDirectoryWImportPointer asm("__imp__GetCurrentDirectoryW@8") = reinterpret_cast<void*>(win98GetCurrentDirectoryW);
void* win98GetDateFormatWImportPointer asm("__imp__GetDateFormatW@24") = reinterpret_cast<void*>(win98GetDateFormatW);
void* win98GetDiskFreeSpaceWImportPointer asm("__imp__GetDiskFreeSpaceW@20") = reinterpret_cast<void*>(win98GetDiskFreeSpaceW);
void* win98GetDiskFreeSpaceExWImportPointer asm("__imp__GetDiskFreeSpaceExW@16") = reinterpret_cast<void*>(win98GetDiskFreeSpaceExW);
void* win98GetFileAttributesWImportPointer asm("__imp__GetFileAttributesW@4") = reinterpret_cast<void*>(win98GetFileAttributesW);
void* win98GetFileAttributesExWImportPointer asm("__imp__GetFileAttributesExW@12") = reinterpret_cast<void*>(win98GetFileAttributesExW);
void* win98GetFullPathNameWImportPointer asm("__imp__GetFullPathNameW@16") = reinterpret_cast<void*>(win98GetFullPathNameW);
void* win98GetLocaleInfoWImportPointer asm("__imp__GetLocaleInfoW@16") = reinterpret_cast<void*>(win98GetLocaleInfoW);
void* win98GetModuleHandleWImportPointer asm("__imp__GetModuleHandleW@4") = reinterpret_cast<void*>(win98GetModuleHandleW);
void* win98GetNumberFormatWImportPointer asm("__imp__GetNumberFormatW@24") = reinterpret_cast<void*>(win98GetNumberFormatW);
void* win98GetTempPathWImportPointer asm("__imp__GetTempPathW@8") = reinterpret_cast<void*>(win98GetTempPathW);
void* win98GetTimeFormatWImportPointer asm("__imp__GetTimeFormatW@24") = reinterpret_cast<void*>(win98GetTimeFormatW);
void* win98GetVersionExWImportPointer asm("__imp__GetVersionExW@4") = reinterpret_cast<void*>(win98GetVersionExW);
void* win98MoveFileExWImportPointer asm("__imp__MoveFileExW@12") = reinterpret_cast<void*>(win98MoveFileExW);
void* win98OutputDebugStringWImportPointer asm("__imp__OutputDebugStringW@4") = reinterpret_cast<void*>(win98OutputDebugStringW);
void* win98RemoveDirectoryWImportPointer asm("__imp__RemoveDirectoryW@4") = reinterpret_cast<void*>(win98RemoveDirectoryW);
void* win98IsDebuggerPresentImportPointer asm("__imp__IsDebuggerPresent@0") = reinterpret_cast<void*>(win98IsDebuggerPresent);
void* win98IsProcessorFeaturePresentImportPointer asm("__imp__IsProcessorFeaturePresent@4") = reinterpret_cast<void*>(win98IsProcessorFeaturePresent);
void* win98LockFileExImportPointer asm("__imp__LockFileEx@24") = reinterpret_cast<void*>(win98LockFileEx);
void* win98CryptAcquireContextWImportPointer asm("__imp__CryptAcquireContextW@20") = reinterpret_cast<void*>(win98CryptAcquireContextW);
void* win98RegEnumKeyExWImportPointer asm("__imp__RegEnumKeyExW@32") = reinterpret_cast<void*>(win98RegEnumKeyExW);
void* win98RegOpenKeyExWImportPointer asm("__imp__RegOpenKeyExW@20") = reinterpret_cast<void*>(win98RegOpenKeyExW);
void* win98RegQueryInfoKeyWImportPointer asm("__imp__RegQueryInfoKeyW@48") = reinterpret_cast<void*>(win98RegQueryInfoKeyW);
void* win98RegQueryValueExWImportPointer asm("__imp__RegQueryValueExW@24") = reinterpret_cast<void*>(win98RegQueryValueExW);

int clock_gettime(clockid_t, struct timespec*);
int nanosleep(const struct timespec*, struct timespec*);

void* win98ClockGetTimeImportPointer asm("__imp__clock_gettime") = reinterpret_cast<void*>(clock_gettime);
void* win98NanosleepImportPointer asm("__imp__nanosleep") = reinterpret_cast<void*>(nanosleep);

} // extern "C"

#endif
