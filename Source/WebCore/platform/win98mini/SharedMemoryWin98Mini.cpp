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
#include "SharedMemory.h"

#include <windows.h>

namespace WebCore {

static DWORD accessMode(SharedMemory::Protection protection)
{
    switch (protection) {
    case SharedMemory::Protection::ReadOnly:
        return FILE_MAP_READ;
    case SharedMemory::Protection::ReadWrite:
        return FILE_MAP_ALL_ACCESS;
    }

    ASSERT_NOT_REACHED();
    return FILE_MAP_ALL_ACCESS;
}

RefPtr<SharedMemory> SharedMemory::allocate(size_t size)
{
    if (!size || size > std::numeric_limits<DWORD>::max())
        return nullptr;

    auto fileMapping = Win32Handle::adopt(CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, static_cast<DWORD>(size), nullptr));
    if (!fileMapping)
        return nullptr;

    void* data = MapViewOfFile(fileMapping.get(), FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (!data)
        return nullptr;

    RefPtr<SharedMemory> instance = adoptRef(new SharedMemory());
    instance->m_data = data;
    instance->m_handle = WTF::move(fileMapping);
    instance->m_size = size;
    return instance;
}

RefPtr<SharedMemory> SharedMemory::map(Handle&& handle, Protection protection, CopyOnWrite copyOnWrite)
{
    ASSERT_UNUSED(copyOnWrite, copyOnWrite == CopyOnWrite::No);

    if (!handle.size())
        return nullptr;

    void* data = MapViewOfFile(handle.m_handle.get(), accessMode(protection), 0, 0, handle.size());
    if (!data)
        return nullptr;

    RefPtr<SharedMemory> instance = adoptRef(new SharedMemory());
    instance->m_data = data;
    instance->m_handle = WTF::move(handle.m_handle);
    instance->m_size = handle.size();
    return instance;
}

SharedMemory::~SharedMemory()
{
    if (m_data)
        UnmapViewOfFile(m_data);
}

auto SharedMemory::createHandle(Protection) -> std::optional<Handle>
{
    Win32Handle duplicate { m_handle };
    if (!duplicate)
        return std::nullopt;
    return { Handle(WTF::move(duplicate), m_size) };
}

} // namespace WebCore
