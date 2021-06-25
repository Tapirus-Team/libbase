// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

#include "stdext.h"
#include "module.h"
#include "console.h"
#include "version.h"
#include "strings/util.h"
#include "strings/codepage.h"
#include "memory/singleton.h"
#include "process/info.h"
#include "files/version_info.h"
#include "registry.h"
