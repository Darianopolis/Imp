#pragma once

#include <imp/imp_Importer.hpp>
#include <imp/imp_BasisMath.hpp>

namespace imp::detail
{
    inline
    void ProcessGeometry(Importer& importer, Scene& scene)
    {
        auto& memory_pool = importer.memory_pool;
        auto& geometries = importer.geometries;

        // Geometries

        scene.geometry_ranges = { memory_pool.Allocate<GeometryRange>(geometries.size()), geometries.size() };

        // Build geometry ranges and compute accumulated geometry stats

        uint32_t index_count = 0;
        uint32_t vertex_count = 0;
        uint32_t max_vertex_count_per_range = 0;
        uint32_t max_index_count_per_range = 0;
        for (uint32_t i = 0; i < geometries.size(); ++i) {
            auto& geometry = geometries[i];
            scene.geometry_ranges[i] = GeometryRange {
                .geometry_idx = 0,
                .vertex_offset = vertex_count,
                .max_vertex = uint32_t(geometry.positions.count - 1),
                .first_index = index_count,
                .triangle_count = uint32_t(geometry.indices.count / 3),
            };
            index_count += uint32_t(geometry.indices.count);
            vertex_count += uint32_t(geometry.positions.count);
            max_vertex_count_per_range = std::max(max_vertex_count_per_range, uint32_t(geometry.positions.count));
            max_index_count_per_range = std::max(max_index_count_per_range, uint32_t(geometry.indices.count));
        }

        // Allocate geometry

        scene.geometries = { memory_pool.Allocate<Geometry>(1), 1 };
        scene.geometries[0] = Geometry {
            .indices        = { memory_pool.Allocate<uint32_t>(index_count),    index_count  },
            .positions      = { memory_pool.Allocate<glm::vec3>(vertex_count), vertex_count },
            .tangent_spaces = { memory_pool.Allocate<Basis>(vertex_count),     vertex_count },
        };

        // Temporary vertex basis scratch space

        struct VertexBasis
        {
            glm::vec3 normal = {};
            glm::vec3 tangent = {};
            glm::vec3 bitangent = {};
        };
        std::vector<VertexBasis> vertex_basis(max_vertex_count_per_range);

        for (uint32_t i = 0; i < geometries.size(); ++i) {
            auto& geometry = geometries[i];
            auto& range = scene.geometry_ranges[i];

            bool has_normals = geometry.normals.count;
            bool has_texcoords = geometry.tex_coords.count;

            if (has_normals) {
                geometry.normals.CopyTo({ &vertex_basis[0].normal, max_vertex_count_per_range, sizeof(VertexBasis) });
            }

            geometry.indices.CopyTo(scene.geometries[0].indices.Slice(range.first_index));
            geometry.positions.CopyTo(scene.geometries[0].positions.Slice(range.vertex_offset));

            // Accumulate area weighted tangent space for each face

            auto update_basis = [&](uint32_t vid, glm::vec3 normal, glm::vec3 tangent, glm::vec3 bitangent, float area)
            {
                auto& v = vertex_basis[vid];

                if (!has_normals) {
                    v.normal += area * normal;
                }
                v.tangent   += area * tangent;
                v.bitangent += area * bitangent;
            };

            for (uint32_t j = 0; j < geometry.indices.count; j += 3) {
                uint32_t v1i = geometry.indices[j + 0];
                uint32_t v2i = geometry.indices[j + 1];
                uint32_t v3i = geometry.indices[j + 2];

                auto v1 = geometry.positions[v1i];
                auto v2 = geometry.positions[v2i];
                auto v3 = geometry.positions[v3i];

                auto v12 = v2 - v1;
                auto v13 = v3 - v1;

                glm::vec3 tangent = {};
                glm::vec3 bitangent = {};

                if (has_texcoords) {
                    auto tc1 = geometry.tex_coords[v1i];
                    auto tc2 = geometry.tex_coords[v2i];
                    auto tc3 = geometry.tex_coords[v3i];

                    auto u12 = tc2 - tc1;
                    auto u13 = tc3 - tc1;

                    float f = 1.f / (u12.x * u13.y - u13.x * u12.y);

                    tangent = f * glm::vec3 {
                        u13.y * v12.x - u12.y * v13.x,
                        u13.y * v12.y - u12.y * v13.y,
                        u13.y * v12.z - u12.y * v13.z,
                    };

                    bitangent = f * glm::vec3 {
                        u13.x * v12.x - u12.x * v13.x,
                        u13.x * v12.y - u12.x * v13.y,
                        u13.x * v12.z - u12.x * v13.z,
                    };
                }

                auto cross = glm::cross(v12, v13);
                auto area = glm::length(0.5f * cross);
                auto normal = glm::normalize(cross);

                if (area) {
                    update_basis(v1i, normal, tangent, bitangent, area);
                    update_basis(v2i, normal, tangent, bitangent, area);
                    update_basis(v3i, normal, tangent, bitangent, area);
                }
            }

            // Quantize generated tangent spaces

#pragma omp parallel for
            for (uint32_t j = 0; j < geometry.positions.count; ++j) {
                auto& basis_in = vertex_basis[j];
                Basis basis_out;

                // Normalize and reorthogonalize generated tangent spaces

                basis_in.normal = glm::normalize(basis_in.normal);
                basis_in.tangent = detail::Reorthogonalize(glm::normalize(basis_in.tangent), basis_in.normal);
                basis_in.bitangent = glm::normalize(basis_in.bitangent);

                auto enc_normal = detail::SignedOctEncode(basis_in.normal);
                basis_out.oct_x = uint32_t(enc_normal.x * 1023.f);
                basis_out.oct_y = uint32_t(enc_normal.y * 1023.f);
                basis_out.oct_s = uint32_t(enc_normal.z);

                // Decode quantized normal before computing tangent to
                //  ensure consistent tangent basis

                auto decoded_normal = detail::SignedOctDecode(glm::vec3 {
                    float(basis_out.oct_x) / 1023.f,
                    float(basis_out.oct_y) / 1023.f,
                    float(basis_out.oct_s),
                });

                auto enc_tangent = detail::EncodeTangent(decoded_normal, basis_in.tangent);
                basis_out.tgt_a = uint32_t(enc_tangent * 1023.f);

                auto enc_bitangent = glm::dot(glm::cross(basis_in.normal, basis_in.tangent), basis_in.bitangent) > 0.f;
                basis_out.btg_s = uint32_t(enc_bitangent);

                scene.geometries[0].tangent_spaces[range.vertex_offset + j] = basis_out;
            }
        }
    }
}