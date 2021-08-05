// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "../universal.inl"


namespace base::process
{
    std::string RunSystemCommandGetResult(const std::string_view& cmd)
    {
        std::wstring result;

        wchar_t buffer[128]{};
        FILE* pipe = _wpopen(mbstowcs(cmd).c_str(), L"r");
        if (!pipe)
        {
            return {};
        }
        try
        {
            while (fgetws(buffer, _countof(buffer), pipe) != nullptr)
            {
                result += buffer;
            }
        }
        catch (...)
        {
            _pclose(pipe);
            throw;
        }

        _pclose(pipe);
        return wcstombs(result);
    }

}
