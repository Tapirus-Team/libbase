// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define UMDF_USING_NTSTATUS
#include <windows.h>
#include <windowsx.h>
#include <winioctl.h>
#include <ntstatus.h>
#include <include/libbase.h>

#include <iostream>

int main(int /*argc*/, char* /*argv*/[])
{
    base::SetConsoleCodePage();
    base::SetProcessPrivilege(GetCurrentProcessToken(), SE_DEBUG_NAME, true);

    return 0;
}
