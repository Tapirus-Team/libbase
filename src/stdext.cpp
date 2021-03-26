// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "universal.inl"


namespace base::stdext::filesystem
{
    std::filesystem::path module_path(void* mod)
    {
        enum : size_t { INIT_SIZE = MAX_PATH, MAX_SIZE = 23768ul };

        size_t size = INIT_SIZE;
        std::wstring path(size, L'\0');

        if (mod == nullptr)
        {
            mod = HINST_THISCOMPONENT;
        }

        while (MAX_SIZE >= size)
        {
            size = GetModuleFileNameW(
                static_cast<HINSTANCE>(mod), &path[0],
                static_cast<unsigned long>(path.size()));
            if (size == 0)
            {
                break;
            }

            if (size < path.size())
            {
                path.resize(size);
                break;
            }

            path.resize(path.size() * 2);
        }

        path.shrink_to_fit();

        return path;
    }

    std::filesystem::path expand_path(const std::filesystem::path& unexpand)
    {
        std::wstring buf;

        auto cch = ExpandEnvironmentStringsW(unexpand.c_str(), nullptr, 0);
        if (cch == 0)
        {
            return unexpand;
        }

        buf.resize(cch - 1);
        ExpandEnvironmentStringsW(unexpand.c_str(), &buf[0], cch);

        return buf;
    }
}
