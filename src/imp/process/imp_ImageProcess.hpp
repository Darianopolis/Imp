#pragma once

#include <imp/imp_Scene.hpp>

#include <span>

namespace imp
{
    struct ImageView
    {
        Range<std::byte> bytes;
        glm::uvec2       size;
        TextureFormat    format;
    };

    struct ImageProcessInput
    {
        ImageView             buffer;
        std::array<int8_t, 4> swizzles = { -1, -1, -1, -1 };
    };

    enum class ImageProcessFlags
    {
        GenMipMaps,
    };

    void Process(std::span<ImageProcessInput> inputs, ImageProcessFlags flags, ImageView& output);
}