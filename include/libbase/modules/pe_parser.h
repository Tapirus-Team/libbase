// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// refs: https://chromium.googlesource.com/chromium/chromium/+/refs/heads/main/base/win/pe_image.h

#pragma once
#include <delayimp.h>


namespace base::modules
{
    // This class is a wrapper for the Portable Executable File Format (PE).
    // It's main purpose is to provide an easy way to work with imports and exports
    // from a file, mapped in memory as image.
    class PEImage
    {
    public:
        // Callback to enumerate sections.
        // cookie is the value passed to the enumerate method.
        // Returns true to continue the enumeration.
        using EnumSectionsFunction = bool (*)(
            const PEImage& image,
            PIMAGE_SECTION_HEADER header,
            PVOID section_start,
            DWORD section_size,
            PVOID cookie);
        // Callback to enumerate exports.
        // function is the actual address of the symbol. If forward is not null, it
        // contains the dll and symbol to forward this export to. cookie is the value
        // passed to the enumerate method.
        // Returns true to continue the enumeration.
        using EnumExportsFunction = bool (*)(
            const PEImage& image,
            DWORD ordinal,
            DWORD hint,
            LPCSTR name,
            PVOID function,
            LPCSTR forward,
            PVOID cookie);
        // Callback to enumerate import blocks.
        // name_table and iat point to the imports name table and address table for
        // this block. cookie is the value passed to the enumerate method.
        // Returns true to continue the enumeration.
        using EnumImportChunksFunction = bool (*)(
            const PEImage& image,
            LPCSTR module,
            PIMAGE_THUNK_DATA name_table,
            PIMAGE_THUNK_DATA iat,
            PVOID cookie);
        // Callback to enumerate imports.
        // module is the dll that exports this symbol. cookie is the value passed to
        // the enumerate method.
        // Returns true to continue the enumeration.
        using EnumImportsFunction = bool (*)(
            const PEImage& image,
            LPCSTR module,
            DWORD ordinal,
            LPCSTR name,
            DWORD hint,
            PIMAGE_THUNK_DATA iat,
            PVOID cookie);
        // Callback to enumerate dalayed import blocks.
        // module is the dll that exports this block of symbols. cookie is the value
        // passed to the enumerate method.
        // Returns true to continue the enumeration.
        using EnumDelayImportChunksFunction = bool (*)(
            const PEImage& image,
            PImgDelayDescr delay_descriptor,
            LPCSTR module,
            PIMAGE_THUNK_DATA name_table,
            PIMAGE_THUNK_DATA iat,
            PIMAGE_THUNK_DATA bound_iat,
            PIMAGE_THUNK_DATA unload_iat,
            PVOID cookie);
        // Callback to enumerate relocations.
        // cookie is the value passed to the enumerate method.
        // Returns true to continue the enumeration.
        using EnumRelocsFunction = bool (*)(
            const PEImage& image,
            WORD type,
            PVOID address,
            PVOID cookie);

        explicit PEImage(HMODULE module)
            : _Module(module) {
            //
        }

        explicit PEImage(const void* module) {
            _Module = reinterpret_cast<HMODULE>(const_cast<void*>(module));
        }

