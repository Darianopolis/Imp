#pragma once

#include <iostream>

namespace imp
{
    inline
    void Error(std::string_view msg)
    {
        std::cout << "Error: " <<  msg << '\n';
        std::terminate();
    }

    template<class T>
    T* Alloc(size_t count)
    {
        return static_cast<T*>(malloc(count * sizeof(T)));
    }

    inline
    void Free(void* ptr)
    {
        free(ptr);
    }
}