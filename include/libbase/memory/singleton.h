// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once


namespace base::memory
{
    template<typename T>
    struct Singleton
    {
        static T& GetInstance() noexcept
        {
            static T _Instance;
            return _Instance;
        }
    };
}

namespace base
{
    using memory::Singleton;
}
