// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "../universal.inl"


namespace base::process
{
    std::wstring RunSystemCommandGetResult(const std::wstring& cmd)
    {
        std::wstring result;

        wchar_t buffer[128]{};
        FILE* pipe = _wpopen(cmd.c_str(), L"r");
        if (!pipe)
        {
            return result;
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
        return result;
    }

}
