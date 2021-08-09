// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "../universal.inl"


namespace base::files
{
    namespace {

        struct LanguageAndCodePage {
            WORD language;
            WORD code_page;
        };

        // Returns the \VarFileInfo\Translation value extracted from the
        // VS_VERSION_INFO resource in |data|.
        LanguageAndCodePage* GetTranslate(const void* data)
        {
            static constexpr wchar_t kTranslation[] = L"\\VarFileInfo\\Translation";

            LPVOID translate = nullptr;
            UINT dummy_size  = 0;

            if (::VerQueryValueW(data, kTranslation, &translate, &dummy_size))
                return static_cast<LanguageAndCodePage*>(translate);
            return nullptr;
        }

        const VS_FIXEDFILEINFO& GetVsFixedFileInfo(const void* data)
        {
            static constexpr wchar_t kRoot[] = L"\\";

            LPVOID fixed_file_info = nullptr;
            UINT dummy_size = 0;

            ::VerQueryValueW(data, kRoot, &fixed_file_info, &dummy_size);

            return *static_cast<VS_FIXEDFILEINFO*>(fixed_file_info);
        }

    }  // namespace

    std::unique_ptr<FileVersionInfo> FileVersionInfo::New(_In_ const std::filesystem::path& file_path)
    {
        DWORD dummy = 0;
        const wchar_t* path = file_path.c_str();
        const DWORD length = ::GetFileVersionInfoSizeW(path, &dummy);

        if (length == 0)
            return nullptr;

        std::vector<uint8_t> data(length, 0);

        if (!::GetFileVersionInfoW(path, 0, length, data.data()))
            return nullptr;

        const auto translate = GetTranslate(data.data());
        if (!translate)
            return nullptr;

        return std::unique_ptr<FileVersionInfo>(new FileVersionInfo(
            std::move(data), translate->language, translate->code_page));
    }

    FileVersionInfo::FileVersionInfo(std::vector<uint8_t>&& data, WORD language, WORD code_page)
        : _OwnedData(std::move(data))
        , _Data(_OwnedData.data())
        , _Language(language)
        , _CodePage(code_page)
        , _FixedFileInfo(GetVsFixedFileInfo(_Data))
    {
    }

    FileVersionInfo::FileVersionInfo(void* data, WORD language, WORD code_page)
        : _Data(data)
        , _Language(language)
        , _CodePage(code_page)
        , _FixedFileInfo(GetVsFixedFileInfo(_Data))
    {
    }

    std::wstring FileVersionInfo::GetCompanyName() const
    {
        return GetStringValue(L"CompanyName");
    }

    std::wstring FileVersionInfo::GetCompanyShortName() const
    {
        return GetStringValue(L"CompanyShortName");
    }

    std::wstring FileVersionInfo::GetProductName() const
    {
        return GetStringValue(L"ProductName");
    }

    std::wstring FileVersionInfo::GetProductShortName() const
    {
        return GetStringValue(L"ProductShortName");
    }

    std::wstring FileVersionInfo::GetInternalName() const
    {
        return GetStringValue(L"InternalName");
    }

    std::wstring FileVersionInfo::GetProductVersion() const
    {
        return GetStringValue(L"ProductVersion");
    }

    std::wstring FileVersionInfo::GetSpecialBuild() const
    {
        return GetStringValue(L"SpecialBuild");
    }

    std::wstring FileVersionInfo::GetOriginalFileName() const
    {
        return GetStringValue(L"OriginalFilename");
    }

    std::wstring FileVersionInfo::GetFileDescription() const
    {
        return GetStringValue(L"FileDescription");
    }

    std::wstring FileVersionInfo::GetFileVersion() const
    {
        return GetStringValue(L"FileVersion");
    }

    bool FileVersionInfo::GetValue(_In_ const wchar_t* name, _Out_ std::wstring* value) const
    {
        value->clear();

        const struct LanguageAndCodePage lang_codepages[] =
        {
            // Use the language and codepage from the DLL.
            { _Language, _CodePage },
            // Use the default language and codepage from the DLL.
            { ::GetUserDefaultLangID(), _CodePage },
            // Use the language from the DLL and Latin codepage (most common).
            { _Language, 1252 },
            // Use the default language and Latin codepage (most common).
            { ::GetUserDefaultLangID(), 1252 },
        };

        for (const auto& lang_codepage : lang_codepages)
        {
            wchar_t sub_block[MAX_PATH]{};

            _snwprintf_s(sub_block, _countof(sub_block), _countof(sub_block),
                L"\\StringFileInfo\\%04x%04x\\%ls", lang_codepage.language,
                lang_codepage.code_page, name);
            
            LPVOID value_ptr = nullptr;
            uint32_t size = 0;
            
            BOOL r = ::VerQueryValueW(_Data, sub_block, &value_ptr, &size);
            if (r && value_ptr && size)
            {
                value->assign(static_cast<wchar_t*>(value_ptr), size - 1);
                return true;
            }
        }

        return false;
    }

    std::wstring FileVersionInfo::GetStringValue(_In_ const wchar_t* name) const
    {
        std::wstring str;
        GetValue(name, &str);
        return std::move(str);
    }

}
