// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#pragma once

// Global define

#define WIN32_LEAN_AND_MEAN

// System Header

#include <third/MINT/MINT.h>
#include <strsafe.h>

// C/C++ Header


// 3rdParty Header


// Global Var/Fun define

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

// Self

#include "inc/libbase.h"
