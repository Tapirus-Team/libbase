// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#include "base/universal.inl"


namespace base::memory
{
    SharedMemory::SharedMemory(const std::string_view name)
        : _Name(name)
    {

    }

    SharedMemory::SharedMemory(HANDLE handle, bool read_only)
        : _Section(handle)
        , _ReadOnly(read_only)
    {

    }

    SharedMemory::SharedMemory(HANDLE handle, bool read_only, HANDLE process)
        : _ReadOnly(read_only)
    {
        ::DuplicateHandle(
            process, handle,
            GetCurrentProcess(), &_Section,
            STANDARD_RIGHTS_REQUIRED | (read_only ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS),
            FALSE,
            0);
    }

    SharedMemory::~SharedMemory()
    {
        Close();

        if (_Lock) {
            CloseHandle(_Lock);
        }
    }

    bool SharedMemory::Create(_In_opt_ const std::string_view name, _In_ bool open_existing, _In_ uint32_t size)
    {
        if (size == 0) {
            return false;
        }

        auto name_wcs = mbstowcs(name, CP_UTF8);

        // NaCl's memory allocator requires 0mod64K alignment and size for
        // shared memory objects.  To allow passing shared memory to NaCl,
        // therefore we round the size actually created to the nearest 64K unit.
        // To avoid client impact, we continue to retain the size as the
        // actual requested size.
        uint32_t rounded_size = (size + 0xffff) & ~0xffff;

        _Section = CreateFileMappingW(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0, static_cast<DWORD>(rounded_size),
            name_wcs.empty() ? nullptr : name_wcs.c_str());
        if (!_Section) {
            return false;
        }

        // Check if the shared memory pre-exists.
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            // If the file already existed, set created_size_ to 0 to show that
            // we don't know the size.

            if (!open_existing) {
                Close();
                return false;
            }
        }

        _Name = name;
        return true;
    }

    bool SharedMemory::CreateAndMapAnonymous(_In_ uint32_t size)
    {
        return CreateAnonymous(size) && Map(size);
    }

    bool SharedMemory::Open(_In_ const std::string_view name, _In_ bool read_only)
    {
        auto name_wcs = mbstowcs(name, CP_UTF8);

        _Section  = OpenFileMappingW(
            read_only ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS,
            false,
            name_wcs.empty() ? nullptr : name_wcs.c_str());

        _Name     = name;
        _ReadOnly = read_only;

        return !!_Section;
    }

    bool SharedMemory::Map(_In_ uint32_t bytes)
    {
        if (_Section == nullptr) {
            return false;
        }

        _Memory = MapViewOfFile(
            _Section,
            _ReadOnly ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS,
            0, 0,
            bytes);

        return !!_Memory;
    }

    bool SharedMemory::Unmap()
    {
        if (_Memory == nullptr) {
            return false;
        }

        UnmapViewOfFile(_Memory);
        _Memory = nullptr;

        return true;
    }

    uint32_t SharedMemory::Size() const
    {
        if (_Section == nullptr) {
            return 0;
        }

        SECTION_BASIC_INFORMATION info{};

        if (!NT_SUCCESS(ZwQuerySection(_Section, SectionBasicInformation,
            &info, sizeof(info), nullptr))) {
            return 0;
        }

        return info.MaximumSize.LowPart;
    }

    void*& SharedMemory::Data()
    {
        return _Memory;
    }

    HANDLE SharedMemory::Handle() const
    {
        return _Section;
    }

    void SharedMemory::Close()
    {
        if (_Memory != nullptr) {
            UnmapViewOfFile(_Memory);
            _Memory = nullptr;
        }

        if (_Section != nullptr) {
            CloseHandle(_Section);
            _Section = nullptr;
        }
    }

    bool SharedMemory::ShareToProcessCommon(_In_ HANDLE process, _Out_ HANDLE* new_handle, _In_ bool close_self)
    {
        *new_handle     = nullptr;
        DWORD access    = STANDARD_RIGHTS_REQUIRED | FILE_MAP_READ;
        DWORD options   = 0;
        HANDLE mapped_file = _Section;
        HANDLE result   = nullptr;

        if (!_ReadOnly) {
            access |= FILE_MAP_WRITE;
        }

        if (close_self) {
            // DUPLICATE_CLOSE_SOURCE causes DuplicateHandle to close mapped_file.
            options  = DUPLICATE_CLOSE_SOURCE;
            _Section = nullptr;
            Unmap();
        }

        if (process == GetCurrentProcess() && close_self) {
            *new_handle = mapped_file;
            return true;
        }

        if (!DuplicateHandle(GetCurrentProcess(), mapped_file, process, &result, access, FALSE, options)) {
            return false;
        }

        *new_handle = result;
        return true;
    }

    bool SharedMemory::Lock(_In_ uint32_t timeout_ms, _In_opt_ SECURITY_ATTRIBUTES* sec_attr)
    {
        if (_Lock == nullptr) {
            auto name_wcs = mbstowcs(_Name, CP_UTF8).append(L"_lock");

            _Lock = CreateMutexW(sec_attr, FALSE, name_wcs.c_str());
            if (_Lock == nullptr) {
                // there is nothing good we can do here.
                return false;
            }
        }

        // Return false for WAIT_ABANDONED, WAIT_TIMEOUT or WAIT_FAILED.
        return (WaitForSingleObject(_Lock, timeout_ms) == WAIT_OBJECT_0);
    }

    void SharedMemory::Unlock()
    {
        ReleaseMutex(_Lock);
    }

}
