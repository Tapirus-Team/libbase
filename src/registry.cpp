// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "universal.inl"


namespace base
{

    namespace
    {
        // RegEnumValue() reports the number of characters from the name that were
        // written to the buffer, not how many there are. This constant is the maximum
        // name size, such that a buffer with this size should read any name.
        const DWORD MAX_REGISTRY_NAME_SIZE = 16384;

        // Registry values are read as BYTE* but can have wchar_t* data whose last
        // wchar_t is truncated. This function converts the reported |byte_size| to
        // a size in wchar_t that can store a truncated wchar_t if necessary.
        inline DWORD to_wchar_size(DWORD byte_size)
        {
            return (byte_size + sizeof(wchar_t) - 1) / sizeof(wchar_t);
        }

        // Recursively deletes a key and all of its subkeys.
        static HRESULT RegDelRecurse(_In_ HKEY root_key, _In_ const wchar_t* name, _In_ REGSAM access)
        {// First, see if the key can be deleted without having to recurse.
            LONG result = RegDeleteKeyExW(root_key, name, access, 0);
            if (result == ERROR_SUCCESS)
                return S_OK;

            HKEY target_key = nullptr;
            result = RegOpenKeyExW(root_key, name, 0, KEY_ENUMERATE_SUB_KEYS | access, &target_key);

            if (result == ERROR_FILE_NOT_FOUND)
                return S_OK;
            if (result != ERROR_SUCCESS)
                return HRESULT_FROM_WIN32(result);

            std::wstring subkey_name(name);

            // Check for an ending slash and add one if it is missing.
            if (!subkey_name.empty() && subkey_name.back() != '\\')
                subkey_name.push_back('\\');

            // Enumerate the keys
            result = ERROR_SUCCESS;

            const DWORD  MaxKeyNameLength = MAX_PATH;
            const size_t base_key_length = subkey_name.length();

            std::wstring key_name;            
            while (result == ERROR_SUCCESS)
            {
                key_name.reserve(MaxKeyNameLength);
                key_name.resize(MaxKeyNameLength - 1);

                DWORD key_size = MaxKeyNameLength;
                result = RegEnumKeyExW(target_key, 0, &key_name[0],
                        &key_size, nullptr, nullptr, nullptr, nullptr);

                if (result != ERROR_SUCCESS)
                    break;

                key_name.resize(key_size);
                subkey_name.resize(base_key_length);
                subkey_name += key_name;

                if (RegDelRecurse(root_key, subkey_name.c_str(), access) != ERROR_SUCCESS)
                    break;
            }

            RegCloseKey(target_key);

            // Try again to delete the key.
            result = RegDeleteKeyExW(root_key, name, access, 0);

            return HRESULT_FROM_WIN32(result);
        }

    }  // namespace

    RegKey::RegKey(_In_ HKEY key)
        : _Key(key)
    {

    }

