// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once


namespace base::modules
{
    void* dlopen(
        _In_ const char* name,
        _In_opt_ uint32_t mode = 0
    );

    int dlclose(
        _In_ void* handle
    );

    void* dlsym(
        _In_ void* handle,
        _In_ const char* symbol
    );
}

namespace base
{
    using modules::dlopen;
    using modules::dlclose;
    using modules::dlsym;
}
