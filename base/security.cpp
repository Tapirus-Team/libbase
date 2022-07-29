// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/universal.inl"

#include <sddl.h>
#include <wtsapi32.h>


namespace base::security
{
    bool SetTokenPrivilege(_In_ HANDLE token, _In_ LPCSTR privilege, _In_ BOOL enable)
    {
        TOKEN_PRIVILEGES tp = { 0 };
        LUID luid = { 0 };

        if (!::LookupPrivilegeValue(
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

    bool SetObjectSecurity(
        _In_  HANDLE  object,
        _In_  LPCSTR sddl,
        _In_  SECURITY_INFORMATION information
    )
    {
        bool result = false;
        PSECURITY_DESCRIPTOR secdesc{};

        do
        {
            if (!::ConvertStringSecurityDescriptorToSecurityDescriptor(
                sddl, SDDL_REVISION, &secdesc, nullptr))
            {
                break;
            }

            if (!NT_SUCCESS(::NtSetSecurityObject(object, information, secdesc)))
            {
                break;
            }

            result = true;

        } while (false);

        if (secdesc)
        {
            ::LocalFree(secdesc);
        }

        return result;
    }

    HANDLE GetTokenLinkedToken(
        _In_  HANDLE  token
    )
    {
        DWORD  length = 0;
        HANDLE linked = nullptr;
        TOKEN_LINKED_TOKEN   linkedInfo{};
        TOKEN_ELEVATION_TYPE elvtype = TokenElevationTypeDefault;

        do
        {

            if (!::GetTokenInformation(token, TokenElevationType, &elvtype,
                sizeof TOKEN_ELEVATION_TYPE, &length))
            {
                break;
            }

            if (elvtype != TokenElevationTypeDefault)
            {
                if (!::GetTokenInformation(token, TokenLinkedToken, &linkedInfo,
                    sizeof TOKEN_LINKED_TOKEN, &length))
                {
                    break;
                }
            }
            else
            {
                linkedInfo.LinkedToken = token;
            }

            if (!::DuplicateTokenEx(linkedInfo.LinkedToken, MAXIMUM_ALLOWED, nullptr,
                SecurityIdentification, TokenPrimary, &linked))
            {
                break;
            }

        } while (false);

        if (linkedInfo.LinkedToken)
        {
            ::CloseHandle(linkedInfo.LinkedToken);
        }

        return linked;
    }

    HANDLE CreateSystemToken(
        _In_  DWORD access
    )
    {
        HANDLE token        = nullptr;
        HANDLE sysProcess   = nullptr;
        HANDLE sysToken     = nullptr;

        do
        {
            auto SessionID = WTSGetActiveConsoleSessionId();
            if (SessionID == -1)
            {
                break;
            }

            DWORD count = 0;
            PWTS_PROCESS_INFO procInfoList = nullptr;

            if (!WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, 0, 1, &procInfoList, &count))
            {
                break;
            }

            DWORD lsassPID = 0;
            DWORD winlogonPID = 0;

            for (size_t i = 0; i < count; ++i)
            {
                auto& procInfo = procInfoList[i];

                if ((!procInfo.pProcessName) ||
                    (!procInfo.pUserSid) ||
                    (!::IsWellKnownSid(procInfo.pUserSid, WELL_KNOWN_SID_TYPE::WinLocalSystemSid)))
                {
                    continue;
                }

                if ((0 == lsassPID) &&
                    (0 == procInfo.SessionId) &&
                    (0 == ::_stricmp("lsass.exe", procInfo.pProcessName)))
                {
                    lsassPID = procInfo.ProcessId;
                    continue;
                }

                if ((0 == winlogonPID) &&
                    (SessionID == procInfo.SessionId) &&
                    (0 == ::_stricmp("winlogon.exe", procInfo.pProcessName)))
                {
                    winlogonPID = procInfo.ProcessId;
                    continue;
                }

                if (winlogonPID && lsassPID)
                {
                    break;
                }
            }

            WTSFreeMemory(procInfoList);

            sysProcess = ::OpenProcess(
                PROCESS_QUERY_INFORMATION,
                FALSE,
                lsassPID);
            if (!sysProcess)
            {
                sysProcess = ::OpenProcess(
                    PROCESS_QUERY_INFORMATION,
                    FALSE,
                    winlogonPID);
            }

            if (sysProcess == nullptr)
            {
                break;
            }

            if (!::OpenProcessToken(
                sysProcess,
                TOKEN_DUPLICATE,
                &sysToken))
            {
                break;
            }

            if (!::DuplicateTokenEx(
                sysToken,
                access,
                nullptr,
                SecurityIdentification,
                TokenPrimary,
                &token))
            {
                break;
            }

        } while (false);

        if (sysToken)
        {
            ::CloseHandle(sysToken);
        }

        if (sysProcess)
        {
            ::CloseHandle(sysProcess);
        }

        return token;
    }

    HANDLE CreateSessionToken(
        _In_  DWORD session_id
    )
    {
        HANDLE token = nullptr;
        WTSQueryUserToken(session_id, &token);

        return token;
    }

}
