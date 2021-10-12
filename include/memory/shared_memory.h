// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <mutex>
#include <chrono>


namespace base::memory
{
    // refs: https://chromium.googlesource.com/chromium/chromium/+/refs/heads/main/base/shared_memory.h

    // Platform abstraction for shared memory.  Provides a C++ wrapper
    // around the OS primitive for a memory mapped file.
    class SharedMemory
    {
    public:
        SharedMemory() = default;
        SharedMemory(SharedMemory&) = delete;
        SharedMemory& operator=(const SharedMemory&) = delete;

        explicit SharedMemory(const std::string_view name);

        // Create a new SharedMemory object from an existing, open
        // shared memory file.
        SharedMemory(HANDLE handle, bool read_only);

        // Create a new SharedMemory object from an existing, open
        // shared memory file that was created by a remote process and not shared
        // to the current process.
        SharedMemory(HANDLE handle, bool read_only, HANDLE process);

        // Closes any open files.
        ~SharedMemory();

        // Creates a shared memory object as described by the options struct.
        // Returns true on success and false on failure.
        bool Create(_In_opt_ const std::string_view name, _In_ bool open_existing, _In_ uint32_t size);

        // Creates and maps an anonymous shared memory segment of size size.
        // Returns true on success and false on failure.
        bool CreateAndMapAnonymous(_In_ uint32_t size);

        // Creates an anonymous shared memory segment of size size.
        // Returns true on success and false on failure.
        bool CreateAnonymous(_In_ uint32_t size) {
            return Create({}, false, size);
        }

        // Creates or opens a shared memory segment based on a name.
        // If open_existing is true, and the shared memory already exists,
        // opens the existing shared memory and ignores the size parameter.
        // If open_existing is false, shared memory must not exist.
        // size is the size of the block to be created.
        // Returns true on success, false on failure.
        bool CreateNamed(_In_ const std::string_view name, _In_ bool open_existing, _In_ uint32_t size) {
            return Create(name, open_existing, size);
        }

        // Opens a shared memory segment based on a name.
        // If read_only is true, opens for read-only access.
        // Returns true on success, false on failure.
        bool Open(_In_ const std::string_view name, _In_ bool read_only);

        // Maps the shared memory into the caller's address space.
        // Returns true on success, false otherwise.  The memory address
        // is accessed via the memory() accessor.
        bool Map(_In_ uint32_t bytes);

        // Unmaps the shared memory from the caller's address space.
        // Returns true if successful; returns false on error or if the
        // memory is not mapped.
        bool Unmap();

        // Get the size of the shared memory backing file.
        uint32_t Size() const;

        // Gets a pointer to the opened memory space if it has been
        // Mapped via Map().  Returns NULL if it is not mapped.
        void*& Data();

        // Returns the underlying OS handle for this segment.
        // Use of this handle for anything other than an opaque
        // identifier is not portable.
        HANDLE Handle() const;

        // Closes the open shared memory segment.
        // It is safe to call Close repeatedly.
        void Close();

        // Shares the shared memory to another process.  Attempts
        // to create a platform-specific new_handle which can be
        // used in a remote process to access the shared memory
        // file.  new_handle is an ouput parameter to receive
        // the handle for use in the remote process.
        // Returns true on success, false otherwise.
        bool ShareToProcess(_In_ HANDLE process, _Out_ HANDLE* new_handle) {
            return ShareToProcessCommon(process, new_handle, false);
        }

        // Logically equivalent to:
        //   bool ok = ShareToProcess(process, new_handle);
        //   Close();
        //   return ok;
        // Note that the memory is unmapped by calling this method, regardless of the
        // return value.
        bool GiveToProcess(_In_ HANDLE process, _Out_ HANDLE* new_handle) {
            return ShareToProcessCommon(process, new_handle, true);
        }

        // Locks the shared memory.
        //
        // WARNING: on POSIX the memory locking primitive only works across
        // processes, not across threads.  The Lock method is not currently
        // used in inner loops, so we protect against multiple threads in a
        // critical section using a class global lock.
        void Lock() {
            Lock(INFINITE, nullptr);
        }

        // A Lock() implementation with a timeout that also allows setting
        // security attributes on the mutex. sec_attr may be NULL.
        // Returns true if the Lock() has been acquired, false if the timeout was
        // reached.
        bool Lock(_In_ uint32_t timeout_ms, _In_opt_ SECURITY_ATTRIBUTES* sec_attr);

        // Releases the shared memory lock.
        void Unlock();

    public:
        // std::unique_lock traits
        void lock() {
            Lock();
        }

        void unlock() {
            Unlock();
        }

        template <class Rep, class Period>
        bool try_lock_for(const std::chrono::duration<Rep, Period>& rel_time) {
            return try_lock_until(std::chrono::abs(rel_time));
        }

        template <class Clock, class Duration>
        bool try_lock_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
            return Lock(std::chrono::time_point_cast<std::chrono::milliseconds>(abs_time), nullptr);
        }

    private:
        bool ShareToProcessCommon(_In_ HANDLE process, _Out_ HANDLE* new_handle, _In_ bool close_self);

        std::string _Name;
        HANDLE      _Section  = nullptr;
        void*       _Memory   = nullptr;
        bool        _ReadOnly = false;

        HANDLE      _Lock     = nullptr;
    };
}

namespace base
{
    using memory::SharedMemory;
}
