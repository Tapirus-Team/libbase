// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include <string>
#include <memory>


namespace base::process
{
    class process_entry
    {
        HANDLE _Handle = nullptr;

        uint32_t _ProcessId = 0u;
        std::string _Name;

    public: // Modifiers
        ~process_entry() noexcept;

        process_entry() noexcept = default;
        process_entry(const process_entry& other);
        process_entry(process_entry&& other) noexcept;

        process_entry& operator=(const process_entry& other);
        process_entry& operator=(process_entry&& other) noexcept;

        explicit process_entry(uint32_t id);
        explicit process_entry(HANDLE handle);
        explicit process_entry(uint32_t id, const std::string_view& name);

        operator HANDLE() const noexcept {
            return native_handle();
        }

    public: // Observers
        uint32_t parent_id() const noexcept;
        uint32_t process_id() const noexcept;
        uint32_t session_id() const noexcept;

        bool is_wow64_process() const noexcept;
        bool is_process_deleting() const noexcept;

        std::string name() const noexcept;
        std::string cmdline() const noexcept;
        std::filesystem::path path() const noexcept;

        HANDLE native_handle() const noexcept;
        HANDLE detach() noexcept;
    };

    class process_iterator
    {
        process_entry   _Entry;
        uint8_t*        _Position = nullptr;
        std::shared_ptr<uint8_t> _Buffer;

    public: // Modifiers
        using value_type        = process_entry;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const process_entry*;
        using reference         = const process_entry&;
        using iterator_category = std::input_iterator_tag;

        process_iterator() noexcept = default;
        process_iterator(const process_iterator&);
        process_iterator(process_iterator&&) noexcept;

        process_iterator& operator=(const process_iterator&);
        process_iterator& operator=(process_iterator&&) noexcept;

        const value_type& operator*() const noexcept;
        const value_type* operator->() const noexcept;

        process_iterator& operator++();

        bool operator==(const process_iterator& other) const noexcept;
        bool operator!=(const process_iterator& other) const noexcept;

    public: // Observers
        process_iterator& snapshot() noexcept;
    };

    process_iterator begin(process_iterator iter) noexcept;
    process_iterator end(const process_iterator& iter) noexcept;
}

namespace base
{
    using process::process_entry;
    using process::process_iterator;
}
