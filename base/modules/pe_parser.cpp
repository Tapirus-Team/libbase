// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/universal.inl"


namespace base::modules
{
    // Structure to perform imports enumerations.
    struct EnumAllImportsStorage {
        PEImage::EnumImportsFunction Callback;
        PVOID Cookie;
    };

    // Callback used to enumerate imports. See EnumImportChunksFunction.
    bool ProcessImportChunk(
        const PEImage& image,
        LPCSTR module,
        PIMAGE_THUNK_DATA name_table,
        PIMAGE_THUNK_DATA iat,
        PVOID cookie
    ) {
        EnumAllImportsStorage& storage = *reinterpret_cast<EnumAllImportsStorage*>(
            cookie);
        return image.EnumOneImportChunk(storage.Callback, module, name_table, iat,
            storage.Cookie);
    }

    // Callback used to enumerate delay imports. See EnumDelayImportChunksFunction.
    bool ProcessDelayImportChunk(
        const PEImage& image,
        PImgDelayDescr delay_descriptor,
        LPCSTR module,
        PIMAGE_THUNK_DATA name_table,
        PIMAGE_THUNK_DATA iat,
        PIMAGE_THUNK_DATA bound_iat,
        PIMAGE_THUNK_DATA unload_iat,
        PVOID cookie
    ) {
        EnumAllImportsStorage& storage = *reinterpret_cast<EnumAllImportsStorage*>(
            cookie);
        return image.EnumOneDelayImportChunk(storage.Callback, delay_descriptor,
            module, name_table, iat, bound_iat,
            unload_iat, storage.Cookie);
    }

    void PEImage::SetModule(_In_ HMODULE module) {
        _Module = module;
    }

    PIMAGE_DOS_HEADER PEImage::GetDosHeader() const {
        return reinterpret_cast<PIMAGE_DOS_HEADER>(_Module);
    }

    PIMAGE_NT_HEADERS PEImage::GetNTHeaders() const {
        PIMAGE_DOS_HEADER dos_header = GetDosHeader();
        return reinterpret_cast<PIMAGE_NT_HEADERS>(
            reinterpret_cast<char*>(dos_header) + dos_header->e_lfanew);
    }

    PIMAGE_SECTION_HEADER PEImage::GetSectionHeader(_In_ UINT section) const {
        PIMAGE_NT_HEADERS     nt_headers    = GetNTHeaders();
        PIMAGE_SECTION_HEADER first_section = IMAGE_FIRST_SECTION(nt_headers);

        if (section < nt_headers->FileHeader.NumberOfSections) {
            return first_section + section;
        }

        return nullptr;
    }

    WORD PEImage::GetNumSections() const {
        return GetNTHeaders()->FileHeader.NumberOfSections;
    }

    DWORD PEImage::GetImageDirectoryEntrySize(_In_ UINT directory) const {
        PIMAGE_NT_HEADERS nt_headers = GetNTHeaders();
        return nt_headers->OptionalHeader.DataDirectory[directory].Size;
    }

    PVOID PEImage::GetImageDirectoryEntryAddr(_In_ UINT directory) const {
        PIMAGE_NT_HEADERS nt_headers = GetNTHeaders();
        return RVAToAddr(
            nt_headers->OptionalHeader.DataDirectory[directory].VirtualAddress);
    }

    PIMAGE_SECTION_HEADER PEImage::GetImageSectionFromAddr(_In_ PVOID address) const {
        auto target = reinterpret_cast<PBYTE>(address);
        PIMAGE_SECTION_HEADER section = nullptr;

        for (UINT i = 0; (section = GetSectionHeader(i)) != nullptr; i++) {
            // Don't use the virtual RVAToAddr.
            auto start = reinterpret_cast<PBYTE>(PEImage::RVAToAddr(section->VirtualAddress));
            DWORD size = section->Misc.VirtualSize;

            if ((start <= target) && (start + size > target))
                return section;
        }

        return nullptr;
    }

