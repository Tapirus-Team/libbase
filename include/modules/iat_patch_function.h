// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// refs: https://chromium.googlesource.com/chromium/chromium/+/refs/heads/main/base/win/pe_image.h

#pragma once


namespace base::modules
{
    // A class that encapsulates Import Address Table patching helpers and restores
    // the original function in the destructor.
    //
    // It will intercept functions for a specific DLL imported from another DLL.
    // This is the case when, for example, we want to intercept
    // CertDuplicateCertificateContext function (exported from crypt32.dll) called
    // by wininet.dll.
    class IATPatchFunction {
    public:
        IATPatchFunction() = default;
        ~IATPatchFunction();

        IATPatchFunction(const IATPatchFunction&) = delete;
        IATPatchFunction& operator=(const IATPatchFunction&) = delete;

        // Intercept a function in an import table of a specific
        // module. Save the original function and the import
        // table address. These values will be used later
        // during Unpatch
        //
        // Arguments:
        // module                 Module to be intercepted
        // imported_from_module   Module that exports the 'function_name'
        // function_name          Name of the API to be intercepted
        //
        // Returns: Windows error code (winerror.h). NO_ERROR if successful
        //
        // Note: Patching a function will make the IAT patch take some "ownership" on
        // |module|.  It will LoadLibrary(module) to keep the DLL alive until a call
        // to Unpatch(), which will call FreeLibrary() and allow the module to be
        // unloaded.  The idea is to help prevent the DLL from going away while a
        // patch is still active.
        DWORD Patch(
            _In_ const char* module,
            _In_ const char* imported_from_module,
            _In_ const char* function_name,
            _In_ void* new_function);
        // Unpatch the IAT entry using internally saved original
        // function.
        //
        // Returns: Windows error code (winerror.h). NO_ERROR if successful
        DWORD Unpatch();

        bool IsPatched() const;

    private:
        HMODULE _Module = nullptr;
        void*   _InterceptFunction  = nullptr;
        void*   _OriginalFunction   = nullptr;
        IMAGE_THUNK_DATA* _IatThunk = nullptr;
    };
}
