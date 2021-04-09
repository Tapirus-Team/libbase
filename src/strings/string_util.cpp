// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "../universal.inl"


namespace base::strings
{
    template<>
    char tolower(char c)
    {
        return static_cast<char>(::std::tolower(static_cast<unsigned char>(c)));
    }

    template<>
    wchar_t tolower(wchar_t c)
    {
        return ::std::towlower(c);
    }

    template<>
    char toupper(char c)
    {
        return static_cast<char>(::std::toupper(static_cast<unsigned char>(c)));
    }

    template<>
    wchar_t toupper(wchar_t c)
    {
        return ::std::towupper(c);
    }
}
