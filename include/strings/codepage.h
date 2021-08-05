// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include <string>


namespace base::codepage
{
    // Converts between 8-bit and wide strings, using the given code page. The
    // code page identifier is one accepted by the Windows function
    // MultiByteToWideChar().
    std::wstring mbstowcs(_In_ const std::string_view& mbs,  _In_opt_ uint32_t code_page = CP_UTF8);
    std::string  wcstombs(_In_ const std::wstring_view& wcs, _In_opt_ uint32_t code_page = CP_UTF8);

    size_t mbslen(_In_ const std::string_view&  mbs);
    size_t wcslen(_In_ const std::wstring_view& wcs);
}

namespace base
{
    using codepage::mbstowcs;
    using codepage::wcstombs;

    using codepage::mbslen;
    using codepage::wcslen;
}
