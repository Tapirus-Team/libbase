// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <third/MINT/MINT.h>
#include <include/libbase.h>

#include <iostream>

int main(int /*argc*/, char* /*argv*/[])
{
    base::SetConsoleCodePage();
    base::SetProcessPrivilege(GetCurrentProcessToken(), SE_DEBUG_NAME, true);

    auto process_iter = base::process_iterator().snapshot();
    for (auto& process : process_iter) {
        std::cout << "id      : " << process.process_id() << std::endl;
        std::cout << "name    : " << process.name() << std::endl;
        std::cout << "path    : " << process.path().u8string() << std::endl;
        std::cout << "cmdline : " << process.cmdline() << std::endl;
        std::cout << std::endl;
    }

    return 0;
}
