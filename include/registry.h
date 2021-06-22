// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once


namespace base
{
    // Utility class to read, write and manipulate the Windows Registry.
    // Registry vocabulary primer: a "key" is like a folder, in which there
    // are "values", which are <name, data> pairs, with an associated data type.
    //
    // Note:
    //  * ReadValue family of functions guarantee that the out-parameter
    //    is not touched in case of failure.
    //  * Functions returning LONG indicate success as ERROR_SUCCESS or an
    //    error as a (non-zero) win32 error code.
    //
    // Reference: 
    class RegKey
    {
        HKEY    _Key = nullptr;
        REGSAM  _Wow64Access = 0;

    public:
        RegKey() = default;
        explicit RegKey(_In_ HKEY key);
        RegKey(_In_ HKEY rootkey, _In_ const wchar_t* subkey, _In_ REGSAM access);
        RegKey(_Inout_ RegKey&& other) noexcept;
        RegKey& operator=(_Inout_ RegKey&& other) noexcept;
        ~RegKey();

        HRESULT Create(
            _In_ HKEY rootkey,
            _In_ const wchar_t* subkey,
            _In_ REGSAM access,
            _Out_opt_ DWORD* disposition = nullptr
        );

        HRESULT Open(
            _In_ HKEY rootkey,
            _In_ const wchar_t* subkey,
            _In_ REGSAM access
        );

        // Creates a subkey or open it if it already exists.
        HRESULT CreateKey(
            _In_ const wchar_t* subkey,
            _In_ REGSAM access
        );

        // Opens an existing reg key, given the relative key name.
        HRESULT OpenKey(
            _In_ const wchar_t* subkey,
            _In_ REGSAM access
        );

        // Closes this reg key.
        void Close();

        // Replaces the handle of the registry key and takes ownership of the handle.
        void Set(_In_ HKEY key);

        // Transfers ownership away from this object.
        HKEY Take();

        // Returns false if this key does not have the specified value, or if an error
        // occurrs while attempting to access it.
        bool HasValue(_In_ const wchar_t* value_name) const;

        // Returns the number of values for this key, or 0 if the number cannot be
        // determined.
        DWORD GetValueCount() const;

        // Determines the nth value's name.
        HRESULT GetValueNameAt(_In_ int index, _Out_ std::wstring* name) const;

        // True while the key is valid.
        bool Valid() const;

        // Kills a key and everything that lives below it; please be careful when
        // using it.
        HRESULT DeleteKey(_In_ const wchar_t* name);

        // Deletes an empty subkey. If the subkey has subkeys or values then this
        // will fail.
        HRESULT DeleteEmptyKey(_In_ const wchar_t* name);

        // Deletes a single value within the key.
        HRESULT DeleteValue(_In_ const wchar_t* value_name);

        // Getters:

        // Reads a REG_DWORD (uint32_t) into |out_value|. If |name| is null or empty,
        // reads the key's default value, if any.
        HRESULT ReadValue(_In_opt_ const wchar_t* name, _Out_ DWORD* out_value) const;

        // Reads a REG_QWORD (int64_t) into |out_value|. If |name| is null or empty,
        // reads the key's default value, if any.
        HRESULT ReadValue(_In_opt_ const wchar_t* name, _Out_ int64_t* out_value) const;

        // Reads a string into |out_value|. If |name| is null or empty, reads
        // the key's default value, if any.
        HRESULT ReadValue(_In_opt_ const wchar_t* name, _Out_ std::wstring* out_value) const;

        // Reads a REG_MULTI_SZ registry field into a vector of strings. Clears
        // |values| initially and adds further strings to the list. Returns
        // ERROR_CANTREAD if type is not REG_MULTI_SZ.
        HRESULT ReadValues(_In_opt_ const wchar_t* name, _Out_ std::vector<std::wstring>* values);

        // Reads raw data into |data|. If |name| is null or empty, reads the key's
        // default value, if any.
        HRESULT ReadValue(
            _In_opt_    const wchar_t* name,
            _Out_opt_   void* data,
            _Inout_opt_ DWORD* dsize,
            _Out_opt_   DWORD* dtype
        ) const;

        // Setters:

        // Sets an int32_t value.
        HRESULT WriteValue(_In_opt_ const wchar_t* name, _In_ DWORD in_value);

        // Sets an int64_t value.
        HRESULT WriteValue(_In_opt_ const wchar_t* name, _In_ int64_t in_value);

        // Sets a string value.
        HRESULT WriteValue(_In_opt_ const wchar_t* name, _In_ const wchar_t* in_value);

        // Sets raw data, including type.
        HRESULT WriteValue(
            _In_opt_ const wchar_t* name,
            _In_ const void* data,
            _In_ DWORD dsize,
            _In_ DWORD dtype
        );

        HKEY Handle() const noexcept;

        explicit operator HANDLE() const noexcept;

    };
}
