// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include <type_traits>
#include <memory>
#include <string>
#include <filesystem>


namespace base::stdext
{
    // Implementation of C++23's std::scope_exit.
    //
    // Reference: https://github.com/microsoft/wil/blob/master/include/wil/resource.h#L423
    namespace details
    {
        template <typename TLambda>
        class lambda_call
        {
        public:
            lambda_call(const lambda_call&) = delete;
            lambda_call& operator=(const lambda_call&) = delete;
            lambda_call& operator=(lambda_call&& other) = delete;

            explicit lambda_call(TLambda&& lambda) noexcept : _Lambda(std::move(lambda))
            {
                static_assert(std::is_same<decltype(lambda()), void>::value,
                    "scope_exit lambdas must not have a return value");

                static_assert(!std::is_lvalue_reference<TLambda>::value && !std::is_rvalue_reference<TLambda>::value,
                    "scope_exit should only be directly used with a lambda");
            }

            lambda_call(lambda_call&& other) noexcept : _Lambda(std::move(other._Lambda)), _Call(other._Call)
            {
                other._Call = false;
            }

            ~lambda_call() noexcept
            {
                reset();
            }

            // Ensures the scope_exit lambda will not be called
            void release() noexcept
            {
                _Call = false;
            }

            // Executes the scope_exit lambda immediately if not yet run; ensures it will not run again
            void reset() noexcept
            {
                if (_Call)
                {
                    _Call = false;
                    _Lambda();
                }
            }

            // Returns true if the scope_exit lambda is still going to be executed
            explicit operator bool() const noexcept
            {
                return _Call;
            }

        protected:
            TLambda _Lambda;
            bool    _Call = true;
        };

        // Implementation of scope_resource.
        template<typename T, typename D>
        class resource_t
        {
            std::unique_ptr<T, D> _Handle;

            resource_t(const resource_t&) = delete;
            resource_t& operator=(const resource_t&) = delete;

        public:
            explicit resource_t(T* aValue, D aDeleter) noexcept
                : _Handle(aValue, aDeleter)
            {}

            operator T* () noexcept
            {
                return get();
            }

            T* get() const noexcept
            {
                return _Handle.get();
            }
        };
    }

    // Returns an object that executes the given lambda when destroyed.
    // Capture the object with 'auto'; use reset() to execute the lambda early or release() to avoid
    //   execution.  Exceptions thrown in the lambda will fail-fast; use scope_exit_log to avoid.
    template <typename TLambda>
    [[nodiscard]] inline auto scope_exit(TLambda&& lambda) noexcept
    {
        return details::lambda_call<TLambda>(std::forward<TLambda>(lambda));
    }

    template<typename T, typename D>
    [[nodiscard]] inline auto scope_resource(T* value, D deleter) noexcept
    {
        return details::resource_t(value, deleter);
    }

    // Implementation of C++23's std::to_underlying.
    //
    // Note: This has an additional `std::is_enum<EnumT>` requirement to be SFINAE
    // friendly prior to C++20.
    //
    // Reference: https://en.cppreference.com/w/cpp/utility/to_underlying
    template <typename EnumT, typename = std::enable_if<std::is_enum<EnumT>::value>>
    constexpr auto to_underlying(EnumT e) noexcept
    {
        return static_cast<std::underlying_type_t<EnumT>>(e);
    }
}

namespace base::stdext::filesystem
{
    std::filesystem::path module_path(void* mod = nullptr);
}

namespace base::stdext
{
    namespace fs = filesystem;
}