    RegKey::RegKey(_In_ HKEY rootkey, _In_ const wchar_t* subkey, _In_ REGSAM access)
    {
        if (rootkey)
        {
            if (access & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_CREATE_LINK))
                Create(rootkey, subkey, access);
            else
                Open(rootkey, subkey, access);
        }
        else
        {
            _Wow64Access = access & KEY_WOW64_RES;
        }
    }

    RegKey::RegKey(_Inout_ RegKey&& other) noexcept
    {
        std::swap(*this, other);
    }

    RegKey::~RegKey()
    {
        Close();
    }

    RegKey& RegKey::operator=(_Inout_ RegKey&& other) noexcept
    {
        Close();

        std::swap(_Key, other._Key);
        std::swap(_Wow64Access, other._Wow64Access);

        return *this;
    }

    HRESULT RegKey::Create(
        _In_ HKEY rootkey,
        _In_ const wchar_t* subkey,
        _In_ REGSAM access,
        _Out_opt_ DWORD* disposition /*= nullptr */)
    {
        HKEY subhkey = nullptr;
        LONG result  = RegCreateKeyExW(rootkey, subkey, 0, nullptr, REG_OPTION_NON_VOLATILE,
                access, nullptr, &subhkey, disposition);
        if (result == ERROR_SUCCESS)
        {
            Close();
            _Key = subhkey;
            _Wow64Access = access & KEY_WOW64_RES;
        }

        return HRESULT_FROM_WIN32(result);
    }

    HRESULT RegKey::Open(_In_ HKEY rootkey, _In_ const wchar_t* subkey, _In_ REGSAM access)
    {
        HKEY subhkey = nullptr;
        LONG result  = RegOpenKeyExW(rootkey, subkey, 0, access, &subhkey);
        if (result == ERROR_SUCCESS) {
            Close();
            _Key = subhkey;
            _Wow64Access = access & KEY_WOW64_RES;
        }

        return HRESULT_FROM_WIN32(result);
    }

    HRESULT RegKey::CreateKey(_In_ const wchar_t* subkey, _In_ REGSAM access)
    { 
        // After the application has accessed an alternate registry view using one of
        // the [KEY_WOW64_32KEY / KEY_WOW64_64KEY] flags, all subsequent operations
        // (create, delete, or open) on child registry keys must explicitly use the
        // same flag. Otherwise, there can be unexpected behavior.
        // http://msdn.microsoft.com/en-us/library/windows/desktop/aa384129.aspx.
        if ((access & KEY_WOW64_RES) != _Wow64Access)
        {
            return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }

        HKEY subhkey = nullptr;
        LONG result  = RegCreateKeyExW(_Key, subkey, 0, nullptr, REG_OPTION_NON_VOLATILE,
            access, nullptr, &subhkey, nullptr);
        if (result == ERROR_SUCCESS)
        {
            Close();
            _Key = subhkey;
            _Wow64Access = access & KEY_WOW64_RES;
        }

        return HRESULT_FROM_WIN32(result);
    }

    HRESULT RegKey::OpenKey(_In_ const wchar_t* subkey, _In_ REGSAM access)
    {
        // After the application has accessed an alternate registry view using one of
        // the [KEY_WOW64_32KEY / KEY_WOW64_64KEY] flags, all subsequent operations
        // (create, delete, or open) on child registry keys must explicitly use the
        // same flag. Otherwise, there can be unexpected behavior.
        // http://msdn.microsoft.com/en-us/library/windows/desktop/aa384129.aspx.
        if ((access & KEY_WOW64_RES) != _Wow64Access)
        {
            return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }

        HKEY subhkey = nullptr;
        LONG result = RegOpenKeyExW(_Key, subkey, 0, access, &subhkey);

        // We have to close the current opened key before replacing it with the new
        // one.
        if (result == ERROR_SUCCESS)
        {
            Close();
            _Key = subhkey;
            _Wow64Access = access & KEY_WOW64_RES;
        }

        return HRESULT_FROM_WIN32(result);
    }

    void RegKey::Close()
    {
        if (_Key)
        {
            ::RegCloseKey(_Key);

            _Key = nullptr;
            _Wow64Access = 0;
        }
    }

    void RegKey::Set(_In_ HKEY key)
    {
        if (_Key != key)
        {
            Close();
            _Key = key;
        }
    }

    HKEY RegKey::Take()
    {
        HKEY key = _Key;
        _Key = nullptr;
        return key;
    }

    bool RegKey::HasValue(_In_ const wchar_t* value_name) const
    {
        return RegQueryValueExW(_Key, value_name, nullptr, nullptr, nullptr, nullptr)
            == ERROR_SUCCESS;
    }

    DWORD RegKey::GetValueCount() const
    {
        DWORD count = 0;
        LONG result = RegQueryInfoKey(_Key, nullptr, nullptr, nullptr, nullptr, nullptr,
                nullptr, &count, nullptr, nullptr, nullptr, nullptr);
        return (result == ERROR_SUCCESS) ? count : 0;
    }

    HRESULT RegKey::GetValueNameAt(_In_ int index, _Out_ std::wstring* name) const
    {
        wchar_t buf[256]{};
        DWORD bufsize = (DWORD)std::size(buf);
        LONG  result  = ::RegEnumValueW(_Key, index, buf, &bufsize,
            nullptr, nullptr, nullptr,nullptr);

        if (result == ERROR_SUCCESS)
            name->assign(buf, bufsize);

        return HRESULT_FROM_WIN32(result);
    }

    bool RegKey::Valid() const
    {
        return _Key != nullptr;
    }

    HRESULT RegKey::DeleteKey(_In_ const wchar_t* name)
    {
        HKEY subkey = nullptr;

        // Verify the key exists before attempting delete to replicate previous
        // behavior.
        LONG result = RegOpenKeyExW(_Key, name, 0, READ_CONTROL | _Wow64Access, &subkey);
        if (result != ERROR_SUCCESS)
            return HRESULT_FROM_WIN32(result);

        RegCloseKey(subkey);

        return RegDelRecurse(_Key, name, _Wow64Access);
    }

    HRESULT RegKey::DeleteEmptyKey(_In_ const wchar_t* name)
    {
        HKEY target_key = nullptr;
        LONG result = RegOpenKeyExW(_Key, name, 0, KEY_READ | _Wow64Access, &target_key);

        if (result != ERROR_SUCCESS)
            return HRESULT_FROM_WIN32(result);

        DWORD count = 0;
        result = RegQueryInfoKey(target_key, nullptr, nullptr, nullptr, nullptr, nullptr,
                nullptr, &count, nullptr, nullptr, nullptr, nullptr);

        RegCloseKey(target_key);

        if (result != ERROR_SUCCESS)
            return HRESULT_FROM_WIN32(result);

        if (count == 0)
            return HRESULT_FROM_WIN32(RegDeleteKeyExW(_Key, name, _Wow64Access, 0));

        return HRESULT_FROM_WIN32(ERROR_DIR_NOT_EMPTY);
    }

    HRESULT RegKey::DeleteValue(_In_ const wchar_t* value_name)
    {
        return HRESULT_FROM_WIN32(RegDeleteValueW(_Key, value_name));
    }

    HRESULT RegKey::ReadValue(_In_opt_ const wchar_t* name, _Out_ DWORD* out_value) const
    {
        *out_value = 0ul;

        DWORD type = REG_DWORD;
        DWORD size = sizeof(DWORD);
        DWORD local_value = 0;
        LONG result = ReadValue(name, &local_value, &size, &type);

        if (result == ERROR_SUCCESS)
        {
            if ((type == REG_DWORD || type == REG_BINARY) && size == sizeof(DWORD))
                *out_value = local_value;
            else
                result = ERROR_CANTREAD;
        }

        return HRESULT_FROM_WIN32(result);
    }

    HRESULT RegKey::ReadValue(_In_opt_ const wchar_t* name, _Out_ int64_t* out_value) const
    {
        *out_value = 0;

        DWORD type = REG_QWORD;
        DWORD size = sizeof(int64_t);
        int64_t local_value = 0;
        LONG result = ReadValue(name, &local_value, &size, &type);

        if (result == ERROR_SUCCESS)
        {
            if ((type == REG_QWORD || type == REG_BINARY) && size == sizeof(local_value))
                *out_value = local_value;
            else
                result = ERROR_CANTREAD;
        }

        return HRESULT_FROM_WIN32(result);
    }

    HRESULT RegKey::ReadValue(_In_opt_ const wchar_t* name, _Out_ std::wstring* out_value) const
    {
        const size_t MAX_STRING_LENGTH = 1024;  // This is after expansion.

        out_value->clear();

        // Use the one of the other forms of ReadValue if 1024 is too small for you.
        wchar_t raw_value[MAX_STRING_LENGTH]{};

        DWORD type = REG_SZ;
        DWORD size = sizeof(raw_value);
        LONG result = ReadValue(name, raw_value, &size, &type);

        if (result == ERROR_SUCCESS)
        {
            if (type == REG_SZ)
            {
                *out_value = raw_value;
            }
            else if (type == REG_EXPAND_SZ)
            {
                wchar_t expanded[MAX_STRING_LENGTH]{};
                size = ExpandEnvironmentStringsW(raw_value, expanded, MAX_STRING_LENGTH);
                // Success: returns the number of wchar_t's copied
                // Fail: buffer too small, returns the size required
                // Fail: other, returns 0
                if (size == 0 || size > MAX_STRING_LENGTH)
                {
                    result = ERROR_MORE_DATA;
                }
                else
                {
                    *out_value = expanded;
                }
            }
            else
            {
                // Not a string. Oops.
                result = ERROR_CANTREAD;
            }
        }

        return HRESULT_FROM_WIN32(result);
    }

    HRESULT RegKey::ReadValue(_In_opt_ const wchar_t* name, _Out_opt_ void* data, _Inout_opt_ DWORD* dsize, _Out_opt_ DWORD* dtype) const
    {
        return HRESULT_FROM_WIN32(RegQueryValueExW(
            _Key, name, nullptr, dtype, reinterpret_cast<LPBYTE>(data), dsize));
    }

    HRESULT RegKey::ReadValues(_In_opt_ const wchar_t* name, _Out_ std::vector<std::wstring>* values)
    {
        values->clear();

        DWORD type = REG_MULTI_SZ;
        DWORD size = 0;
        LONG result = ReadValue(name, nullptr, &size, &type);

        if (result != ERROR_SUCCESS || size == 0)
            return HRESULT_FROM_WIN32(result);

        if (type != REG_MULTI_SZ)
            return HRESULT_FROM_WIN32(ERROR_CANTREAD);

        std::vector<wchar_t> buffer(size / sizeof(wchar_t));

        result = ReadValue(name, buffer.data(), &size, nullptr);
        if (result != ERROR_SUCCESS || size == 0)
            return HRESULT_FROM_WIN32(result);

        // Parse the double-null-terminated list of strings.
        // Note: This code is paranoid to not read outside of |buf|, in the case where
        // it may not be properly terminated.
        auto entry = buffer.cbegin();
        auto buffer_end = buffer.cend();

        while (entry < buffer_end && *entry != '\0')
        {
            auto entry_end = std::find(entry, buffer_end, '\0');
            values->emplace_back(entry, entry_end);
            entry = entry_end + 1;
        }

        return S_OK;
    }

    HRESULT RegKey::WriteValue(_In_opt_ const wchar_t* name, _In_ DWORD in_value)
    {
        return WriteValue(name, &in_value, static_cast<DWORD>(sizeof(in_value)),
            REG_DWORD);
    }

    HRESULT RegKey::WriteValue(_In_opt_ const wchar_t* name, _In_ int64_t in_value)
    {
        return WriteValue(name, &in_value, static_cast<DWORD>(sizeof(in_value)),
            REG_QWORD);
    }

    HRESULT RegKey::WriteValue(_In_opt_ const wchar_t* name, _In_ const wchar_t* in_value)
    {
        return WriteValue(
            name, in_value,
            static_cast<DWORD>(sizeof(*in_value) * (std::char_traits<wchar_t>::length(in_value) + 1)),
            REG_SZ);
    }

    HRESULT RegKey::WriteValue(_In_opt_ const wchar_t* name, _In_ const void* data, _In_ DWORD dsize, _In_ DWORD dtype)
    {
        LONG result = RegSetValueExW(_Key, name, 0, dtype,
            reinterpret_cast<LPBYTE>(const_cast<void*>(data)), dsize);

        return HRESULT_FROM_WIN32(result);
    }

    HKEY RegKey::Handle() const noexcept
    {
        return _Key;
    }

    RegKey::operator HANDLE() const noexcept
    {
        return Handle();
    }

}
