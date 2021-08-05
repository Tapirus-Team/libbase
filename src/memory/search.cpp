// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#include "../universal.inl"


namespace base::memory
{
    void* MemorySearch(
        _In_bytecount_(aBytes) void* aAddress,
        _In_ size_t aBytes,
        _In_ const char* aPattern,
        _In_opt_ bool aOptimization
    )
    {
        static auto CalcPatternHexCount = [](const char* aPattern) -> size_t
        {
            const auto vPatternLength = strlen(aPattern);

            auto vCount = 0u;
            for (auto i = 0u; i < vPatternLength - 1; ++i)
            {
                if (isxdigit(aPattern[i + 0]) &&
                    isxdigit(aPattern[i + 1]))
                {
                    vCount += 1;
                }
                if (aPattern[i + 0] == '?' &&
                    aPattern[i + 1] == '?')
                {
                    vCount += 1;
                }
            }

            return vCount;
        };

        const auto vPatternLength = strlen(aPattern);
        const auto vPatternHexCount = CalcPatternHexCount(aPattern);
        const auto vAddress = static_cast<unsigned __int8*>(aAddress);

        auto vHitAddress = (void*)nullptr;

        __try
        {
            for (auto i = 0u; i < aBytes; ++i)
            {
                if (i + vPatternHexCount > aBytes)
                {
                    break;
                }

                auto v = 0u; // PatternHexIndex
                for (auto x = 0ul; x < vPatternLength - 1;)
                {
                    if (i + v > aBytes)
                    {
                        break;
                    }

                    if (aPattern[x] == ' ')
                    {
                        ++x;
                        continue;
                    }

                    if (!isxdigit(aPattern[x + 0]) ||
                        !isxdigit(aPattern[x + 1]))
                    {
                        if (aPattern[x + 0] == '?' &&
                            aPattern[x + 1] == '?')
                        {
                            v += 1;
                            x += 2;
                            continue;
                        }
                        else
                        {
                            x += 1;
                            continue;
                        }
                    }

                    auto vHex = 0ul;
                    RtlCharToInteger(&aPattern[x], 16, &vHex);

                    if (vHex == vAddress[i + v])
                    {
                        v += 1;
                        x += 2;
                        continue;
                    }

                    break;
                }
                if (v == vPatternHexCount)
                {
                    vHitAddress = &vAddress[i];
                    break;
                }

                if (aOptimization)
                {
                    i += v;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            vHitAddress = nullptr;
        }

        return vHitAddress;
    }
}
