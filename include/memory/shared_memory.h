// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once


namespace base::memory
{
    template<typename T, size_t _SIZE = sizeof(T)>
    class SharedMemory
    {
        HANDLE _Section = nullptr;
        void*  _Address = nullptr;

    public:
        ~SharedMemory();

        HRESULT CreateSharedMemory(
            _In_ const char* name,
            _In_ ACCESS_MASK desired = FILE_MAP_READ | FILE_MAP_WRITE,
            _In_opt_ LPSECURITY_ATTRIBUTES attributes = nullptr
        );

        HRESULT OpenSharedMemory(
            _In_ const char* name,
            _In_ ACCESS_MASK desired = FILE_MAP_READ | FILE_MAP_WRITE
        );

        VOID CloseSharedMemory();

        bool IsMapping() const noexcept;

    public:
        T* Get() const noexcept
        {
            return static_cast<T*>(_Address);
        }

        operator T* () const noexcept
        {
            return Get();
        }

        T* operator->() const throw()
        {
            return Get();
        }

        T& GetRef() const throw()
        {
            return *Get();
        }
    };
}

namespace base::memory
{
    template<typename T, size_t _SIZE /*= sizeof(T)*/>
    base::memory::SharedMemory<T, _SIZE>::~SharedMemory()
    {
        CloseSharedMemory();
    }

    template<typename T, size_t _SIZE /*= sizeof(T)*/>
    HRESULT SharedMemory<T, _SIZE>::CreateSharedMemory(
        _In_ const char* name,
        _In_ ACCESS_MASK desired,
        _In_opt_ LPSECURITY_ATTRIBUTES attributes)
    {
        HRESULT Result = S_OK;

        do
        {
            DWORD Protect = 0;
            if ((desired & FILE_MAP_ALL_ACCESS) == FILE_MAP_ALL_ACCESS)
            {
                desired = desired & (~SECTION_MAP_EXECUTE);
                Protect = PAGE_READWRITE;
            }
            else
            {
                if (desired == FILE_MAP_READ)
                {
                    Protect = PAGE_READONLY;
                }
                if (desired & FILE_MAP_WRITE)
                {
                    desired = desired | FILE_MAP_READ;
                    Protect = PAGE_READWRITE;
                }
            }

            LARGE_INTEGER MaximumSize = { _SIZE };
            HANDLE Section = CreateFileMappingA(INVALID_HANDLE_VALUE, attributes, Protect,
                MaximumSize.HighPart, MaximumSize.LowPart, name);
            if (Section == nullptr)
            {
                Result = HRESULT_FROM_WIN32(GetLastError());
                break;
            }

            void* Address = MapViewOfFile(Section, desired, 0, 0, _SIZE);
            if (Address == nullptr)
            {
                Result = HRESULT_FROM_WIN32(GetLastError());

                CloseHandle(Section);
                break;
            }

            _Section = Section;
            _Address = Address;

        } while (false);

        return Result;
    }

    template<typename T, size_t _SIZE /*= sizeof(T)*/>
    HRESULT base::memory::SharedMemory<T, _SIZE>::OpenSharedMemory(
        _In_ const char* name,
        _In_ ACCESS_MASK desired)
    {
        HRESULT Result = S_OK;

        do
        {
            HANDLE Section = OpenFileMappingA(desired, FALSE, name);
            if (Section == nullptr)
            {
                Result = HRESULT_FROM_WIN32(GetLastError());
                break;
            }

            void* Address = MapViewOfFile(Section, desired, 0, 0, _SIZE);
            if (Address == nullptr)
            {
                Result = HRESULT_FROM_WIN32(GetLastError());

                CloseHandle(Section);
                break;
            }

            _Section = Section;
            _Address = Address;

        } while (false);

        return Result;
    }

    template<typename T, size_t _SIZE /*= sizeof(T)*/>
    VOID base::memory::SharedMemory<T, _SIZE>::CloseSharedMemory()
    {
        if (_Section)
        {
            UnmapViewOfFile(_Address);
            CloseHandle(_Section);

            _Section = nullptr;
            _Address = nullptr;
        }
    }

    template<typename T, size_t _SIZE /*= sizeof(T)*/>
    bool base::memory::SharedMemory<T, _SIZE>::IsMapping() const noexcept
    {
        return !!_Section;
    }
}
