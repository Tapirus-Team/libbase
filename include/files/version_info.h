// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include <string>
#include <memory>
#include <filesystem>


namespace base::files
{
    // Provides an interface for accessing the version information for a file. This
    // is the information you access when you select a file in the Windows Explorer,
    // right-click select Properties, then click the Version tab, and on the Mac
    // when you select a file in the Finder and do a Get Info.
    //
    // This list of properties is straight out of Win32's VerQueryValue
    // <http://msdn.microsoft.com/en-us/library/ms647464.aspx> and the Mac
    // version returns values from the Info.plist as appropriate. TODO(avi): make
    // this a less-obvious Windows-ism.
    //
    // Reference: https://github.com/chromium/chromium/blob/master/base/file_version_info_win.h
    class FileVersionInfo
    {
        const std::vector<uint8_t> _OwnedData;
        const void* const _Data = nullptr;
        const WORD _Language = 0;
        const WORD _CodePage = 0;

        // This is a reference for a portion of |_Data|.
        const VS_FIXEDFILEINFO& _FixedFileInfo;


        // |data| is a VS_VERSION_INFO resource. |language| and |code_page| are
        // extracted from the \VarFileInfo\Translation value of |data|.
        FileVersionInfo(std::vector<uint8_t>&& data,
            WORD language,
            WORD code_page);
        FileVersionInfo(void* data, WORD language, WORD code_page);

    public:
        ~FileVersionInfo() = default;
        FileVersionInfo(const FileVersionInfo&) = delete;
        FileVersionInfo& operator=(const FileVersionInfo&) = delete;

        std::wstring GetCompanyName() const;
        std::wstring GetCompanyShortName() const;
        std::wstring GetProductName() const;
        std::wstring GetProductShortName() const;
        std::wstring GetInternalName() const;
        std::wstring GetProductVersion() const;
        std::wstring GetSpecialBuild() const;
        std::wstring GetOriginalFileName() const;
        std::wstring GetFileDescription() const;
        std::wstring GetFileVersion() const;

        // Lets you access other properties not covered above. |value| is only
        // modified if GetValue() returns true.
        bool GetValue(const wchar_t* name, std::wstring* value) const;

        // Similar to GetValue but returns a std::u16string (empty string if the
        // property does not exist).
        std::wstring GetStringValue(const wchar_t* name) const;

        // Behaves like CreateFileVersionInfo, but returns a FileVersionInfo.
        static std::unique_ptr<FileVersionInfo> New(
            const std::filesystem::path& file_path);
    };
}

namespace base
{
    using files::FileVersionInfo;
}
