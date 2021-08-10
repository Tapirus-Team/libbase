// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "universal.inl"


namespace base
{
    std::shared_ptr<uint8_t> QueryInformationSystem(
        _In_ SYSTEM_INFORMATION_CLASS query_class
    )
    {
        NTSTATUS result  = STATUS_SUCCESS;
        auto need_bytes  = 0ul;
        auto information = std::shared_ptr<uint8_t>();

        result = ZwQuerySystemInformation(query_class, nullptr, 0, &need_bytes);
        if (need_bytes == 0) {
            return nullptr;
        }
        need_bytes += 512;

        information = std::shared_ptr<uint8_t>(new uint8_t[need_bytes]{});
        if (information == nullptr) {
            return nullptr;
        }

        result = ZwQuerySystemInformation(query_class, information.get(), need_bytes, &need_bytes);
        if (!NT_SUCCESS(result)) {
            return nullptr;
        }

        return information;
    }
}