    PIMAGE_SECTION_HEADER PEImage::GetImageSectionHeaderByName(_In_ LPCSTR section_name) const {
        if (section_name == nullptr) {
            return nullptr;
        }

        PIMAGE_SECTION_HEADER ret = nullptr;

        int num_sections = GetNumSections();
        for (int i = 0; i < num_sections; i++) {
            PIMAGE_SECTION_HEADER section = GetSectionHeader(i);
            if (0 == _strnicmp(reinterpret_cast<LPCSTR>(section->Name), section_name, sizeof(section->Name))) {
                ret = section;
                break;
            }
        }
        return ret;
    }

    PDWORD PEImage::GetExportEntry(_In_ LPCSTR name) const {
        PIMAGE_EXPORT_DIRECTORY exports = GetExportDirectory();
        if (nullptr == exports) {
            return nullptr;
        }

        WORD ordinal = 0;
        if (!GetProcOrdinal(name, &ordinal)) {
            return nullptr;
        }

        auto functions = reinterpret_cast<PDWORD>(RVAToAddr(exports->AddressOfFunctions));

        return functions + ordinal - exports->Base;
    }

    FARPROC PEImage::GetProcAddress(_In_ LPCSTR function_name) const {
        PDWORD export_entry = GetExportEntry(function_name);
        if (nullptr == export_entry) {
            return nullptr;
        }

        auto function = reinterpret_cast<PBYTE>(RVAToAddr(*export_entry));
        auto exports  = reinterpret_cast<PBYTE>(GetImageDirectoryEntryAddr(IMAGE_DIRECTORY_ENTRY_EXPORT));
        auto size     = GetImageDirectoryEntrySize(IMAGE_DIRECTORY_ENTRY_EXPORT);

        // Check for forwarded exports as a special case.
        if (exports <= function && exports + size > function)
#pragma warning(push)
#pragma warning(disable: 4312)
            // This cast generates a warning because it is 32 bit specific.
            return reinterpret_cast<FARPROC>(~0);
#pragma warning(pop)
        return reinterpret_cast<FARPROC>(function);
    }

    bool PEImage::GetProcOrdinal(_In_ LPCSTR function_name, _Out_ WORD* ordinal) const {
        if (nullptr == ordinal) {
            return false;
        }

        *ordinal = 0;

        PIMAGE_EXPORT_DIRECTORY exports = GetExportDirectory();
        if (nullptr == exports) {
            return false;
        }

        if (IsOrdinal(function_name)) {
            *ordinal = ToOrdinal(function_name);
        }
        else {
            PDWORD names = reinterpret_cast<PDWORD>(RVAToAddr(exports->AddressOfNames));
            PDWORD lower = names;
            PDWORD upper = names + exports->NumberOfNames;
            int cmp = -1;
            // Binary Search for the name.
            while (lower != upper) {
                PDWORD middle = lower + (upper - lower) / 2;
                LPCSTR name = reinterpret_cast<LPCSTR>(RVAToAddr(*middle));
                // This may be called by sandbox before MSVCRT dll loads, so can't use
                // CRT function here.
                cmp = strcmp(function_name, name);
                if (cmp == 0) {
                    lower = middle;
                    break;
                }
                if (cmp > 0)
                    lower = middle + 1;
                else
                    upper = middle;
            }
            if (cmp != 0)
                return false;
            PWORD ordinals = reinterpret_cast<PWORD>(
                RVAToAddr(exports->AddressOfNameOrdinals));
            *ordinal = ordinals[lower - names] + static_cast<WORD>(exports->Base);
        }
        return true;
    }

    bool PEImage::EnumSections(_In_ EnumSectionsFunction callback, _In_opt_ PVOID cookie) const {
        auto nt_headers     = GetNTHeaders();
        UINT num_sections   = nt_headers->FileHeader.NumberOfSections;
        auto section        = GetSectionHeader(0);

        for (UINT i = 0; i < num_sections; i++, section++) {
            PVOID section_start = RVAToAddr(section->VirtualAddress);
            DWORD size = section->Misc.VirtualSize;
            if (!callback(*this, section, section_start, size, cookie)) {
                return false;
            }
        }

        return true;
    }