        // Gets the HMODULE for this object.
        HMODULE Module() const;
        // Sets this object's HMODULE.
        void SetModule(_In_ HMODULE module);
        // Checks if this symbol is actually an ordinal.
        static bool IsOrdinal(_In_ LPCSTR name);
        // Converts a named symbol to the corresponding ordinal.
        static WORD ToOrdinal(_In_ LPCSTR name);
        // Returns the DOS_HEADER for this PE.
        PIMAGE_DOS_HEADER GetDosHeader() const;
        // Returns the NT_HEADER for this PE.
        PIMAGE_NT_HEADERS GetNTHeaders() const;
        // Returns number of sections of this PE.
        WORD GetNumSections() const;
        // Returns the header for a given section.
        // returns NULL if there is no such section.
        PIMAGE_SECTION_HEADER GetSectionHeader(_In_ UINT section) const;
        // Returns the size of a given directory entry.
        DWORD GetImageDirectoryEntrySize(_In_ UINT directory) const;
        // Returns the address of a given directory entry.
        PVOID GetImageDirectoryEntryAddr(_In_ UINT directory) const;
        // Returns the section header for a given address.
        // Use: s = image.GetImageSectionFromAddr(a);
        // Post: 's' is the section header of the section that contains 'a'
        //       or NULL if there is no such section.
        PIMAGE_SECTION_HEADER GetImageSectionFromAddr(_In_ PVOID address) const;
        // Returns the section header for a given section.
        PIMAGE_SECTION_HEADER GetImageSectionHeaderByName(_In_ LPCSTR section_name) const;
        // Returns the first block of imports.
        PIMAGE_IMPORT_DESCRIPTOR GetFirstImportChunk() const;
        // Returns the exports directory.
        PIMAGE_EXPORT_DIRECTORY GetExportDirectory() const;
        // Returns a given export entry.
        // Use: e = image.GetExportEntry(f);
        // Pre: 'f' is either a zero terminated string or ordinal
        // Post: 'e' is a pointer to the export directory entry
        //       that contains 'f's export RVA, or NULL if 'f'
        //       is not exported from this image
        PDWORD GetExportEntry(_In_ LPCSTR name) const;
        // Returns the address for a given exported symbol.
        // Use: p = image.GetProcAddress(f);
        // Pre: 'f' is either a zero terminated string or ordinal.
        // Post: if 'f' is a non-forwarded export from image, 'p' is
        //       the exported function. If 'f' is a forwarded export
        //       then p is the special value 0xFFFFFFFF. In this case
        //       RVAToAddr(*GetExportEntry) can be used to resolve
        //       the string that describes the forward.
        FARPROC GetProcAddress(_In_ LPCSTR function_name) const;
        // Retrieves the ordinal for a given exported symbol.
        // Returns true if the symbol was found.
        bool GetProcOrdinal(_In_ LPCSTR function_name, _Out_ WORD* ordinal) const;
        // Enumerates PE sections.
        // cookie is a generic cookie to pass to the callback.
        // Returns true on success.
        bool EnumSections(_In_ EnumSectionsFunction callback, _In_opt_ PVOID cookie) const;
        // Enumerates PE exports.
        // cookie is a generic cookie to pass to the callback.
        // Returns true on success.
        bool EnumExports(_In_ EnumExportsFunction callback, _In_opt_ PVOID cookie) const;
        // Enumerates PE imports.
        // cookie is a generic cookie to pass to the callback.
        // Returns true on success.
        bool EnumAllImports(_In_ EnumImportsFunction callback, _In_opt_ PVOID cookie) const;
        // Enumerates PE import blocks.
        // cookie is a generic cookie to pass to the callback.
        // Returns true on success.
        bool EnumImportChunks(_In_ EnumImportChunksFunction callback, _In_opt_ PVOID cookie) const;
        // Enumerates the imports from a single PE import block.
        // cookie is a generic cookie to pass to the callback.
        // Returns true on success.
        bool EnumOneImportChunk(
            _In_ EnumImportsFunction callback,
            _In_ LPCSTR module_name,
            _In_ PIMAGE_THUNK_DATA name_table,
            _In_ PIMAGE_THUNK_DATA iat,
            _In_opt_ PVOID cookie) const;
        // Enumerates PE delay imports.
        // cookie is a generic cookie to pass to the callback.
        // Returns true on success.
        bool EnumAllDelayImports(_In_ EnumImportsFunction callback, _In_opt_ PVOID cookie) const;
        // Enumerates PE delay import blocks.
        // cookie is a generic cookie to pass to the callback.
        // Returns true on success.
        bool EnumDelayImportChunks(_In_ EnumDelayImportChunksFunction callback, _In_opt_ PVOID cookie) const;
        // Enumerates imports from a single PE delay import block.
        // cookie is a generic cookie to pass to the callback.
        // Returns true on success.
        bool EnumOneDelayImportChunk(
            _In_ EnumImportsFunction callback,
            _In_ PImgDelayDescr delay_descriptor,
            _In_ LPCSTR module_name,
            _In_ PIMAGE_THUNK_DATA name_table,
            _In_ PIMAGE_THUNK_DATA iat,
            _In_ PIMAGE_THUNK_DATA bound_iat,
            _In_ PIMAGE_THUNK_DATA unload_iat,
            _In_opt_ PVOID cookie) const;
        // Enumerates PE relocation entries.
        // cookie is a generic cookie to pass to the callback.
        // Returns true on success.
        bool EnumRelocs(_In_ EnumRelocsFunction callback, _In_opt_ PVOID cookie) const;
        // Verifies the magic values on the PE file.
        // Returns true if all values are correct.
        bool VerifyMagic() const;
        // Converts an rva value to the appropriate address.
        virtual PVOID RVAToAddr(_In_ size_t rva) const;
        // Converts an rva value to an offset on disk.
        // Returns true on success.
        bool ImageRVAToOnDiskOffset(_In_ size_t rva, _Out_ DWORD* on_disk_offset) const;
        // Converts an address to an offset on disk.
        // Returns true on success.
        bool ImageAddrToOnDiskOffset(_In_ LPVOID address, _Out_ DWORD* on_disk_offset) const;

    private:
        HMODULE _Module;
    };

    // This class is an extension to the PEImage class that allows working with PE
    // files mapped as data instead of as image file.
    class PEImageAsData : public PEImage
    {
    public:
        explicit PEImageAsData(HMODULE hModule) : PEImage(hModule) {}
        virtual PVOID RVAToAddr(_In_ size_t rva) const;
    };

    inline bool PEImage::IsOrdinal(_In_ LPCSTR name) {
#pragma warning(push)
#pragma warning(disable: 4311)
        // This cast generates a warning because it is 32 bit specific.
        return reinterpret_cast<size_t>(name) <= 0xFFFF;
#pragma warning(pop)
    }

    inline WORD PEImage::ToOrdinal(_In_ LPCSTR name) {
        return static_cast<WORD>(reinterpret_cast<size_t>(name));
    }

    inline HMODULE PEImage::Module() const {
        return _Module;
    }

    inline PIMAGE_IMPORT_DESCRIPTOR PEImage::GetFirstImportChunk() const {
        return reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
            GetImageDirectoryEntryAddr(IMAGE_DIRECTORY_ENTRY_IMPORT));
    }

    inline PIMAGE_EXPORT_DIRECTORY PEImage::GetExportDirectory() const {
        return reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(
            GetImageDirectoryEntryAddr(IMAGE_DIRECTORY_ENTRY_EXPORT));
    }

}

namespace base
{
    using modules::PEImage;
}
