// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include <string>


namespace base::strings
{
    // Converts between wide and UTF-8 representations of a string. On error, the
    // result is system-dependent.
    std::string SysWideToUTF8(const std::wstring_view& wide);
    std::wstring SysUTF8ToWide(const std::string_view& utf8);

    // Converts between wide and the system multi-byte representations of a string.
    // DANGER: This will lose information and can change (on Windows, this can
    // change between reboots).
    std::string SysWideToNativeMB(const std::wstring_view& wide);
    std::wstring SysNativeMBToWide(const std::string_view& native_mb);

    // Windows-specific ------------------------------------------------------------

    // Converts between 8-bit and wide strings, using the given code page. The
    // code page identifier is one accepted by the Windows function
    // MultiByteToWideChar().
    std::wstring SysMultiByteToWide(const std::string_view& mb, uint32_t code_page);
    std::string SysWideToMultiByte(const std::wstring_view& wide,uint32_t code_page);
}
