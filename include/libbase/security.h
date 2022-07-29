// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once


namespace base::security
{
    constexpr char LOW_INTEGRITY_SDDL_SACL[] = u8"S:(ML;;NW;;;LW)";
    constexpr char EVERYONE_USER_SDDL_DACL[] = u8"D:(A;OICI;GA;;;SY)(A;OICI;GA;;;BA)(A;OICI;GA;;;BU)(A;OICI;GA;;;WD)";

    bool SetTokenPrivilege(
        _In_ HANDLE  token,      // access token handle
        _In_ LPCSTR  privilege,  // name of privilege to enable/disable (SE_xxx_NAME)
        _In_ BOOL    enable      // to enable or disable privilege
    );

    bool SetObjectSecurity(
        _In_ HANDLE  object,
        _In_ LPCSTR sddl,
        _In_ SECURITY_INFORMATION information_class
    );

    HANDLE GetTokenLinkedToken(
        _In_ HANDLE token
    );

    HANDLE CreateSystemToken(
        _In_ DWORD access
    );

    HANDLE CreateSessionToken(
        _In_ DWORD session_id
    );
}

namespace base
{
    using security::LOW_INTEGRITY_SDDL_SACL;
    using security::EVERYONE_USER_SDDL_DACL;

    using security::SetTokenPrivilege;
}
