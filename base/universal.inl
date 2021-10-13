// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#pragma once

// System Header
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define UMDF_USING_NTSTATUS
#include <windows.h>
#include <windowsx.h>
#include <winioctl.h>
#include <ntstatus.h>
#include <intrin.h>
#include <strsafe.h>

// C/C++ Header
#include <cwctype>
#include <algorithm>

// Self
#include "include/libbase.h"
