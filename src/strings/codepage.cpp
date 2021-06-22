// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "../universal.inl"


namespace base::strings
{
    // Do not assert in this function since it is used by the asssertion code!
    std::string SysWideToUTF8(const std::wstring_view& wide)
    {
        return SysWideToMultiByte(wide, CP_UTF8);
    }

    // Do not assert in this function since it is used by the asssertion code!
    std::wstring SysUTF8ToWide(const std::string_view& utf8)
    {
        return SysMultiByteToWide(utf8, CP_UTF8);
    }

    std::string SysWideToNativeMB(const std::wstring_view& wide)
    {
        return SysWideToMultiByte(wide, CP_ACP);
    }

    std::wstring SysNativeMBToWide(const std::string_view& native_mb)
    {
        return SysMultiByteToWide(native_mb, CP_ACP);
    }

    // Do not assert in this function since it is used by the asssertion code!
    std::wstring SysMultiByteToWide(const std::string_view& mb, uint32_t code_page)
    {
        if (mb.empty())
            return std::wstring();

        int mb_length = static_cast<int>(mb.length());
        // Compute the length of the buffer.
        int charcount = MultiByteToWideChar(code_page, 0,
            mb.data(), mb_length, nullptr, 0);
        if (charcount == 0)
            return std::wstring();

        std::wstring wide;
        wide.resize(charcount);
        MultiByteToWideChar(code_page, 0, mb.data(), mb_length, &wide[0], charcount);

        return std::move(wide);
    }

    // Do not assert in this function since it is used by the asssertion code!
    std::string SysWideToMultiByte(const std::wstring_view& wide, uint32_t code_page)
    {
        int wide_length = static_cast<int>(wide.length());
        if (wide_length == 0)
            return std::string();

        // Compute the length of the buffer we'll need.
        int charcount = WideCharToMultiByte(code_page, 0, wide.data(), wide_length,
            nullptr, 0, nullptr, nullptr);
        if (charcount == 0)
            return std::string();

        std::string mb;
        mb.resize(charcount);
        WideCharToMultiByte(code_page, 0, wide.data(), wide_length,
            &mb[0], charcount, nullptr, nullptr);

        return std::move(mb);
    }
}
