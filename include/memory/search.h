// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once


namespace base::memory
{
    void* MemorySearch(
        _In_bytecount_(aBytes)  void* aAddress,
        _In_ size_t aBytes,
        _In_ const char* aPattern,
        _In_opt_ bool aOptimization = true
    );
}
