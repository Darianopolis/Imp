#pragma once

#include <ankerl/unordered_dense.h>

template<class FirstT, class SecondT>
struct ankerl::unordered_dense::hash<std::pair<FirstT, SecondT>>
{
    using is_avalanching = void;
    uint64_t operator()(const std::pair<FirstT, SecondT>& key) const noexcept
    {
        return detail::wyhash::mix(
            hash<FirstT>{}(key.first),
            hash<SecondT>{}(key.second));
    }
};