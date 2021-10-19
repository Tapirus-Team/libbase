// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/universal.inl"


namespace base::modules
{
    void* dlopen(
        _In_ const char* name,
        _In_opt_ uint32_t mode
    )
    {
        auto name_wcs = mbstowcs(name, CP_UTF8);
        HMODULE handle = nullptr;

        // ref:x+1
        if (GetModuleHandleExW(0, name_wcs.c_str(), &handle))
        {
            return handle;
        }

        // ref:0+1
        if (LoadLibraryExW(name_wcs.c_str(), nullptr, mode))
        {
            // ref:1+1
            if (GetModuleHandleExW(0, name_wcs.c_str(), &handle))
            {
                return handle;
            }
        }

        return nullptr;
    }

    void* dlref(
        _In_ const char* name,
        _In_opt_ uint32_t mode
    )
    {
        auto name_wcs = mbstowcs(name, CP_UTF8);
        HMODULE handle = nullptr;

        // ref:x+1
        if (GetModuleHandleExW(mode, name_wcs.c_str(), &handle))
        {
            return handle;
        }

        return nullptr;
    }

    int dlclose(
        _In_ void* handle
    )
    {
        // ref:x-1
        return FreeLibrary((HMODULE)handle);
    }

    void* dlsym(
        _In_ void* handle,
        _In_ const char* symbol
    )
    {
        return GetProcAddress((HMODULE)handle, symbol);
    }
}
