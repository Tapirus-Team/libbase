// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once


namespace base::console
{
    void SetConsoleUTF8();
    void RedirectIOToConsole(_In_ short MaxConsoleLines);
}