    bool PEImage::EnumExports(_In_ EnumExportsFunction callback, _In_opt_ PVOID cookie) const {
        PVOID directory = GetImageDirectoryEntryAddr(IMAGE_DIRECTORY_ENTRY_EXPORT);
        DWORD size = GetImageDirectoryEntrySize(IMAGE_DIRECTORY_ENTRY_EXPORT);
        // Check if there are any exports at all.
        if (nullptr == directory || 0 == size) {
            return true;
        }

        auto exports        = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(directory);
        UINT ordinal_base   = exports->Base;
        UINT num_funcs      = exports->NumberOfFunctions;
        UINT num_names      = exports->NumberOfNames;
        auto functions      = reinterpret_cast<PDWORD>(RVAToAddr(exports->AddressOfFunctions));
        auto names          = reinterpret_cast<PDWORD>(RVAToAddr(exports->AddressOfNames));
        auto ordinals       = reinterpret_cast<PWORD >(RVAToAddr(exports->AddressOfNameOrdinals));

        for (UINT count = 0; count < num_funcs; count++) {
            PVOID func = RVAToAddr(functions[count]);
            if (nullptr == func) {
                continue;
            }

            // Check for a name.
            LPCSTR name = nullptr;
            UINT hint = 0;
            for (hint = 0; hint < num_names; hint++) {
                if (ordinals[hint] == count) {
                    name = reinterpret_cast<LPCSTR>(RVAToAddr(names[hint]));
                    break;
                }
            }

            if (name == nullptr) {
                hint = 0;
            }

            // Check for forwarded exports.
            LPCSTR forward = nullptr;
            if (reinterpret_cast<char*>(func) >= reinterpret_cast<char*>(directory) &&
                reinterpret_cast<char*>(func) <= reinterpret_cast<char*>(directory) +
                size) {
                forward = reinterpret_cast<LPCSTR>(func);
                func = nullptr;
            }

            if (!callback(*this, ordinal_base + count, hint, name, func, forward, cookie))
                return false;
        }

        return true;
    }

    bool PEImage::EnumRelocs(_In_ EnumRelocsFunction callback, _In_opt_ PVOID cookie) const {
        PVOID directory = GetImageDirectoryEntryAddr(IMAGE_DIRECTORY_ENTRY_BASERELOC);
        DWORD size = GetImageDirectoryEntrySize(IMAGE_DIRECTORY_ENTRY_BASERELOC);

        auto base = reinterpret_cast<PIMAGE_BASE_RELOCATION>(directory);
        if (directory == nullptr || size < sizeof(IMAGE_BASE_RELOCATION)) {
            return true;
        }

        while (base->SizeOfBlock) {
            PWORD reloc = reinterpret_cast<PWORD>(base + 1);
            UINT num_relocs = (base->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);

            for (UINT i = 0; i < num_relocs; i++, reloc++) {
                WORD type = *reloc >> 12;
                PVOID address = RVAToAddr(base->VirtualAddress + (*reloc & 0x0FFF));

                if (!callback(*this, type, address, cookie)) {
                    return false;
                }
            }

            base = reinterpret_cast<PIMAGE_BASE_RELOCATION>(reinterpret_cast<char*>(base) + base->SizeOfBlock);
        }

        return true;
    }

    bool PEImage::EnumImportChunks(_In_ EnumImportChunksFunction callback, _In_opt_ PVOID cookie) const {
        DWORD size  = GetImageDirectoryEntrySize(IMAGE_DIRECTORY_ENTRY_IMPORT);
        auto import = GetFirstImportChunk();

        if (import == nullptr || size < sizeof(IMAGE_IMPORT_DESCRIPTOR)) {
            return true;
        }

        for (; import->FirstThunk; import++) {
            auto module_name    = reinterpret_cast<LPCSTR>(RVAToAddr(import->Name));
            auto name_table     = reinterpret_cast<PIMAGE_THUNK_DATA>(RVAToAddr(import->OriginalFirstThunk));
            auto iat            = reinterpret_cast<PIMAGE_THUNK_DATA>(RVAToAddr(import->FirstThunk));

            if (!callback(*this, module_name, name_table, iat, cookie)) {
                return false;
            }
        }

        return true;
    }

