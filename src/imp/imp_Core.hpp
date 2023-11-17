#pragma once

#include <fmt/printf.h>

#include <ankerl/unordered_dense.h>
#include <vendor/ankerl_hashes.hpp>

#include <chrono>

namespace imp
{
    namespace chr = std::chrono;

    template <typename... T>
    void Error(fmt::format_string<T...> fmt_str, T&&... args)
    {
        fmt::println("Error: {}", fmt::format(fmt_str, std::forward<T>(args)...));
        std::terminate();
    }

// -----------------------------------------------------------------------------

    template<typename... Ts>
    struct OverloadSet : Ts... {
        using Ts::operator()...;
    };

    template<typename... Ts> OverloadSet(Ts...) -> OverloadSet<Ts...>;

// -----------------------------------------------------------------------------

    template<class Ret, class... Types>
    struct FuncBase {
        void* body;
        Ret(*fptr)(void*, Types...);

        Ret operator()(Types... args) {
            return fptr(body, std::forward<Types>(args)...);
        }
    };

    template<class Tx>
    struct GetFunctionImpl {};

    template<class Ret, class... Types>
    struct GetFunctionImpl<Ret(Types...)> { using type = FuncBase<Ret, Types...>; };

    template<class Sig>
    struct LambdaRef : GetFunctionImpl<Sig>::type {
        template<class Fn>
        LambdaRef(Fn&& fn)
            : GetFunctionImpl<Sig>::type(&fn,
                [](void*b, auto... args) -> auto {
                    return (*(Fn*)b)(args...);
                })
        {};
    };

// -----------------------------------------------------------------------------

    struct MemoryPool
    {
        std::vector<void*>                  allocations;
        ankerl::unordered_dense::set<void*> freed;

    public:
        ~MemoryPool()
        {
            Clear();
        }

        void Clear()
        {
            for (auto* ptr : allocations) {
                if (!freed.contains(ptr)) {
                    free(ptr);
                }
            }
            allocations.clear();
            freed.clear();
        }

        template<class T>
        T* Allocate(size_t count)
        {
            auto* ptr = static_cast<T*>(malloc(count * sizeof(T)));
            if (ptr) {
                allocations.push_back(ptr);
            }
            return ptr;
        }

        void Free(void* ptr)
        {
            if (ptr) {
                free(ptr);
                freed.insert(ptr);
            }
        }
    };

    template<class T>
    struct Range
    {
        T*     begin = nullptr;
        size_t count = 0;
        size_t stride = sizeof(T);

        T& operator[](ptrdiff_t index) const noexcept
        {
            return *reinterpret_cast<T*>(reinterpret_cast<std::byte*>(begin) + index * stride);
        }

        Range Slice(size_t first, size_t _count = UINT64_MAX) const noexcept
        {
            return { &(*this)[first], _count == UINT64_MAX ? (count - first) : _count, stride };
        }

        void CopyTo(Range<T> target) const noexcept
        {
            if (stride == sizeof(T) && target.stride == sizeof(T)) {
                std::memcpy(target.begin, begin, count * sizeof(T));
            } else {
                for (size_t i = 0; i < count; ++i) {
                    target[i] = (*this)[i];
                }
            }
        }
    };
}