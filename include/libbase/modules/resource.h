// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once


namespace base::modules
{
    // Function for getting a data resource (BINDATA) from a dll.  Some
    // resources are optional, especially in unit tests, so this returns false
    // but doesn't raise an error if the resource can't be loaded.
    bool GetResourceData(
        _In_ void* module,
        _In_ int resource_id,
        _Out_ void** data,
        _Out_ size_t* length
    );
}
