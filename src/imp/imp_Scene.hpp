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

    using UNorm16 = uint16_t;
    using SNorm16 = uint16_t;
    using Float16 = uint16_t;
    using Float32 = float;
    template<class T>
    using Vec2 = std::array<T, 2>;
    template<class T>
    using Vec3 = std::array<T, 3>;

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

    struct Mesh
    {
        uint32_t    geometry_range_idx;
        glm::mat4x3 transform;
    };

    struct Scene
    {
        Range<Geometry>      geometries;
        Range<GeometryRange> geometry_ranges;
        Range<Mesh>          meshes;
    };
}