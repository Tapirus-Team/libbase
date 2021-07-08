// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include <shared_mutex>
#include <unordered_map>


typedef VOID(NTAPI* PLDR_DLL_NOTIFICATION_FUNCTION)(
    _In_ ULONG NotificationReason,
    _In_ union _LDR_DLL_NOTIFICATION_DATA* NotificationData,
    _In_opt_ PVOID Context
);

namespace base
{
    class ModuleNotification
    {
        void* _Cookie = nullptr;

        std::shared_mutex _Lock;
        std::unordered_map<PLDR_DLL_NOTIFICATION_FUNCTION, void*> _Callbacks;

    public:
        HRESULT Start();
        HRESULT Stop ();

        VOID RegisterNotificationCallback  (_In_ PLDR_DLL_NOTIFICATION_FUNCTION callback, _In_opt_ void* context);
        VOID UnregisterNotificationCallback(_In_ PLDR_DLL_NOTIFICATION_FUNCTION callback);

    private:
        static VOID NTAPI LoaderNotificationCallback(
            _In_ ULONG NotificationReason,
            _In_ union _LDR_DLL_NOTIFICATION_DATA* NotificationData,
            _In_opt_ PVOID Context
        );

        VOID LoaderNotificationDispatch(
            _In_ ULONG NotificationReason,
            _In_ union _LDR_DLL_NOTIFICATION_DATA* NotificationData,
            _In_opt_ PVOID Context
        );
    };

}
