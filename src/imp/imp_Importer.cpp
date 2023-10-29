#include "imp_Importer.hpp"

#include "process/imp_ProcessGeometry.hpp"
#include "process/imp_ProcessMaterials.hpp"

namespace imp
{
    namespace loaders
    {
        std::vector<ModelLoaderFn> loader_fns;
    }

    std::monostate loaders::RegisterModelLoader(ModelLoaderFn fn)
    {
        loader_fns.emplace_back(fn);
        return {};
    }

// -----------------------------------------------------------------------------

    void Importer::SetBaseDir(const std::filesystem::path& path)
    {
        base_dir = path;
    }

    void Importer::LoadFile(const std::filesystem::path& path)
    {
        fmt::println("Loading file [{}]", path.string());

        for (auto& loader_fn : loaders::loader_fns) {
            auto _loader = loader_fn();
            if (_loader->Import(*this, path)) {
                loader = std::move(_loader);
                break;
            }
        }

        if (!loader) {
            fmt::println("Could not find loader for [{}]", path.string());
        }
    }

    void Importer::ReportStatistics()
    {
        uint64_t unique_vertex_count = 0;
        uint64_t unique_index_count = 0;
        uint64_t effective_triangle_count = 0;

        for (auto& geometry : geometries) {
            unique_vertex_count += geometry.positions.count;
            unique_index_count += geometry.indices.count;
        }

        for (auto& mesh : meshes) {
            auto& geom = geometries[mesh.geometry_idx];
            effective_triangle_count += geom.indices.count / 3;
        }

        fmt::print("{}", fmt::format(
            std::locale("en_US.UTF-8"),
            "Scene:\n"
            "  Geometries: {:L}\n"
            "  Meshes:     {:L}\n"
            "  Unique Vertices: {:L}\n"
            "  Unique Indices:  {:L}\n"
            "  Triangles:       {:L}\n",
            geometries.size(),
            meshes.size(),
            unique_vertex_count,
            unique_index_count,
            effective_triangle_count));
    }

    void Importer::ReportDetailed()
    {
        for (uint32_t i = 0; i < geometries.size(); ++i) {
            auto& geometry = geometries[i];

            fmt::print(
                "Geometry[{}]:\n"
                "  Positions: {}\n"
                "  Indices:   {}\n",
                i,
                geometry.positions.count,
                geometry.indices.count);
        }

        for (uint32_t i = 0; i < meshes.size(); ++i) {
            auto& mesh = meshes[i];
            auto& M = mesh.transform;

            fmt::print(
                "Mesh[{}]:\n"
                "  Geometry: {}\n"
                "  Transform:\n"
                "    {:12.5f} {:12.5f} {:12.5f} {:12.5f}\n"
                "    {:12.5f} {:12.5f} {:12.5f} {:12.5f}\n"
                "    {:12.5f} {:12.5f} {:12.5f} {:12.5f}\n",
                i,
                mesh.geometry_idx,
                M[0][0], M[1][0], M[2][0], M[3][0],
                M[0][1], M[1][1], M[2][1], M[3][1],
                M[0][2], M[1][2], M[2][2], M[3][2]);
        }
    }

    Scene Importer::GenerateScene()
    {
        Scene scene;

        detail::ProcessGeometry(*this, scene);
        detail::ProcessMaterials(*this, scene);

        scene.meshes = { memory_pool.Allocate<Mesh>(meshes.size()), meshes.size() };

        for (uint32_t i = 0; i < meshes.size(); ++i) {
            scene.meshes[i] = Mesh {
                .geometry_range_idx = meshes[i].geometry_idx,
                .transform = meshes[i].transform,
            };
        }

        return scene;
    }
}