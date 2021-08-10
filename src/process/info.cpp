// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "../universal.inl"


namespace base::process
{
    // Reference: https://github.com/chromium/chromium/blob/master/base/process/process_info_win.cc#L30
    IntegrityLevel GetProcessIntegrityLevel(_In_ HANDLE process)
    {
        HANDLE process_token = nullptr;

        if (!::OpenProcessToken(process, TOKEN_QUERY, &process_token))
        {
            return IntegrityLevel::INTEGRITY_UNKNOWN;
        }

        auto process_token_guard = stdext::scope_resource(
            process_token,
            [](HANDLE token) { CloseHandle(token); });

        DWORD token_info_length = 0;
        if (::GetTokenInformation(process_token, TokenIntegrityLevel, nullptr, 0, &token_info_length) ||
            ::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            return IntegrityLevel::INTEGRITY_UNKNOWN;
        }

        auto token_label_bytes = std::make_unique<char[]>(token_info_length);
        auto token_label = reinterpret_cast<TOKEN_MANDATORY_LABEL*>(token_label_bytes.get());
        if (!::GetTokenInformation(process_token, TokenIntegrityLevel, token_label,
            token_info_length, &token_info_length))
        {
            return IntegrityLevel::INTEGRITY_UNKNOWN;
        }

        DWORD integrity_level = *::GetSidSubAuthority(
            token_label->Label.Sid,
            static_cast<DWORD>(*::GetSidSubAuthorityCount(token_label->Label.Sid) - 1));

        if (integrity_level < SECURITY_MANDATORY_LOW_RID)
            return IntegrityLevel::UNTRUSTED_INTEGRITY;

        if (integrity_level < SECURITY_MANDATORY_MEDIUM_RID)
            return IntegrityLevel::LOW_INTEGRITY;

        if (integrity_level < SECURITY_MANDATORY_MEDIUM_PLUS_RID)
            return IntegrityLevel::MEDIUM_INTEGRITY;

        if (integrity_level < SECURITY_MANDATORY_HIGH_RID)
            return IntegrityLevel::MEDIUM_PLUS_INTEGRITY;

        if (integrity_level < SECURITY_MANDATORY_SYSTEM_RID)
            return IntegrityLevel::HIGH_INTEGRITY;

        if (integrity_level < SECURITY_MANDATORY_PROTECTED_PROCESS_RID)
            return IntegrityLevel::SYSTEM_INTEGRITY;
        else
            return IntegrityLevel::PROTECTED_PROCESS_INTEGRITY;

        return IntegrityLevel::INTEGRITY_UNKNOWN;
    }

    // Reference: https://github.com/chromium/chromium/blob/master/base/process/process_info_win.cc#L71
    bool IsProcessElevated(_In_ HANDLE process)
    {
        HANDLE process_token = nullptr;

        if (!::OpenProcessToken(process, TOKEN_QUERY, &process_token)) {
            false;
        }

        auto process_token_guard = stdext::scope_resource(
            process_token,
            [](HANDLE token) { CloseHandle(token); });

        // Unlike TOKEN_ELEVATION_TYPE which returns TokenElevationTypeDefault when
        // UAC is turned off, TOKEN_ELEVATION returns whether the process is elevated.
        DWORD size = 0;
        TOKEN_ELEVATION elevation = { 0 };
        if (!GetTokenInformation(process_token, TokenElevation, &elevation,
            sizeof(elevation), &size))
        {
            return false;
        }

        return !!elevation.TokenIsElevated;
    }

    bool SetProcessPrivilege(_In_ HANDLE token, _In_ LPCWSTR privilege, _In_ BOOL enable)
    {
        TOKEN_PRIVILEGES tp = { 0 };
        LUID luid = { 0 };

        if (!::LookupPrivilegeValueW(
            nullptr,        // lookup privilege on local system
            privilege,      // privilege to lookup 
            &luid))         // receives LUID of privilege
        {
            return false;
        }

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        if (enable)
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        else
            tp.Privileges[0].Attributes = 0;

        // Enable the privilege or disable all privileges.
        if (!::AdjustTokenPrivileges(
            token,
            FALSE,
            &tp,
            sizeof(TOKEN_PRIVILEGES),
            nullptr,
            nullptr))
        {
            return false;
        }

        if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
        {
            return false;
        }

        return true;
    }

    std::shared_ptr<uint8_t> QueryInformationProcess(
        _In_ PROCESSINFOCLASS query_class,
        _In_ HANDLE process
    )
    {
        NTSTATUS result = STATUS_SUCCESS;
        auto need_bytes = 0ul;
        auto information = std::shared_ptr<uint8_t>();

        result = ZwQueryInformationProcess(process, query_class, nullptr, 0, &need_bytes);
        if (need_bytes == 0) {
            return nullptr;
        }
        need_bytes += 512;

        information = std::shared_ptr<uint8_t>(new uint8_t[need_bytes]{});
        if (information == nullptr) {
            return nullptr;
        }

        result = ZwQueryInformationProcess(process, query_class, information.get(), need_bytes, &need_bytes);
        if (!NT_SUCCESS(result)) {
            return nullptr;
        }

        return information;
    }
}
