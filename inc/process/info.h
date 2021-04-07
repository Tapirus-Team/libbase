#pragma once


namespace base
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
}
