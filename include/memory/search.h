// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once


namespace base::memory
{    
    // Change the page protection (of code pages) to writable and copy
    // the data at the specified location
    //
    // Arguments:
    // old_code               Target location to copy
    // new_code               Source
    // length                 Number of bytes to copy
    //
    // Returns: Windows error code (winerror.h). NO_ERROR if successful
    DWORD ModifyCode(
        _Inout_ void* old_code,
        _In_bytecount_(length)  void* new_code,
        _In_ int length
    );

    void* MemorySearch(
        _In_bytecount_(aBytes)  void* aAddress,
        _In_ size_t aBytes,
        _In_ const char* aPattern,
        _In_opt_ bool aOptimization = true
    );
}

namespace base
{
    using memory::ModifyCode;
    using memory::MemorySearch;
}
