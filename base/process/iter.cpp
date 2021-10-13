// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/universal.inl"


namespace base::process
{
    process_entry::process_entry(uint32_t id)
        : _ProcessId(id)
    {
        _Handle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, id);
    }

    process_entry::process_entry(HANDLE handle)
    {
        if (handle) {
            DuplicateHandle(
                GetCurrentProcess(), handle,
                GetCurrentProcess(), &_Handle,
                PROCESS_QUERY_INFORMATION,
                FALSE,
                0);
        }
    }

    process_entry::process_entry(const process_entry& other)
        : process_entry(other._Handle)
    {
        _ProcessId = other._ProcessId;
        _Name = other._Name;
    }

    process_entry::process_entry(process_entry&& other) noexcept
    {
        std::swap(_Handle, other._Handle);
        std::swap(_ProcessId, other._ProcessId);

        _Name = std::move(other._Name);
    }

    process_entry::process_entry(uint32_t id, const std::string_view& name)
        : process_entry(id)
    {
        _Name = name;
    }

    process_entry& process_entry::operator=(const process_entry& other)
    {
        if (_Handle) {
            CloseHandle(_Handle);
        }
        _Handle     = nullptr;
        _ProcessId  = other._ProcessId;
        _Name = other._Name;

        auto temp = process_entry(other._Handle);
        std::swap(_Handle, temp._Handle);

        return *this;
    }

    process_entry& process_entry::operator=(process_entry&& other) noexcept
    {
        if (_Handle) {
            CloseHandle(_Handle);
        }
        _Handle     = nullptr;
        _ProcessId  = 0u;
        _Name = std::move(other._Name);

        std::swap(_Handle, other._Handle);
        std::swap(_ProcessId, other._ProcessId);

        return *this;
    }

    process_entry::~process_entry() noexcept
    {
        if (_Handle) {
            CloseHandle(_Handle);
        }
    }

    uint32_t process_entry::parent_id() const noexcept
    {
        uint32_t id = 0;

        if (_Handle != nullptr) {

            auto buffer = QueryInformationProcess(ProcessBasicInformation, _Handle);
            if (buffer != nullptr) {
                id = static_cast<uint32_t>(reinterpret_cast<size_t>(
                    reinterpret_cast<PPROCESS_BASIC_INFORMATION>(buffer.get())->InheritedFromUniqueProcessId));
            }
        }

        return id;
    }

    uint32_t process_entry::process_id() const noexcept
    {
        if (_ProcessId) {
            return _ProcessId;
        }

        return static_cast<uint32_t>(GetProcessId(_Handle));
    }

    uint32_t process_entry::session_id() const noexcept
    {
        uint32_t id = 0;

        if (_Handle != nullptr) {

            auto buffer = QueryInformationProcess(ProcessSessionInformation, _Handle);
            if (buffer != nullptr) {
                id = static_cast<uint32_t>(
                    reinterpret_cast<PPROCESS_SESSION_INFORMATION>(buffer.get())->SessionId);
            }
        }

        return id;
    }

    bool process_entry::is_wow64_process() const noexcept
    {
        if (_Handle != nullptr) {

            auto buffer = QueryInformationProcess(ProcessWow64Information, _Handle);
            if (buffer != nullptr) {
                return !!(*reinterpret_cast<ULONG_PTR*>(buffer.get()));
            }
        }

        return false;
    }

    bool process_entry::is_process_deleting() const noexcept
    {
        if (_Handle) {
            unsigned long code = STILL_ACTIVE;
            GetExitCodeProcess(_Handle, &code);
            if (code != STILL_ACTIVE) {
                return false;
            }
        }

        return true;
    }

    std::string process_entry::name() const noexcept
    {
        if (!_Name.empty()) {
            return _Name;
        }

        return path().filename().u8string();
    }

    std::filesystem::path process_entry::path() const noexcept
    {
        std::wstring name;

        if (_Handle != nullptr) {

            auto buffer = QueryInformationProcess(ProcessImageFileNameWin32, _Handle);
            if (buffer != nullptr) {
                auto name_u = reinterpret_cast<PUNICODE_STRING>(buffer.get());
                name = { name_u->Buffer, name_u->Length / sizeof(wchar_t) };
            }
        }

        return name;
    }

    std::string process_entry::cmdline() const noexcept
    {
        std::string cmdline;

        if (_Handle != nullptr) {

            auto buffer = QueryInformationProcess(ProcessCommandLineInformation, _Handle);
            if (buffer != nullptr) {
                auto name_u = reinterpret_cast<PUNICODE_STRING>(buffer.get());
                cmdline = wcstombs({ name_u->Buffer, name_u->Length / sizeof(wchar_t) });
            }
        }

        return cmdline;
    }

    HANDLE process_entry::native_handle() const noexcept
    {
        return _Handle;
    }

    HANDLE process_entry::detach() noexcept
    {
        HANDLE handle = nullptr;
        std::swap(_Handle, handle);
        return handle;
    }

    const process_iterator::value_type& process_iterator::operator*() const noexcept
    {
        return _Entry;
    }

    const process_iterator::value_type* process_iterator::operator->() const noexcept
    {
        return &**this;
    }

    process_iterator& process_iterator::operator++()
    {
        if (_Buffer != nullptr && _Position != nullptr)
        {
            _Entry = {};

            auto information = reinterpret_cast<PSYSTEM_PROCESS_INFORMATION>(_Position);
            if (information->NextEntryOffset == 0) {
                _Position = nullptr;
            }

            _Position   = _Position + information->NextEntryOffset;
            information = reinterpret_cast<PSYSTEM_PROCESS_INFORMATION>(_Position);
            if (information) {
                _Entry = process_entry(static_cast<uint32_t>(
                    reinterpret_cast<size_t>(information->UniqueProcessId)),
                    wcstombs({information->ImageName.Buffer, information->ImageName.Length / sizeof(wchar_t)}));
            }
        }

        return *this;
    }

    bool process_iterator::operator==(const process_iterator& other) const noexcept
    {
        return _Position == other._Position;
    }

    bool process_iterator::operator!=(const process_iterator& other) const noexcept
    {
        return _Position != other._Position;
    }

    base::process::process_iterator& process_iterator::snapshot() noexcept
    {
        _Entry    = {};
        _Position = nullptr;
        _Buffer   = nullptr;

        _Buffer = QueryInformationSystem(SystemProcessInformation);
        if (_Buffer != nullptr) {
            _Position = _Buffer.get();

            auto information = reinterpret_cast<PSYSTEM_PROCESS_INFORMATION>(_Position);
            _Entry = process_entry(static_cast<uint32_t>(
                reinterpret_cast<size_t>(information->UniqueProcessId)),
                "System Idle Process");
        }

        return *this;
    }

    process_iterator::process_iterator(const process_iterator& other)
    {
        *this = other;
    }

    process_iterator::process_iterator(process_iterator&& other) noexcept
    {
        *this = std::move(other);
    }

    process_iterator& process_iterator::operator=(const process_iterator& other)
    {
        _Entry      = other._Entry;
        _Position   = other._Position;
        _Buffer     = other._Buffer;

        return *this;
    }

    process_iterator& process_iterator::operator=(process_iterator&& other) noexcept
    {
        _Entry      = std::move(other._Entry);
        _Position   = std::move(other._Position);
        _Buffer     = std::move(other._Buffer);

        other._Position = nullptr;

        return *this;
    }

    base::process::process_iterator begin(process_iterator iter) noexcept
    {
        return iter;
    }

    base::process::process_iterator end(const process_iterator&) noexcept
    {
        return {};
    }
}
