// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#include "base/universal.inl"


namespace base
{
    static auto _LdrRegisterDllNotification   = (decltype(::LdrRegisterDllNotification)*)nullptr;
    static auto _LdrUnregisterDllNotification = (decltype(::LdrUnregisterDllNotification)*)nullptr;

    VOID NTAPI ModuleNotification::LoaderNotificationCallback(
        _In_ ULONG NotificationReason,
        _In_ PLDR_DLL_NOTIFICATION_DATA NotificationData,
        _In_opt_ PVOID Context
        )
    {
        static_cast<ModuleNotification*>(Context)->LoaderNotificationDispatch(
            NotificationReason,
            NotificationData,
            Context);
    }

    VOID ModuleNotification::LoaderNotificationDispatch(
        _In_ ULONG NotificationReason,
        _In_ PLDR_DLL_NOTIFICATION_DATA NotificationData,
        _In_opt_ PVOID /*Context*/
    )
    {
        auto Guard = std::shared_lock(_Lock);

        for (const auto& Callback : _Callbacks)
        {
            [&]()
            {
                __try
                {
                    Callback.first(NotificationReason, NotificationData, Callback.second);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    __nop();
                }
            }();
        }
    }

    HRESULT ModuleNotification::Start()
    {
        if (_Cookie)
        {
            return S_OK;
        }

        if (_LdrRegisterDllNotification == nullptr ||
            _LdrUnregisterDllNotification == nullptr)
        {
            auto Ntdll = base::stdext::scope_resource(
                base::dlopen("ntdll"), [](void* handle) { base::dlclose(handle); });
            if (Ntdll == nullptr)
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }

            _LdrRegisterDllNotification = (decltype(::LdrRegisterDllNotification)*)
                base::dlsym(Ntdll, "LdrRegisterDllNotification");

            _LdrUnregisterDllNotification = (decltype(::LdrUnregisterDllNotification)*)
                base::dlsym(Ntdll, "LdrUnregisterDllNotification");

            if (_LdrRegisterDllNotification == nullptr ||
                _LdrUnregisterDllNotification == nullptr)
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }
        }

        NTSTATUS Result = _LdrRegisterDllNotification(0, &LoaderNotificationCallback, this, &_Cookie);

        return NT_SUCCESS(Result) ? S_OK : HRESULT_FROM_NT(Result);
    }

    HRESULT ModuleNotification::Stop()
    {
        if (_LdrUnregisterDllNotification && _Cookie)
        {
            NTSTATUS Result = _LdrUnregisterDllNotification(_Cookie);
            if (!NT_SUCCESS(Result))
            {
                return HRESULT_FROM_NT(Result);
            }

            _Cookie = nullptr;
        }

        return S_OK;
    }

    VOID ModuleNotification::RegisterNotificationCallback(_In_ PLDR_DLL_NOTIFICATION_FUNCTION callback, _In_opt_ void* context)
    {
        auto Guard = std::unique_lock(_Lock);
        _Callbacks[callback] = context;
    }

    VOID ModuleNotification::UnregisterNotificationCallback(_In_ PLDR_DLL_NOTIFICATION_FUNCTION callback)
    {
        auto Guard = std::unique_lock(_Lock);
        _Callbacks.erase(callback);
    }
}