    bool PEImage::EnumOneImportChunk(
        _In_ EnumImportsFunction callback,
        _In_ LPCSTR module_name,
        _In_ PIMAGE_THUNK_DATA name_table,
        _In_ PIMAGE_THUNK_DATA iat,
        _In_opt_ PVOID cookie
    ) const {
        if (nullptr == name_table) {
            return false;
        }

        for (; name_table && name_table->u1.Ordinal; name_table++, iat++) {
            LPCSTR name = nullptr;
            WORD ordinal = 0;
            WORD hint = 0;

            if (IMAGE_SNAP_BY_ORDINAL(name_table->u1.Ordinal)) {
                ordinal = static_cast<WORD>(IMAGE_ORDINAL32(name_table->u1.Ordinal));
            }
            else {
                auto import = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(RVAToAddr(name_table->u1.ForwarderString));
                hint = import->Hint;
                name = reinterpret_cast<LPCSTR>(&import->Name);
            }

            if (!callback(*this, module_name, ordinal, name, hint, iat, cookie)) {
                return false;
            }
        }

        return true;
    }
    bool PEImage::EnumAllImports(_In_ EnumImportsFunction callback, _In_opt_ PVOID cookie) const {
        EnumAllImportsStorage temp = { callback, cookie };
        return EnumImportChunks(ProcessImportChunk, &temp);
    }

    bool PEImage::EnumDelayImportChunks(_In_ EnumDelayImportChunksFunction callback, _In_opt_ PVOID cookie) const {
        PVOID directory = GetImageDirectoryEntryAddr(IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
        DWORD size = GetImageDirectoryEntrySize(IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);

        auto delay_descriptor = reinterpret_cast<PImgDelayDescr>(directory);
        if (directory == nullptr || size == 0) {
            return true;
        }

        for (; delay_descriptor->rvaHmod; delay_descriptor++) {
            PIMAGE_THUNK_DATA name_table = nullptr;
            PIMAGE_THUNK_DATA iat        = nullptr;
            PIMAGE_THUNK_DATA bound_iat  = nullptr;     // address of the optional bound IAT
            PIMAGE_THUNK_DATA unload_iat = nullptr;     // address of optional copy of original IAT
            LPCSTR module_name = nullptr;
            // check if VC7-style imports, using RVAs instead of
            // VC6-style addresses.
            bool rvas = (delay_descriptor->grAttrs & dlattrRva) != 0;
            if (rvas) {
                module_name = reinterpret_cast<LPCSTR>(
                    RVAToAddr(delay_descriptor->rvaDLLName));
                name_table = reinterpret_cast<PIMAGE_THUNK_DATA>(
                    RVAToAddr(delay_descriptor->rvaINT));
                iat = reinterpret_cast<PIMAGE_THUNK_DATA>(
                    RVAToAddr(delay_descriptor->rvaIAT));
                bound_iat = reinterpret_cast<PIMAGE_THUNK_DATA>(
                    RVAToAddr(delay_descriptor->rvaBoundIAT));
                unload_iat = reinterpret_cast<PIMAGE_THUNK_DATA>(
                    RVAToAddr(delay_descriptor->rvaUnloadIAT));
            }
            else {
#pragma warning(push)
#pragma warning(disable: 4312)
                // These casts generate warnings because they are 32 bit specific.
                module_name = reinterpret_cast<LPCSTR>(delay_descriptor->rvaDLLName);
                name_table = reinterpret_cast<PIMAGE_THUNK_DATA>(
                    delay_descriptor->rvaINT);
                iat = reinterpret_cast<PIMAGE_THUNK_DATA>(delay_descriptor->rvaIAT);
                bound_iat = reinterpret_cast<PIMAGE_THUNK_DATA>(
                    delay_descriptor->rvaBoundIAT);
                unload_iat = reinterpret_cast<PIMAGE_THUNK_DATA>(
                    delay_descriptor->rvaUnloadIAT);
#pragma warning(pop)
            }

            if (!callback(*this, delay_descriptor, module_name, name_table, iat, bound_iat, unload_iat, cookie)) {
                return false;
            }
        }

        return true;
    }

    bool PEImage::EnumOneDelayImportChunk(
        _In_ EnumImportsFunction callback,
        _In_ PImgDelayDescr delay_descriptor,
        _In_ LPCSTR module_name,
        _In_ PIMAGE_THUNK_DATA name_table,
        _In_ PIMAGE_THUNK_DATA iat,
        _In_ PIMAGE_THUNK_DATA bound_iat,
        _In_ PIMAGE_THUNK_DATA unload_iat,
        _In_opt_ PVOID cookie
    ) const {
        UNREFERENCED_PARAMETER(bound_iat);
        UNREFERENCED_PARAMETER(unload_iat);

        for (; name_table->u1.Ordinal; name_table++, iat++) {
            LPCSTR name = nullptr;
            WORD ordinal = 0;
            WORD hint = 0;
            if (IMAGE_SNAP_BY_ORDINAL(name_table->u1.Ordinal)) {
                ordinal = static_cast<WORD>(IMAGE_ORDINAL32(name_table->u1.Ordinal));
            }
            else {
                PIMAGE_IMPORT_BY_NAME import = nullptr;
                bool rvas = (delay_descriptor->grAttrs & dlattrRva) != 0;
                if (rvas) {
                    import = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
                        RVAToAddr(name_table->u1.ForwarderString));
                }
                else {
#pragma warning(push)
#pragma warning(disable: 4312)
                    // This cast generates a warning because it is 32 bit specific.
                    import = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
                        name_table->u1.ForwarderString);
#pragma warning(pop)
                }
                hint = import->Hint;
                name = reinterpret_cast<LPCSTR>(&import->Name);
            }

            if (!callback(*this, module_name, ordinal, name, hint, iat, cookie)) {
                return false;
            }
        }

