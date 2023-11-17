#pragma once

#include "imp_Core.hpp"

#include <vendor/glm_include.hpp>

namespace imp
{
    struct Basis
    {
        uint32_t oct_x : 10;
        uint32_t oct_y : 10;
        uint32_t oct_s :  1;
        uint32_t tgt_a : 10;
        uint32_t btg_s :  1;
    };

    using UNorm8  = uint8_t;
    using UNorm16 = uint16_t;
    using SNorm16 = uint16_t;
    using Float16 = uint16_t;
    using Float32 = float;
    template<class T>
    using Vec2 = std::array<T, 2>;
    template<class T>
    using Vec3 = std::array<T, 3>;
    template<class T>
    using Vec4 = std::array<T, 4>;

    struct Geometry
    {
        Range<uint32_t>      indices;
        Range<glm::vec3>     positions;
        Range<Basis>         tangent_spaces;
        Range<Vec2<Float16>> tex_coords;
    };

    struct GeometryRange
    {
        uint32_t geometry_idx;
        uint32_t vertex_offset;
        uint32_t max_vertex;
        uint32_t first_index;
        uint32_t triangle_count;
    };

    enum class TextureFormat
    {
        RGBA8_SRGB,
        RGBA8_UNORM,
        RG8_UNORM,
        R8_UNORM,
    };

    struct Texture
    {
        glm::uvec2       size;
        TextureFormat    format;
        Range<std::byte> data;
    };

    struct Material
    {
        int32_t      albedo_alpha_texture = -1;
        Vec4<UNorm8> albedo_alpha;

        int32_t      metalness_texture = -1;
        int32_t      roughness_texture = -1;
        Vec2<UNorm8> metalness_roughness;

        int32_t normal_texture = -1;

        int32_t      emission_texture = -1;
        Vec3<UNorm8> emission_factor;

        int32_t transmission_texture = -1;
        UNorm8  transmission_factor;
    };

    struct Mesh
    {
        uint32_t    geometry_range_idx;
        glm::mat4x3 transform;
    };

    struct Scene
    {
        Range<Geometry>      geometries;
        Range<GeometryRange> geometry_ranges;
        Range<Texture>       textures;
        Range<Material>      materials;
        Range<Mesh>          meshes;
    };
}