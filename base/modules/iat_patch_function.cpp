// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/universal.inl"


namespace base::modules
{
    namespace
    {
        struct InterceptFunctionInformation {
            bool finished_operation;
            const char* imported_from_module;
            const char* function_name;
            void* new_function;
            void** old_function;
            IMAGE_THUNK_DATA** iat_thunk;
            DWORD return_code;
        };

        void* GetIATFunction(IMAGE_THUNK_DATA* iat_thunk) {
            if (iat_thunk == nullptr) {
                return nullptr;
            }
            // Works around the 64 bit portability warning:
            // The Function member inside IMAGE_THUNK_DATA is really a pointer
            // to the IAT function. IMAGE_THUNK_DATA correctly maps to IMAGE_THUNK_DATA32
            // or IMAGE_THUNK_DATA64 for correct pointer size.
            union FunctionThunk {
                IMAGE_THUNK_DATA thunk;
                void* pointer;
            } iat_function;
            iat_function.thunk = *iat_thunk;
            return iat_function.pointer;
        }

        bool InterceptEnumCallback(
            const PEImage& /*image*/,
            const char* module,
            DWORD /*ordinal*/,
            const char* name,
            DWORD /*hint*/,
            IMAGE_THUNK_DATA* iat,
            void* cookie
        ) {
            auto intercept_information = reinterpret_cast<InterceptFunctionInformation*>(cookie);
            if (intercept_information == nullptr) {
                return false;
            }

            if ((lstrcmpiA(module, intercept_information->imported_from_module) == 0) &&
                (name != nullptr) &&
                (lstrcmpiA(name, intercept_information->function_name) == 0)) {

                // Save the old pointer.
                if (intercept_information->old_function != nullptr) {
                    *(intercept_information->old_function) = GetIATFunction(iat);
                }
                if (intercept_information->iat_thunk != nullptr) {
                    *(intercept_information->iat_thunk) = iat;
                }

                // portability check
                static_assert(sizeof(iat->u1.Function) == sizeof(intercept_information->new_function));

                // Patch the function.
                intercept_information->return_code = ModifyCode(
                    &(iat->u1.Function),
                    &(intercept_information->new_function),
                    sizeof(intercept_information->new_function));

                // Terminate further enumeration.
                intercept_information->finished_operation = true;
                return false;
            }

            return true;
        }

        // Helper to intercept a function in an import table of a specific
        // module.
        //
        // Arguments:
        // module_handle          Module to be intercepted
        // imported_from_module   Module that exports the symbol
        // function_name          Name of the API to be intercepted
        // new_function           Interceptor function
        // old_function           Receives the original function pointer
        // iat_thunk              Receives pointer to IAT_THUNK_DATA
        //                        for the API from the import table.
        //
        // Returns: Returns NO_ERROR on success or Windows error code
        //          as defined in winerror.h
        DWORD InterceptImportedFunction(
            HMODULE module_handle,
            const char* imported_from_module,
            const char* function_name,
            void* new_function,
            void** old_function,
            IMAGE_THUNK_DATA** iat_thunk
        ) {
            if ((module_handle == nullptr) || (imported_from_module == nullptr) ||
                (function_name == nullptr) || (new_function == nullptr)) {
                return ERROR_INVALID_PARAMETER;
            }

            PEImage target_image(module_handle);
            if (!target_image.VerifyMagic()) {
                return ERROR_INVALID_PARAMETER;
            }

            InterceptFunctionInformation intercept_information = {
              false,
              imported_from_module,
              function_name,
              new_function,
              old_function,
              iat_thunk,
              ERROR_GEN_FAILURE };

            // First go through the IAT. If we don't find the import we are looking
            // for in IAT, search delay import table.
            target_image.EnumAllImports(InterceptEnumCallback, &intercept_information);

            if (!intercept_information.finished_operation) {
                target_image.EnumAllDelayImports(InterceptEnumCallback, &intercept_information);
            }
            return intercept_information.return_code;
        }

        // Restore intercepted IAT entry with the original function.
        //
        // Arguments:
        // intercept_function     Interceptor function
        // original_function      Receives the original function pointer
        //
        // Returns: Returns NO_ERROR on success or Windows error code
        //          as defined in winerror.h
        DWORD RestoreImportedFunction(
            void* intercept_function,
            void* original_function,
            IMAGE_THUNK_DATA* iat_thunk
        ) {
            if ((intercept_function == nullptr) || (original_function == nullptr) || (iat_thunk == nullptr)) {
                return ERROR_INVALID_PARAMETER;
            }

            if (GetIATFunction(iat_thunk) != intercept_function) {
                // Check if someone else has intercepted on top of us.
                // We cannot unpatch in this case, just raise a red flag.
                return ERROR_INVALID_FUNCTION;
            }

            return ModifyCode(&(iat_thunk->u1.Function),
                &original_function,
                sizeof(original_function));
        }
    }  // namespace

    IATPatchFunction::~IATPatchFunction() {
        if (IsPatched()) {
            Unpatch();
        }
    }

    DWORD IATPatchFunction::Patch(
        _In_ const char* module,
        _In_ const char* imported_from_module,
        _In_ const char* function_name,
        _In_ void* new_function
    ) {

        auto name_wcs = mbstowcs(module);

        HMODULE module_handle = LoadLibraryW(name_wcs.c_str());
        if (module_handle == nullptr) {
            return GetLastError();
        }

        DWORD error = InterceptImportedFunction(
            module_handle,
            imported_from_module,
            function_name,
            new_function,
            &_OriginalFunction,
            &_IatThunk);
        if (NO_ERROR == error) {
            _Module = module_handle;
            _InterceptFunction = new_function;
        }
        else {
            FreeLibrary(module_handle);
        }

        return error;
    }

    DWORD IATPatchFunction::Unpatch() {

        DWORD error = RestoreImportedFunction(
            _InterceptFunction,
            _OriginalFunction,
            _IatThunk);

        // Hands off the intercept if we fail to unpatch.
        // If IATPatchFunction::Unpatch fails during RestoreImportedFunction
        // it means that we cannot safely unpatch the import address table
        // patch. In this case its better to be hands off the intercept as
        // trying to unpatch again in the destructor of IATPatchFunction is
        // not going to be any safer
        if (_Module) {
            FreeLibrary(_Module);
        }

        _Module = nullptr;
        _InterceptFunction = nullptr;
        _OriginalFunction = nullptr;
        _IatThunk = nullptr;

        return error;
    }

    bool IATPatchFunction::IsPatched() const
    {
        return (_InterceptFunction != nullptr);
    }

}

namespace base
{
    using modules::IATPatchFunction;
}
