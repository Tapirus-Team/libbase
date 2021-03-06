// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include <string>
#include <vector>
#include <cwctype>
#include <algorithm>


namespace base::strings
{
    template<typename T>
    T tolower(T c);

    template<typename T>
    T toupper(T c);

    template<>
    char tolower(char c);

    template<>
    wchar_t tolower(wchar_t c);

    template<>
    char toupper(char c);

    template<>
    wchar_t toupper(wchar_t c);

    template<typename T>
    void to_lower(_Inout_ std::basic_string<T>& str)
    {
        std::transform(str.begin(), str.end(), str.begin(), &tolower<T>);
    }

    template<typename T>
    void to_upper(_Inout_ std::basic_string<T>& str)
    {
        std::transform(str.begin(), str.end(), str.begin(), &toupper<T>);
    }

    template<typename T>
    std::basic_string<T> to_lower_copy(_In_ std::basic_string<T> str)
    {
        std::transform(str.begin(), str.end(), str.begin(), &tolower<T>);
        return std::move(str);
    }

    template<typename T>
    std::basic_string<T> to_upper_copy(_In_ std::basic_string<T> str)
    {
        std::transform(str.begin(), str.end(), str.begin(), &toupper<T>);
        return std::move(str);
    }

    template<typename T>
    std::vector<typename std::basic_string_view<T>> split(_In_ std::basic_string_view<T> str, _In_ const std::basic_string_view<T> delims)
    {
        size_t first = 0;
        std::vector<std::basic_string_view<T>> output;

        while (first < str.size())
        {
            const auto second = str.find_first_of(delims, first);

            if (first != second)
                output.emplace_back(str.substr(first, second - first));

            if (second == std::basic_string_view<T>::npos)
                break;

            first = second + 1;
        }

        return output;
    }

    template<typename T>
    constexpr bool starts_with(_In_ std::basic_string_view<T> str, _In_ const std::basic_string_view<T> search_for, _In_opt_ bool ignore_case = false)
    {
        if (search_for.size() > str.size())
            return false;

        auto source = str.substr(0, search_for.size());

        if (ignore_case)
        {
            return ::std::equal(search_for.begin(), search_for.end(), source.begin(), [](T x, T y) -> bool
                {
                    return tolower<T>(x) == tolower<T>(y);
                });
        }

        return source == search_for;
    }

    template<typename T>
    constexpr bool ends_with(_In_ std::basic_string_view<T> str, _In_ const std::basic_string_view<T> search_for, _In_opt_ bool ignore_case = false)
    {
        if (search_for.size() > str.size())
            return false;

        auto source = str.substr(str.size() - search_for.size(), search_for.size());

        if (ignore_case)
        {
            return ::std::equal(search_for.begin(), search_for.end(), source.begin(), [](T x, T y) -> bool
                {
                    return tolower<T>(x) == tolower<T>(y);
                });
        }

        return source == search_for;
    }

    template<typename T>
    constexpr bool contains(_In_ const std::basic_string_view<T> str, _In_ const std::basic_string_view<T> search_for, _In_opt_ bool ignore_case = false)
    {
        if (search_for.size() > str.size())
            return false;

        if (ignore_case)
        {
            return std::search(str.begin(), str.end(), search_for.begin(), search_for.end(), [](T x, T y) -> bool
                {
                    return tolower<T>(x) == tolower<T>(y);

                }) != str.end();
        }

        return str.find(search_for) != std::basic_string_view<T>::npos;
    }

}
