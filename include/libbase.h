// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

#include "stdext.h"
#include "console.h"
#include "version.h"
#include "registry.h"
#include "system.h"
#include "strings/util.h"
#include "strings/codepage.h"
#include "memory/search.h"
#include "memory/singleton.h"
#include "memory/shared_memory.h"
#include "process/info.h"
#include "process/launch.h"
#include "process/iter.h"
#include "modules/library.h"
#include "modules/resource.h"
#include "modules/pe_parser.h"
#include "modules/iat_patch_function.h"
#include "files/version_info.h"
#include "notifications/module.h"
