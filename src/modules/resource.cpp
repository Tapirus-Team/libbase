// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "../universal.inl"


namespace base::modules
{
    bool GetResourceData(
        _In_ void* module,
        _In_ int resource_id,
        _Out_ void** data,
        _Out_ size_t* length
    )
    {
        *data = nullptr;
        *length = 0;

        if (!module) {
            return false;
        }

        if (!IS_INTRESOURCE(resource_id)) {
            return false;
        }

        auto hres_info = FindResourceW(static_cast<HMODULE>(module), MAKEINTRESOURCEW(resource_id), L"BINDATA");
        if (hres_info == nullptr) {
            return false;
        }

        auto hres = LoadResource(static_cast<HMODULE>(module), hres_info);
        if (!hres) {
            return false;
        }

        auto data_size = SizeofResource(static_cast<HMODULE>(module), hres_info);

        void* resource = LockResource(hres);
        if (!resource) {
            return false;
        }

        *data = resource;
        *length = static_cast<size_t>(data_size);

        return true;
    }
}
