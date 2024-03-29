// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include <string>


namespace base::process
{
    std::string RunSystemCommandGetResult(_In_ const std::string_view cmd);
}

namespace base
{
    using process::RunSystemCommandGetResult;
}