        return true;
    }

    bool PEImage::EnumAllDelayImports(_In_ EnumImportsFunction callback, _In_opt_ PVOID cookie) const {
        EnumAllImportsStorage temp = { callback, cookie };
        return EnumDelayImportChunks(ProcessDelayImportChunk, &temp);
    }

    bool PEImage::VerifyMagic() const {
        PIMAGE_DOS_HEADER dos_header = GetDosHeader();
        if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
            return false;
        PIMAGE_NT_HEADERS nt_headers = GetNTHeaders();
        if (nt_headers->Signature != IMAGE_NT_SIGNATURE)
            return false;
        if (nt_headers->FileHeader.SizeOfOptionalHeader != sizeof(IMAGE_OPTIONAL_HEADER))
            return false;
        if (nt_headers->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
            return false;
        return true;
    }

    bool PEImage::ImageRVAToOnDiskOffset(_In_ size_t rva, _Out_ DWORD* on_disk_offset) const {
        LPVOID address = RVAToAddr(rva);
        return ImageAddrToOnDiskOffset(address, on_disk_offset);
    }

    bool PEImage::ImageAddrToOnDiskOffset(_In_ LPVOID address, _Out_ DWORD* on_disk_offset) const {
        *on_disk_offset = 0;

        if (nullptr == address) {
            return false;
        }
        // Get the section that this address belongs to.
        PIMAGE_SECTION_HEADER section_header = GetImageSectionFromAddr(address);
        if (nullptr == section_header) {
            return false;
        }
#pragma warning(push)
#pragma warning(disable: 4311)
        // These casts generate warnings because they are 32 bit specific.
        // Don't follow the virtual RVAToAddr, use the one on the base.
        size_t offset_within_section = reinterpret_cast<size_t>(address) -
            reinterpret_cast<size_t>(PEImage::RVAToAddr(section_header->VirtualAddress));
#pragma warning(pop)

        * on_disk_offset = static_cast<DWORD>(section_header->PointerToRawData + offset_within_section);
        return true;
    }

    PVOID PEImage::RVAToAddr(_In_ size_t rva) const {
        if (rva == 0)
            return nullptr;
        return reinterpret_cast<char*>(_Module) + rva;
    }

    PVOID PEImageAsData::RVAToAddr(_In_ size_t rva) const {
        if (rva == 0) {
            return nullptr;
        }

        PVOID in_memory   = PEImage::RVAToAddr(rva);
        DWORD disk_offset = 0;
        if (!ImageAddrToOnDiskOffset(in_memory, &disk_offset)) {
            return nullptr;
        }

        return PEImage::RVAToAddr(disk_offset);
    }
}
