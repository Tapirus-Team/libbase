// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include <memory>


namespace base::process
{
    enum class IntegrityLevel
    {
        INTEGRITY_UNKNOWN,
        UNTRUSTED_INTEGRITY,
        LOW_INTEGRITY,
        MEDIUM_INTEGRITY,
        MEDIUM_PLUS_INTEGRITY,
        HIGH_INTEGRITY,
        SYSTEM_INTEGRITY,
        PROTECTED_PROCESS_INTEGRITY,
    };

    IntegrityLevel GetProcessIntegrityLevel(
        _In_ HANDLE process
    );

    bool IsProcessElevated(
        _In_ HANDLE process
    );

    bool SetProcessPrivilege(
        _In_ HANDLE  token,      // access token handle
        _In_ LPCWSTR privilege,  // name of privilege to enable/disable
        _In_ BOOL    enable      // to enable or disable privilege
    );

    std::shared_ptr<uint8_t> QueryInformationProcess(
        _In_ PROCESSINFOCLASS query_class,
        _In_ HANDLE process
    );
}

namespace base
{
    using process::IntegrityLevel;
    using process::GetProcessIntegrityLevel;
    using process::IsProcessElevated;
    using process::SetProcessPrivilege;
    using process::QueryInformationProcess;
}
