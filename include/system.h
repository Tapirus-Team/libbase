// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include <memory>


namespace base
{
    std::shared_ptr<uint8_t> QueryInformationSystem(
        _In_ SYSTEM_INFORMATION_CLASS query_class
        );

}
