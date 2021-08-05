// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once


namespace base::console
{
    void SetConsoleCodePage(
        _In_opt_ uint32_t    CodePage = CP_UTF8,
        _In_opt_ const char* FontName = u8"Lucida Console"
    );

    void RedirectIOToConsole(
        _In_opt_ short MaxConsoleLines = 5000
    );
}

namespace base
{
    using console::SetConsoleCodePage;
    using console::RedirectIOToConsole;
}
